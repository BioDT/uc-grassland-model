#include "step.h"

STEP::STEP() {};
STEP::~STEP() {};

/**
 * @brief Runs the full simulation period by iterating over all simulation days.
 *
 * Increments `parameter.day` each iteration and delegates to
 * runModelSimulationStep() for the per-day logic. The total number of days is
 * given by `parameter.simulationTimeInDays`.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; `day` is incremented each iteration.
 * @param init        Provides per-step reset functions.
 * @param allometry   Allometric helper passed through to daily sub-steps.
 * @param community   Plant community; updated in place each day.
 * @param recruitment Recruitment state; updated each day.
 * @param mortality   Mortality module; applied each day.
 * @param growth      Growth module; applied each day.
 * @param management  Management module; applied each day.
 * @param soil        Soil state; updated each day.
 * @param weather     Daily weather time-series (read-only inside each step).
 * @param interaction Interaction state (shading, temperature); updated each day.
 * @param output      Output buffers and file streams; filled and flushed each day.
 */
void STEP::runModelSimulation(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output)
{
    /* Daily steps to be simulated */
    for (int day = 1; day <= parameter.simulationTimeInDays; day++)
    {
        parameter.day = day; // increase day according to for-loop

        runModelSimulationStep(utils, parameter, init, allometry, community, recruitment, mortality, growth, management, soil, weather, interaction, output);
    }
}

/**
 * @brief Executes all sub-processes for a single simulation day.
 *
 * Performs the following steps in order:
 * 1. resetVegetationProcessVariables() + resetSoilResourceProcessAndFluxVariables()
 *    — zero all per-step accumulators.
 * 2. getEnvironmentalConditionsOfDay() — read weather and compute soil temperature.
 * 3. doDayStepOfModelSimulation() — run all ecological processes.
 * 4. updateVegetationStateVariablesForOutput() — aggregate community/PFT output fields.
 * 5. saveSimulationResultsToBuffer() — append today's results to output buffers.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; `day` must already be set to the current day.
 * @param init        Provides per-step reset functions.
 * @param allometry   Allometric helper passed through to ecological sub-steps.
 * @param community   Plant community; updated in place.
 * @param recruitment Recruitment state; updated in place.
 * @param mortality   Mortality module.
 * @param growth      Growth module.
 * @param management  Management module.
 * @param soil        Soil state; updated in place.
 * @param weather     Daily weather time-series.
 * @param interaction Interaction state; updated in place.
 * @param output      Output buffers; filled by saveSimulationResultsToBuffer().
 */
void STEP::runModelSimulationStep(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output)
{
    /* Resetting of process- and flux-specific state variables of the vegetation community and soil resources */
    init.resetVegetationProcessVariables(parameter, recruitment, community, interaction, soil);
    init.resetSoilResourceProcessAndFluxVariables(utils, parameter, soil);

    /* Environmental conditions of the day */
    double abovegroundLitterBiomass = (soil.carbonContent_surfaceStructuralLitterPool + soil.carbonContent_surfaceMetabolicLitterPool) * (1.0 / CARBON_CONTENT_ODM);
    interaction.getEnvironmentalConditionsOfDay(weather, community, parameter.day, abovegroundLitterBiomass);

    /* Calculation of ecological and plant processes */
    doDayStepOfModelSimulation(utils, parameter, allometry, community, recruitment, mortality, growth, interaction, management, soil, weather);
    output.updateVegetationStateVariablesForOutput(parameter, community, soil, management, recruitment);

    /* Writing of daily output of simulation results */
    saveSimulationResultsToBuffer(utils, parameter, community, interaction, output, soil);
}

/**
 * @brief Runs all ecological process modules for a single simulation day.
 *
 * Executes, in order:
 * 1. doPlantRecruitment() — seed influx, germination, new cohort creation.
 * 2. doPlantMortality() — senescence, litter fall, crowding and basic mortality.
 * 3. calculateLightAttenuationAndAvailabilityForPlants() — canopy shading pipeline.
 * 4. doPlantGrowth() — photosynthesis, respiration, NPP, allocation, geometry update.
 * 5. applyManagementRegime() — mowing, fertilisation, irrigation.
 * 6. calculateSoilResourceDynamics() — soil water and C/N dynamics.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; `day` used throughout.
 * @param allometry   Allometric helper for cohort geometry.
 * @param community   Plant community; cohort state updated by all modules.
 * @param recruitment Recruitment state; updated by doPlantRecruitment().
 * @param mortality   Mortality module.
 * @param growth      Growth module.
 * @param interaction Interaction state; LAI and radiation updated.
 * @param management  Management module.
 * @param soil        Soil state; litter and resource pools updated.
 * @param weather     Daily weather data.
 */
