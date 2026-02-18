#include "init.h"

INIT::INIT() {};
INIT::~INIT() {};

/* main function to initialize state variables of the simulation start */
/* is done only once at the beginning of a simulation */
void INIT::initModelSimulation(UTILS utils, PARAMETER &parameter, COMMUNITY &community, RECRUITMENT &recruitment, SOIL &soil, INTERACTION &interaction, WEATHER weather)
{
    /* init time variables */
    initTimeVariables(parameter);

    /* init random number generator seed */
    initRandomNumberGeneratorSeed(parameter, community);

    /* init state variables of community (that will change through the process dynamics) */
    initVegetationStateVariables(community, parameter, recruitment, soil);

    /* init process-specific state variables that will also be reset at each time step again */
    resetVegetationProcessVariables(parameter, recruitment, community, interaction, soil);

    /* init soil resource state variables (that will change through the process dynamics) */
    initSoilResourceStateVariables(utils, soil, weather, parameter);

    /* init process- and flux-specific state variables that will be reset at each time step again */
    resetSoilResourceProcessAndFluxVariables(parameter, soil);
}

/* initialization of state variables of time */
void INIT::initTimeVariables(PARAMETER &parameter)
{
    parameter.day = 1; // start with the first day
}

/* initialization of random number generator seed */
void INIT::initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community)
{
    if (parameter.randomNumberGeneratorSeed == std::numeric_limits<int>::min())
    {
        std::random_device rd; // seed generator
        parameter.randomNumberGeneratorSeed = rd();
    }
    community.randomNumberIndex = parameter.randomNumberGeneratorSeed;
}

/* initialization of the community vector and grassland state variables */
void INIT::initVegetationStateVariables(COMMUNITY &community, PARAMETER parameter, RECRUITMENT &recruitment, SOIL &soil)
{
    // simulation-related variables
    community.allPlants.clear();
    community.totalNumberOfCohortsInCommunity = 0;

    /* Recruitment variables */
    recruitment.seedPool.clear();
    recruitment.seedGerminationTimeCounter.clear();
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        recruitment.seedPool.push_back(std::vector<int>());
        recruitment.seedPool[pft].clear();
        recruitment.seedGerminationTimeCounter.push_back(std::vector<int>());
        recruitment.seedGerminationTimeCounter[pft].clear();
    }

    /* Mortality variables */
    soil.carbonContent_surfaceGreenLitterPool = 0;
    soil.carbonContent_surfaceBrownLitterPool = 0;
    soil.carbonContent_soilRootLitterPool = 0;
    soil.carbonContent_soilSeedLitterPool = 0;
    soil.nitrogenContent_surfaceGreenLitterPool = 0;
    soil.nitrogenContent_surfaceBrownLitterPool = 0;
    soil.nitrogenContent_soilRootLitterPool = 0;
    soil.nitrogenContent_soilSeedLitterPool = 0;
}

