#include "step.h"

STEP::STEP() {};
STEP::~STEP() {};

/**
 * @brief Simulates the entire simulation period.
 *
 * This function runs the model simulation for a specified number of days as defined
 * by the `parameter.simulationTimeInDays`. For each day in the simulation, it calls `runModelSimulationStep` which resets specific state variables,
 * performs daily plant processes, updates the community's dynamic state variables, and saves the simulation results.
 *
 * @param utils Utility functions for string manipulation and error handling.
 * @param parameter Reference to a `PARAMETER` object containing simulation parameters,
 *                  including the current day of the simulation.
 * @param init An `INIT` object used for initializing and resetting process variables.
 * @param allometry An `ALLOMETRY` object that contains functions related to allometric
 *                  calculations for plant growth and structure.
 * @param community Reference to a `COMMUNITY` object representing the plant community
 *                  being simulated.
 * @param recruitment Reference to a `RECRUITMENT` object handling the plant recruitment processes
 *                    within the community.
 * @param mortality A `MORTALITY` object that handles plant mortality processes in the community.
 * @param growth A `GROWTH` object that calculates plant growth processes of the community.
 * @param management A `MANAGEMENT` object that performs predefined management regimes.
 * @param soil Reference to a `SOIL` object representing soil characteristics and processes.
 * @param output Reference to an `OUTPUT` object for saving simulation results.
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
 * @brief Simulates one simulation step.
 *
 * This function runs one step of the model simulation. It resets specific state variables,
 * performs daily plant processes, updates the community's dynamic state variables, and saves the simulation results.
 *
 * @param utils Utility functions for string manipulation and error handling.
 * @param parameter Reference to a `PARAMETER` object containing simulation parameters,
 *                  including the current day of the simulation.
 * @param init An `INIT` object used for initializing and resetting process variables.
 * @param allometry An `ALLOMETRY` object that contains functions related to allometric
 *                  calculations for plant growth and structure.
 * @param community Reference to a `COMMUNITY` object representing the plant community
 *                  being simulated.
 * @param recruitment Reference to a `RECRUITMENT` object handling the plant recruitment processes
 *                    within the community.
 * @param mortality A `MORTALITY` object that handles plant mortality processes in the community.
 * @param growth A `GROWTH` object that calculates plant growth processes of the community.
 * @param management A `MANAGEMENT` object that performs predefined management regimes.
 * @param soil Reference to a `SOIL` object representing soil characteristics and processes.
 * @param output Reference to an `OUTPUT` object for saving simulation results.
 */
void STEP::runModelSimulationStep(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output)
{
    /* Resetting of process- and flux-specific state variables of the vegetation community and soil resources */
    init.resetVegetationProcessVariables(parameter, recruitment, community, interaction, soil);
    init.resetSoilResourceProcessAndFluxVariables(parameter, soil);

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
 * @brief Performs one day step of all plant processes.
 *
 * This function executes the daily processes for plant dynamics, including recruitment,
 * mortality, and growth.
 *
 * @param utils Utility functions for various operations including error handling.
 * @param parameter Reference to a `PARAMETER` object containing simulation parameters.
 * @param allometry An `ALLOMETRY` object that handles allometric calculations related
 *                  to plant growth and structure.
 * @param community Reference to a `COMMUNITY` object representing the current state
 *                  of the plant community.
 * @param recruitment Reference to a `RECRUITMENT` object that manages the recruitment
 *                    of new plants into the community.
 * @param mortality A `MORTALITY` object that processes and calculates plant mortality and leaf senescence
 *                  within the community.
 * @param growth A `GROWTH` object responsible for calculating the growth of plants
 *               in the community.
 * @param management A `MANAGEMENT` object that applies predefined management regimes
 *                   to the community.
 * @param soil Reference to a `SOIL` object that represents the soil characteristics
 *              affecting plant processes.
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

void STEP::fillCommunityBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date)
{
    if (parameter.communityOutputFile)
    {
        output.bufferCommunity << date << "\t" << parameter.day << "\t";
        output.bufferCommunity << community.totalNumberOfPlantsInCommunity << "\t" << community.totalNumberOfCohortsInCommunity << "\t" << community.totalLeafAreaIndexOfPlantsInCommunity << "\t";
        output.bufferCommunity << community.maximumHeightOfAllPlants << "\t" << community.coveredAreaOfAllPlants << "\t" << community.ecosystemCarbonBalance << "\t" << community.ecosystemNitrogenBalance << std::endl;
    }
}

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

void STEP::fillSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, INTERACTION interaction, OUTPUT &output, SOIL soil, std::string date)
{
    fillCommunityBuffer(utils, parameter, community, output, date);
    fillPFTBuffer(utils, parameter, community, output, date);
    fillPlantCohortBuffer(utils, parameter, community, output, date);
    fillSoilCarbonBuffer(utils, parameter, soil, output, date);
    fillSoilNitrogenBuffer(utils, parameter, soil, output, date);
    fillSoilWaterBuffer(utils, parameter, soil, interaction, output, date);
    fillSoilResourcePerSoilLayerBuffer(utils, parameter, soil, output, date);
}

/**
 * @brief Saves the simulation results to a buffer for output.
 *
 * This function stores the simulation results of the current day into a buffer,
 * which can later be written to an output file. The results include information
 * about the plant functional types (PFTs) present in the community, including
 * their composition and the number of individual plants. The data is saved
 * conditionally based on whether specific output writing dates are configured.
 *
 * @param parameter A `PARAMETER` object containing simulation parameters such as
 *                  the current day and the number of plant functional types (PFTs).
 * @param community A `COMMUNITY` object that holds the current state of the plant
 *                  community, including its composition and number of plants per PFT.
 * @param output Reference to an `OUTPUT` object that manages the output buffer
 *               and handles writing the results to files.
 *
 * The function distinguishes between two cases:
 * - If the `outputWritingDatesFileOpened` is true, results are only saved for
 *   the days specified in `output.outputWritingDates`.
 * - Otherwise, results for every day of the simulation are stored directly in the
 *   buffer.
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