void STEP::doDayStepOfModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, INTERACTION &interaction, MANAGEMENT management, SOIL &soil, WEATHER weather)
{
    /* Plant recruitment */
    recruitment.doPlantRecruitment(utils, parameter, allometry, community, management, soil);

    /* Plant mortality */
    mortality.doPlantMortality(utils, parameter, community, allometry, growth, interaction, soil);

    /* Light conditions & plant shading */
    interaction.calculateLightAttenuationAndAvailabilityForPlants(utils, parameter, community, interaction.fullSunLight);

    /* Plant photosynthesis, respiration, NPP and allocation */
    growth.doPlantGrowth(utils, parameter, weather, community, interaction, allometry, soil);

    /* Management activities */
    management.applyManagementRegime(utils, community, allometry, parameter, soil);

    /* Soil resource dynamics */
    soil.calculateSoilResourceDynamics(utils, parameter, weather, community, interaction);
}

/**
 * @brief Appends one row of community-level output variables to
 *        `output.bufferCommunity`.
 *
 * Written columns (tab-separated): Date, DayCount, NumberPlants,
 * NumberCohorts, LeafAreaIndex, VegetationHeight, VegetationCover,
 * CBalance, NBalance.
 * Only executed when `parameter.communityOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved for future error handling).
 * @param parameter Provides `day`, `communityOutputFile` flag.
 * @param community Read-only; provides community-level aggregates.
 * @param output    `bufferCommunity` is appended.
 * @param date      ISO-formatted date string (`"YYYY-MM-DD"`).
 */
void STEP::fillCommunityBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date)
{
    if (parameter.communityOutputFile)
    {
        output.bufferCommunity << date << "\t" << parameter.day << "\t";
        output.bufferCommunity << community.totalNumberOfPlantsInCommunity << "\t" << community.totalNumberOfCohortsInCommunity << "\t" << community.totalLeafAreaIndexOfPlantsInCommunity << "\t";
        output.bufferCommunity << community.maximumHeightOfAllPlants << "\t" << (100 * (community.coveredAreaOfAllPlants / SIMULATION_AREA)) << "\t" << community.ecosystemCarbonBalance << "\t" << community.ecosystemNitrogenBalance << std::endl;
    }
}