void INIT::resetVegetationProcessVariables(PARAMETER parameter, RECRUITMENT &recruitment, COMMUNITY &community, INTERACTION &interaction, SOIL soil)
{
    /// Process-related variables
    // 1. Recruitment
    recruitment.incomingSeeds.clear();
    recruitment.outgoingSeeds.clear();
    recruitment.successfullGerminatedSeeds.clear();
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        recruitment.incomingSeeds.push_back(0);
        recruitment.outgoingSeeds.push_back(0);
        recruitment.successfullGerminatedSeeds.push_back(0);
    }

    // 2. Light availability, crowding (interaction)
    interaction.LAI.clear();
    interaction.LAIwithLightExtinction.clear();
    for (int layerindex = 0; layerindex <= maximumHeightLayer; layerindex++)
    {
        interaction.LAI.push_back(0.0);
        interaction.LAIwithLightExtinction.push_back(0.0);
    }
    community.maximumHeightOfAllPlants = 0;
    community.totalLeafAreaIndexOfPlantsInCommunity = 0;
    community.greenleafAreaIndexOfPlantsInCommunity = 0;
    community.coveredAreaOfAllPlants = 0.0;

    // 3. Carbon and nitrogen balances
    community.carbonRespirationOfAllPlants = 0.0;
    community.carbonNPPOfAllPlants = 0.0;
    community.carbonSeedlingIngrowthOfAllPlants = 0.0;
    community.ecosystemNitrogenBalance = 0.0;
    community.ecosystemCarbonBalance = 0.0;
    community.ecosystemCarbonRespiration = 0.0;

    // 4. Soil water demand and uptake
    community.totalSoilWaterDemand = 0.0;
    community.totalSoilWaterUptake = 0.0;
    community.totalSoilWaterDemandPerSoilLayer.clear();
    community.totalSoilWaterUptakePerSoilLayer.clear();
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        community.totalSoilWaterDemandPerSoilLayer.push_back(0);
        community.totalSoilWaterUptakePerSoilLayer.push_back(0);
    }

    // 5. Soil nitrogen demand and uptake
    /* reset total nitrogen demand and competing plants per soil layer */
    community.totalSoilNitrogenDemand = 0;
    community.totalSoilNitrogenUptake = 0;
    community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.clear();
    community.totalSoilNitrogenDemandPerSoilLayer.clear();
    community.totalSoilNitrogenUptakePerSoilLayer.clear();
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.push_back(0);
        community.totalSoilNitrogenDemandPerSoilLayer.push_back(0);
        community.totalSoilNitrogenUptakePerSoilLayer.push_back(0);
    }

    /// Output-related variables
    community.totalNumberOfPlantsInCommunity = 0;
    community.greenBiomassYield = 0.0;
    community.brownBiomassYield = 0.0;
    community.biomassYield = 0.0;

    community.numberOfPlantsPerPFT.clear();
    community.pftComposition.clear();
    community.coveredAreaOfPlantsPerPFT.clear();
    community.shootBiomassOfPlantsPerPFT.clear();
    community.brownShootBiomassOfPlantsPerPFT.clear();
    community.greenShootBiomassOfPlantsPerPFT.clear();
    community.clippedShootBiomassOfPlantsPerPFT.clear();
    community.rootBiomassOfPlantsPerPFT.clear();
    community.recruitmentBiomassOfPlantsPerPFT.clear();
    community.exudationBiomassOfPlantsPerPFT.clear();
    community.gppOfPlantsPerPFT.clear();
    community.nppOfPlantsPerPFT.clear();
    community.carbonRespirationOfPlantsPerPFT.clear();
    community.greenBiomassYieldPerPFT.clear();
    community.brownBiomassYieldPerPFT.clear();
    community.biomassYieldPerPFT.clear();

    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        community.pftComposition.push_back(0);
        community.numberOfPlantsPerPFT.push_back(0);

        community.coveredAreaOfPlantsPerPFT.push_back(0);
        community.shootBiomassOfPlantsPerPFT.push_back(0);
        community.brownShootBiomassOfPlantsPerPFT.push_back(0);
        community.greenShootBiomassOfPlantsPerPFT.push_back(0);
        community.clippedShootBiomassOfPlantsPerPFT.push_back(0);
        community.rootBiomassOfPlantsPerPFT.push_back(0);
        community.recruitmentBiomassOfPlantsPerPFT.push_back(0);
        community.exudationBiomassOfPlantsPerPFT.push_back(0);
        community.gppOfPlantsPerPFT.push_back(0);
        community.nppOfPlantsPerPFT.push_back(0);
        community.carbonRespirationOfPlantsPerPFT.push_back(0);

        community.greenBiomassYieldPerPFT.push_back(0);
        community.brownBiomassYieldPerPFT.push_back(0);
        community.biomassYieldPerPFT.push_back(0);
    }
}

/**
 * @cite: function adapted from Century 4.0 model
 */