/**
 * @brief Appends one row per PFT of population-level output variables to
 *        `output.bufferPFTPopulation`.
 *
 * Written columns (tab-separated): Date, DayCount, PFT, Fraction,
 * NumberPlants, CoveredArea, ShootBiomass, GreenShootBiomass,
 * BrownShootBiomass, ClippedShootBiomass, RootBiomass, RecruitmentBiomass,
 * ExudationBiomass, GPP, NPP, Respiration.
 * Only executed when `parameter.pftOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day`, `pftCount`, `pftOutputFile` flag.
 * @param community Read-only; provides per-PFT accumulator vectors.
 * @param output    `bufferPFTPopulation` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillPFTBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date)
{
    if (parameter.pftOutputFile)
    {
        for (int pft = 0; pft < parameter.pftCount; pft++)
        {
            output.bufferPFTPopulation << date << "\t" << parameter.day << "\t" << pft << "\t";
            output.bufferPFTPopulation << community.pftComposition[pft] << "\t" << community.numberOfPlantsPerPFT[pft] << "\t";
            output.bufferPFTPopulation << community.coveredAreaOfPlantsPerPFT[pft] << "\t" << community.shootBiomassOfPlantsPerPFT[pft] << "\t";
            output.bufferPFTPopulation << community.greenShootBiomassOfPlantsPerPFT[pft] << "\t" << community.brownShootBiomassOfPlantsPerPFT[pft] << "\t";
            output.bufferPFTPopulation << community.clippedShootBiomassOfPlantsPerPFT[pft] << "\t" << community.rootBiomassOfPlantsPerPFT[pft] << "\t";
            output.bufferPFTPopulation << community.recruitmentBiomassOfPlantsPerPFT[pft] << "\t" << community.exudationBiomassOfPlantsPerPFT[pft] << "\t";
            output.bufferPFTPopulation << community.gppOfPlantsPerPFT[pft] << "\t" << community.nppOfPlantsPerPFT[pft] << "\t" << community.carbonRespirationOfPlantsPerPFT[pft];
            output.bufferPFTPopulation << std::endl;
        }
    }
}

/**
 * @brief Appends one row per plant cohort of individual-level output variables
 *        to `output.bufferPlant`.
 *
 * Written columns (tab-separated): Date, DayCount, PFT, Age, NumberPlants,
 * Height, Width, LAI, CoveredArea, RootDepth, NumberSoilLayers,
 * ShootBiomass, GreenShootBiomass, BrownShootBiomass, ClippedShootBiomass,
 * RootBiomass, RecruitmentBiomass, ExudationBiomass, GPP, NPP, Respiration,
 * Radiation, ShadingIndicator, LimitingFactorWater, LimitingFactorNitrogen,
 * AllocationShoot, AllocationRoot, AllocationRecruitment, AllocationExudation.
 * Only executed when `parameter.plantCohortOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day` and `plantCohortOutputFile` flag.
 * @param community Read-only; iterates over `allPlants`.
 * @param output    `bufferPlant` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillPlantCohortBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date)
{
    if (parameter.plantCohortOutputFile)
    {
        for (int cohortindex = 0; cohortindex < community.allPlants.size(); cohortindex++)
        {
            output.bufferPlant << date << "\t" << parameter.day << "\t" << community.allPlants.at(cohortindex)->pft << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->age << "\t" << community.allPlants.at(cohortindex)->amount << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->height << "\t" << community.allPlants.at(cohortindex)->width << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->lai << "\t" << community.allPlants.at(cohortindex)->coveredArea << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->rootingDepth << "\t" << community.allPlants.at(cohortindex)->numberOfSoilLayersRooting << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->shootBiomass << "\t" << community.allPlants.at(cohortindex)->shootBiomassGreenLeaves << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->shootBiomassBrownLeaves << "\t" << community.allPlants.at(cohortindex)->shootBiomassAboveClippingHeight << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->rootBiomass << "\t" << community.allPlants.at(cohortindex)->recruitmentBiomass << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->exudationBiomass << "\t" << community.allPlants.at(cohortindex)->gpp << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->npp << "\t" << community.allPlants.at(cohortindex)->totalRespiration << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->availableRadiation << "\t" << community.allPlants.at(cohortindex)->shadingIndicator << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->limitingFactorGppWater << "\t" << community.allPlants.at(cohortindex)->limitingFactorNppNitrogen << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->nppAllocationShoot << "\t" << community.allPlants.at(cohortindex)->nppAllocationRoot << "\t";
            output.bufferPlant << community.allPlants.at(cohortindex)->nppAllocationRecruitment << "\t" << community.allPlants.at(cohortindex)->nppAllocationExudation;
            output.bufferPlant << std::endl;
        }
    }
}

/**
 * @brief Appends one row of soil carbon pool contents and inter-pool fluxes
 *        to `output.bufferSoilCarbon`.
 *
 * Columns follow the header defined in
 * OUTPUT::writeHeaderInOutputFiles() for the soil carbon file.
 * Only executed when `parameter.soilCarbonOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day` and `soilCarbonOutputFile` flag.
 * @param soil      Read-only; provides all C pool contents and fluxes.
 * @param output    `bufferSoilCarbon` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillSoilCarbonBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date)
{
    if (parameter.soilCarbonOutputFile)
    {
        output.bufferSoilCarbon << date << "\t" << parameter.day << "\t";
        output.bufferSoilCarbon << soil.carbonContent_surfaceGreenLitterPool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_surfaceBrownLitterPool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_soilRootLitterPool << "\t"
                                << soil.carbonContent_soilSeedLitterPool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_surfaceStructuralLitterPool << "\t"
                                << soil.carbonContent_surfaceMetabolicLitterPool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_soilStructuralLitterPool << "\t"
                                << soil.carbonContent_soilMetabolicLitterPool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_soilMicrobesPool << "\t"
                                << soil.carbonContent_soilActivePool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_soilSlowPool << "\t"
                                << soil.carbonContent_soilPassivePool << "\t";
        output.bufferSoilCarbon << soil.carbonContent_leachedFromSoil << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilStructuralLitterPool_to_soilActivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilSlowPool_to_soilPassivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilSlowPool_to_soilPassiveAndActivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilActivePool_to_soilPassivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilActivePool_to_soilSlowPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilActivePool_to_soilPassiveAndSlowPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilMetabolicLitterPool_to_soilActivePool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilMicrobesPool_to_soilSlowPool << "\t";
        output.bufferSoilCarbon << soil.carbonFlux_soilPassivePool_to_soilActivePool << "\t";
        output.bufferSoilCarbon << soil.leachingCarbon;
        output.bufferSoilCarbon << std::endl;
    }
}

/**
 * @brief Appends one row of soil nitrogen pool contents, inter-pool fluxes,
 *        and mineralisation variables to `output.bufferSoilNitrogen`.
 *
 * Columns follow the header defined in
 * OUTPUT::writeHeaderInOutputFiles() for the soil nitrogen file.
 * Only executed when `parameter.soilNitrogenOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day` and `soilNitrogenOutputFile` flag.
 * @param soil      Read-only; provides all N pool contents, fluxes, and
 *                  mineralisation/volatilisation variables.
 * @param output    `bufferSoilNitrogen` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillSoilNitrogenBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date)
{
    if (parameter.soilNitrogenOutputFile)
    {
        output.bufferSoilNitrogen << date << "\t" << parameter.day << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_surfaceGreenLitterPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_surfaceBrownLitterPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_soilRootLitterPool << "\t"
                                  << soil.nitrogenContent_soilSeedLitterPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_surfaceStructuralLitterPool << "\t"
                                  << soil.nitrogenContent_surfaceMetabolicLitterPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_soilStructuralLitterPool << "\t"
                                  << soil.nitrogenContent_soilMetabolicLitterPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_soilMicrobesPool << "\t"
                                  << soil.nitrogenContent_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_soilSlowPool << "\t"
                                  << soil.nitrogenContent_soilPassivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenContent_leachedFromSoil << "\t";

        output.bufferSoilNitrogen << soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilStructuralLitterPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilSlowPool_to_soilPassivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilActivePool_to_soilPassivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilActivePool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilMicrobesPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilPassivePool_to_soilActivePool << "\t";

        output.bufferSoilNitrogen << soil.leachingNitrogen << "\t";
        output.bufferSoilNitrogen << soil.addedMineralNitrogenToSoilByFertilization << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFixationToSoil << "\t";
        output.bufferSoilNitrogen << soil.nitrogenGrossMineralization << "\t";
        output.bufferSoilNitrogen << soil.nitrogenNetMineralization << "\t";
        output.bufferSoilNitrogen << soil.nitrogenVolatilization << "\t";

        output.bufferSoilNitrogen << soil.nitrogenFlow_soilActivePool_to_soilPassivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilActivePool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilMicrobesPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilPassivePool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilSlowPool_to_soilPassivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilStructuralLitterPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilNitrogen << soil.nitrogenFlux_soilSlowPool_to_soilPassivePool;
        output.bufferSoilNitrogen << std::endl;
    }
}

/**
 * @brief Appends one row of soil water flux and snow variables to
 *        `output.bufferSoilWater`.
 *
 * Written columns (tab-separated): Date, DayCount,
 * addedWaterToSoilByIrrigation, interception, surfaceRunOff, soilRunOff,
 * solidSnowContent, liquidSnowContent, evaporation, soilTemperature.
 * Only executed when `parameter.soilWaterOutputFile` is `true`.
 *
 * @param utils       Utility object (reserved).
 * @param parameter   Provides `day` and `soilWaterOutputFile` flag.
 * @param soil        Read-only; provides hydrological flux variables.
 * @param interaction Read-only; provides `soilTemperature`.
 * @param output      `bufferSoilWater` is appended.
 * @param date        ISO-formatted date string.
 */
void STEP::fillSoilWaterBuffer(UTILS utils, PARAMETER parameter, SOIL soil, INTERACTION interaction, OUTPUT &output, std::string date)
{
    if (parameter.soilWaterOutputFile)
    {
        output.bufferSoilWater << date << "\t" << parameter.day << "\t";
        output.bufferSoilWater << soil.addedWaterToSoilByIrrigation << "\t";
        output.bufferSoilWater << soil.interception << "\t";
        output.bufferSoilWater << soil.surfaceRunOff << "\t";
        output.bufferSoilWater << soil.soilRunOff << "\t";
        output.bufferSoilWater << soil.solidSnowContent << "\t";
        output.bufferSoilWater << soil.liquidSnowContent << "\t";
        output.bufferSoilWater << soil.evaporation << "\t";
        output.bufferSoilWater << interaction.soilTemperature;

        output.bufferSoilWater << std::endl;
    }
}

/**
 * @brief Appends one row per soil layer of per-layer water and nitrogen
 *        resources to `output.bufferSoilResourcesPerSoilLayer`.
 *
 * Written columns (tab-separated): Date, DayCount, SoilLayerNumber,
 * SoilLayerWidth, soilWaterFluxDownwardsOutOfSoilLayer,
 * waterContent_soilWaterPoolPerSoilLayer,
 * nitrogenContent_soilMineralPoolPerSoilLayer.
 * Only executed when `parameter.soilResourcesPerSoilLayerOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day`, `numberOfSoilLayers`, `soilLayerWidth`,
 *                  and `soilResourcesPerSoilLayerOutputFile` flag.
 * @param soil      Read-only; provides per-layer water and N content vectors.
 * @param output    `bufferSoilResourcesPerSoilLayer` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillSoilResourcePerSoilLayerBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date)
{
    if (parameter.soilResourcesPerSoilLayerOutputFile)
    {
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            output.bufferSoilResourcesPerSoilLayer << date << "\t" << parameter.day << "\t";
            output.bufferSoilResourcesPerSoilLayer << soilLayer << "\t";
            output.bufferSoilResourcesPerSoilLayer << parameter.soilLayerWidth.at(soilLayer) << "\t";
            output.bufferSoilResourcesPerSoilLayer << soil.soilWaterFluxDownwardsOutOfSoilLayer.at(soilLayer) << "\t";
            output.bufferSoilResourcesPerSoilLayer << soil.waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) << "\t";
            output.bufferSoilResourcesPerSoilLayer << soil.nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer);
            output.bufferSoilResourcesPerSoilLayer << std::endl;
        }
    }
}

/**
 * @brief Appends one row of detailed soil decomposition diagnostics to
 *        `output.bufferSoilFluxesDetails`.
 *
 * Columns follow the header defined in
 * OUTPUT::writeHeaderInOutputFiles() for the soil fluxes details file.
 * Includes decisive C/N ratios, lignin contents, per-transfer C and N
 * respiratory losses, immobilisation and mineralisation fluxes, and the
 * decomposition factor.
 * Only executed when `parameter.soilFluxesDetailsOutputFile` is `true`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `day` and `soilFluxesDetailsOutputFile` flag.
 * @param soil      Read-only; provides all decomposition diagnostic variables.
 * @param output    `bufferSoilFluxesDetails` is appended.
 * @param date      ISO-formatted date string.
 */
void STEP::fillSoilFluxesDetailsBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date)
{
    if (parameter.soilFluxesDetailsOutputFile)
    {
        output.bufferSoilFluxesDetails << date << "\t" << parameter.day << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilMicrobesPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilPassivePool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilSlowPool_soilActivePool << "\t"
                                       << soil.decisiveCNRatio_soilSlowPool_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilActivePool_soilSlowPool << "\t"
                                       << soil.decisiveCNRatio_soilActivePool_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilMetabolicLitterPool_soilActivePool << "\t"
                                       << soil.decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilStructuralLitterPool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.decisiveCNRatio_soilStructuralLitterPool_soilSlowPool << "\t";

        output.bufferSoilFluxesDetails << soil.ligninContent_surfaceStructuralLitterPool << "\t";
        output.bufferSoilFluxesDetails << soil.ligninContent_soilStructuralLitterPool << "\t";

        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionCarbon_soilPassivePool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respirationCarbon_surface_litter << "\t";
        output.bufferSoilFluxesDetails << soil.respirationCarbon_soil_litter << "\t";
        output.bufferSoilFluxesDetails << soil.respirationCarbon_litter << "\t";
        output.bufferSoilFluxesDetails << soil.respirationCarbon_soilpools << "\t";

        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.respiration_decompositionNitrogen_soilPassivePool_soilActivePool << "\t";

        output.bufferSoilFluxesDetails << soil.immobilize_surfaceStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilStructuralLitterPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilMetabolicLitterPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilMicrobesPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilActivePool_to_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilActivePool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilSlowPool_to_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.immobilize_soilPassivePool_to_soilActivePool << "\t";

        output.bufferSoilFluxesDetails << soil.mineralize_surfaceStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool << "\t";

        output.bufferSoilFluxesDetails << soil.mineralize_soilStructuralLitterPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilStructuralLitterPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilMetabolicLitterPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilMicrobesPool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilActivePool_to_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilActivePool_to_soilSlowPool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilSlowPool_to_soilPassivePool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilSlowPool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.mineralize_soilPassivePool_to_soilActivePool << "\t";
        output.bufferSoilFluxesDetails << soil.decompositionFactor;

        output.bufferSoilFluxesDetails << std::endl;
    }
}