void INIT::initSoilResourceStateVariables(UTILS utils, SOIL &soil, WEATHER weather, PARAMETER parameter)
{
    // litter pools are set to zero at the beginning
    soil.carbonContent_surfaceStructuralLitterPool = 0;
    soil.carbonContent_soilStructuralLitterPool = 0;
    soil.carbonContent_surfaceMetabolicLitterPool = 0;
    soil.carbonContent_soilMetabolicLitterPool = 0;
    soil.nitrogenContent_surfaceStructuralLitterPool = 0;
    soil.nitrogenContent_soilStructuralLitterPool = 0;
    soil.nitrogenContent_surfaceMetabolicLitterPool = 0;
    soil.nitrogenContent_soilMetabolicLitterPool = 0;

    soil.ligninContent_surfaceStructuralLitterPool = 0;
    soil.ligninContent_soilStructuralLitterPool = 0;

    // microbes are set to a fixed initial value
    soil.carbonContent_soilMicrobesPool = 10.0; // gm-2
    soil.nitrogenContent_soilMicrobesPool = soil.carbonContent_soilMicrobesPool / 16.0;

    // soil pools are dependent on average weather variables and soil attributes
    double initCarbonContentOfSoilPools = calculateInitialCarbonContentOfAllSoilPools(utils, weather, parameter, soil);
    soil.carbonContent_soilActivePool = initCarbonContentOfSoilPools * 0.02;
    soil.carbonContent_soilSlowPool = initCarbonContentOfSoilPools * 0.64;
    soil.carbonContent_soilPassivePool = initCarbonContentOfSoilPools * 0.34;
    soil.nitrogenContent_soilActivePool = soil.carbonContent_soilActivePool / 12.0;
    soil.nitrogenContent_soilSlowPool = soil.carbonContent_soilSlowPool / 17.0;
    soil.nitrogenContent_soilPassivePool = soil.carbonContent_soilPassivePool / 8.0;

    soil.nitrogenContent_soilMineralPoolPerSoilLayer.clear();
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        soil.nitrogenContent_soilMineralPoolPerSoilLayer.push_back(1.0); // 1 g/m² TODO: add parameter
    }

    /* soil water dynamics */
    soil.waterContent_soilWaterPoolPerSoilLayer.clear();
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers + 1; soilLayer++)
    {
        if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
        {
            if (soilLayer < parameter.numberOfSoilLayers)
            {
                soil.waterContent_soilWaterPoolPerSoilLayer.push_back(soil.fieldCapacity.at(soilLayer)); // initialize soil water content at field capacity
            }
            else if (soilLayer == parameter.numberOfSoilLayers)
            {
                // groundwater storage at layer 'soilLayer' = parameter.numberOfSoilLayers
                soil.waterContent_soilWaterPoolPerSoilLayer.push_back(0.0);
            }
        }
        else
        {
            // no internal water, when coupled to external soil model
            soil.waterContent_soilWaterPoolPerSoilLayer.push_back(NAN);
        }
    }

    soil.solidSnowContent = 0.0;  // snow accumulation
    soil.liquidSnowContent = 0.0; // snow liquid content

    soil.addedMineralNitrogenToSoilByFertilization = 0;
    soil.addedWaterToSoilByIrrigation = 0;
}

/**
 * @cite: Century4.0
 * soil pools are initialized based on average weather data
 */
double INIT::calculateInitialCarbonContentOfAllSoilPools(UTILS utils, WEATHER weather, PARAMETER parameter, SOIL soil)
{
    // TODO: or choose mean annual/average values for entire simulation period instead of first year only ?
    double annualPrecipitation = weather.calculateAnnualPrecipitationOfSpecificYear(utils, parameter, parameter.firstYear) / 10.0; // cm
    double averageAirTemperature = weather.calculateAverageAirTemperatureOfSpecificYear(utils, parameter, parameter.firstYear);    // °C

    if (averageAirTemperature > 23.0) // Germany: summer 20°C, winter 1-2°C
    {
        averageAirTemperature = 23.0;
    }
    if (annualPrecipitation > 120.0) // Germany: average values are around 72.0 mm
    {
        annualPrecipitation = 120.0;
    }

    double initCarbonContentOfSoilPools = ((-8.27E-01 * averageAirTemperature) + (2.24E-02 * averageAirTemperature * averageAirTemperature) + (annualPrecipitation * 1.27E-01) - (9.38E-04 * annualPrecipitation * annualPrecipitation) +
                                           (annualPrecipitation * soil.siltContent * 8.99E-02) + (annualPrecipitation * soil.clayContent * 6.00E-02) + 4.09) *
                                          1000.0; // g/m²

    return (initCarbonContentOfSoilPools);
}