/**
 * @brief Computes the ISO date string for today and delegates to all eight
 *        `fill*Buffer()` helpers.
 *
 * Calls, in order: fillCommunityBuffer(), fillPFTBuffer(),
 * fillPlantCohortBuffer(), fillSoilCarbonBuffer(), fillSoilNitrogenBuffer(),
 * fillSoilWaterBuffer(), fillSoilResourcePerSoilLayerBuffer(), and
 * fillSoilFluxesDetailsBuffer().
 *
 * @param utils       Utility object for date calculations.
 * @param parameter   Provides `day` and `referenceJulianDayStart`.
 * @param community   Read-only; passed to community/PFT/cohort buffer functions.
 * @param interaction Read-only; provides `soilTemperature` for water buffer.
 * @param output      All `buffer*` streams are appended.
 * @param soil        Read-only; passed to all soil buffer functions.
 * @param date        ISO-formatted date string (`"YYYY-MM-DD"`).
 */
void STEP::fillSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, INTERACTION interaction, OUTPUT &output, SOIL soil, std::string date)
{
    fillCommunityBuffer(utils, parameter, community, output, date);
    fillPFTBuffer(utils, parameter, community, output, date);
    fillPlantCohortBuffer(utils, parameter, community, output, date);
    fillSoilCarbonBuffer(utils, parameter, soil, output, date);
    fillSoilNitrogenBuffer(utils, parameter, soil, output, date);
    fillSoilWaterBuffer(utils, parameter, soil, interaction, output, date);
    fillSoilResourcePerSoilLayerBuffer(utils, parameter, soil, output, date);
    fillSoilFluxesDetailsBuffer(utils, parameter, soil, output, date);
}

/**
 * @brief Converts the current simulation day to an ISO date string, then
 *        conditionally fills all output buffers and flushes them to disk.
 *
 * Two operating modes:
 * - **Date-list mode** (`output.outputWritingDatesFileOpened == true`):
 *   results are only buffered and flushed on days that appear in
 *   `output.outputWritingDates`.
 * - **Daily mode**: results are buffered and flushed every simulation day.
 *
 * In both cases, after filling the buffers,
 * OUTPUT::writeSimulationResultsToOutputFiles() is called to flush the
 * buffers to the open file streams.
 *
 * @param utils       Utility object; used for date conversion.
 * @param parameter   Provides `day`, `referenceJulianDayStart`, and output flags.
 * @param community   Read-only; community state variables for output.
 * @param interaction Read-only; provides `soilTemperature`.
 * @param output      Output object; `buffer*` streams and file streams updated.
 * @param soil        Read-only; soil state variables for output.
 */
void STEP::saveSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, INTERACTION interaction, OUTPUT &output, SOIL soil)
{
    int day = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "day");
    int month = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "month");
    int year = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "year");

    std::string sDay, sMonth, date;
    sDay = (day < 10) ? ("0" + std::to_string(day)) : std::to_string(day);
    sMonth = (month < 10) ? ("0" + std::to_string(month)) : std::to_string(month);
    date = std::to_string(year) + "-" + sMonth + "-" + sDay;

    if (output.outputWritingDatesFileOpened)
    { /* results only at outputWritinDates are stored in buffer */
        for (auto day : output.outputWritingDates)
        {
            if (parameter.day == day)
            {
                fillSimulationResultsToBuffer(utils, parameter, community, interaction, output, soil, date);
            }
        }
    }
    else /* daily results stored in buffer */
    {
        fillSimulationResultsToBuffer(utils, parameter, community, interaction, output, soil, date);
    }
}