void INIT::resetSoilResourceProcessAndFluxVariables(PARAMETER parameter, SOIL &soil)
{
    /* soil water fluxes */
    soil.interception = 0.0;  // interception [mm/day]
    soil.evaporation = 0.0;   // snow evaporation [mm/day]
    soil.surfaceRunOff = 0.0; // run-off above-ground [mm/day]
    soil.soilRunOff = 0.0;    // run-off below-ground [mm/day]
    soil.soilWaterFluxDownwardsOutOfSoilLayer.clear();
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        soil.soilWaterFluxDownwardsOutOfSoilLayer.push_back(0);
    }

    /* carbon fluxes */
    soil.carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;
    soil.carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool = 0;

    soil.carbonFlux_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.carbonFlux_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.carbonFlux_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.carbonFlux_soilMicrobesPool_to_soilSlowPool = 0;

    soil.carbonFlux_soilActivePool_to_soilPassiveAndSlowPool = 0;
    soil.carbonFlux_soilActivePool_to_soilPassivePool = 0;
    soil.carbonFlux_soilActivePool_to_soilSlowPool = 0;

    soil.carbonFlux_soilSlowPool_to_soilPassiveAndActivePool = 0;
    soil.carbonFlux_soilSlowPool_to_soilPassivePool = 0;
    soil.carbonFlux_soilSlowPool_to_soilActivePool = 0;

    soil.carbonFlux_soilPassivePool_to_soilActivePool = 0;

    soil.carbonContent_leachedFromSoil = 0; // overall amount of carbon leached from active soil pool during simulation
    soil.leachingCarbon = 0;                // daily amount of carbon leached from active soil pool

    /* carbon respiratory fluxes */
    soil.respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool = 0;
    soil.respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool = 0;
    soil.respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool = 0;

    soil.respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool = 0;
    soil.respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool = 0;
    soil.respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool = 0;

    soil.respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool = 0;
    soil.respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool = 0;
    soil.respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool = 0;
    soil.respiration_decompositionCarbon_soilPassivePool_soilActivePool = 0;

    soil.respirationCarbon_surface_litter = 0;
    soil.respirationCarbon_soil_litter = 0;
    soil.respirationCarbon_litter = 0;
    soil.respirationCarbon_soilpools = 0;

    /* nitrogen fluxes */
    soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;

    soil.nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlow_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.nitrogenFlow_soilMicrobesPool_to_soilSlowPool = 0;

    soil.nitrogenFlow_soilActivePool_to_soilSlowPool = 0;
    soil.nitrogenFlow_soilActivePool_to_soilPassivePool = 0;

    soil.nitrogenFlow_soilSlowPool_to_soilActivePool = 0;
    soil.nitrogenFlow_soilSlowPool_to_soilPassivePool = 0;

    soil.nitrogenFlow_soilPassivePool_to_soilActivePool = 0;

    soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlow_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.nitrogenFlow_soilMicrobesPool_to_soilSlowPool = 0;

    soil.nitrogenFlow_soilActivePool_to_soilPassivePool = 0;
    soil.nitrogenFlow_soilActivePool_to_soilSlowPool = 0;

    soil.nitrogenFlow_soilSlowPool_to_soilPassivePool = 0;
    soil.nitrogenFlow_soilSlowPool_to_soilActivePool = 0;

    soil.nitrogenFlow_soilPassivePool_to_soilActivePool = 0;

    soil.nitrogenContent_leachedFromSoil = 0; // overall amount of nitrogen leached from active soil pool during simulation
    soil.leachingNitrogen = 0;                // daily amount of nitrogen leached from active soil pool

    /* nitrogen respiratory fluxes */
    soil.respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool = 0;
    soil.respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool = 0;
    soil.respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool = 0;

    soil.respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool = 0;
    soil.respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool = 0;
    soil.respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool = 0;

    soil.respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool = 0;
    soil.respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool = 0;
    soil.respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool = 0;
    soil.respiration_decompositionNitrogen_soilPassivePool_soilActivePool = 0;

    /* nitrogen mineralization, immobilization and volatilization */
    soil.nitrogenGrossMineralization = 0;
    soil.nitrogenNetMineralization = 0;
    soil.nitrogenVolatilization = 0;

    soil.mineralize_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;

    soil.mineralize_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.mineralize_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.mineralize_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.mineralize_soilMicrobesPool_to_soilSlowPool = 0;
    soil.mineralize_soilActivePool_to_soilPassivePool = 0;
    soil.mineralize_soilActivePool_to_soilSlowPool = 0;
    soil.mineralize_soilSlowPool_to_soilPassivePool = 0;
    soil.mineralize_soilSlowPool_to_soilActivePool = 0;
    soil.mineralize_soilPassivePool_to_soilActivePool = 0;

    soil.immobilize_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;

    soil.immobilize_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.immobilize_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.immobilize_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.immobilize_soilMicrobesPool_to_soilSlowPool = 0;
    soil.immobilize_soilActivePool_to_soilPassivePool = 0;
    soil.immobilize_soilActivePool_to_soilSlowPool = 0;
    soil.immobilize_soilSlowPool_to_soilPassivePool = 0;
    soil.immobilize_soilSlowPool_to_soilActivePool = 0;
    soil.immobilize_soilPassivePool_to_soilActivePool = 0;

    soil.decompositionFactor = 1.0;
}
