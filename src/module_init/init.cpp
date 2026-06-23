#include "init.h"

INIT::INIT() {};
INIT::~INIT() {};

/**
 * @brief Initialises all model state variables at the start of a simulation.
 *
 * This is the single entry point for model initialisation and is called exactly
 * once before the main time-step loop begins. It delegates to specialised
 * sub-functions in the following order:
 * 1. initTimeVariables() — set the start day counter.
 * 2. initRandomNumberGeneratorSeed() — seed the RNG (from parameter or system).
 * 3. initVegetationStateVariables() — clear plant cohort lists, recruitment pools,
 *    and mortality litter pools.
 * 4. resetVegetationProcessVariables() — zero all per-step process/flux accumulators.
 * 5. initSoilResourceStateVariables() — initialise soil C/N pools and water content.
 * 6. resetSoilResourceProcessAndFluxVariables() — zero all soil flux variables.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameter struct; `day` and RNG seed are written here.
 * @param community   Plant community object; fully cleared and reset.
 * @param recruitment Recruitment state; seed pools and germination counters cleared.
 * @param soil        Soil state object; C/N pools, water content, and all flux variables initialised.
 * @param interaction Interaction state; LAI arrays reset to zero.
 * @param weather     Weather data used to compute initial soil pool sizes.
 */
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
    resetSoilResourceProcessAndFluxVariables(utils, parameter, soil);
}

/**
 * @brief Initialises the simulation time counter.
 *
 * Sets `parameter.day` to 1, representing the first day of the simulation.
 *
 * @param parameter Model parameter struct; `day` is set to 1.
 */
void INIT::initTimeVariables(PARAMETER &parameter)
{
    parameter.day = 1; // start with the first day
}

/**
 * @brief Initialises the random number generator seed.
 *
 * If `parameter.randomNumberGeneratorSeed` is set to `std::numeric_limits<int>::min()`
 * (the sentinel value indicating no user-specified seed), a non-deterministic seed is
 * drawn from `std::random_device` and stored back in `parameter.randomNumberGeneratorSeed`.
 * The seed is then assigned to `community.randomNumberIndex` so that all stochastic
 * processes in the simulation share the same reproducible sequence.
 *
 * @param parameter Model parameter struct; `randomNumberGeneratorSeed` may be updated.
 * @param community Plant community object; `randomNumberIndex` is set to the chosen seed.
 */
void INIT::initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community)
{
    if (parameter.randomNumberGeneratorSeed == std::numeric_limits<int>::min())
    {
        std::random_device rd; // seed generator
        parameter.randomNumberGeneratorSeed = rd();
    }
    community.randomNumberIndex = parameter.randomNumberGeneratorSeed;
}

/**
 * @brief Initialises vegetation and recruitment state variables to their start-of-simulation values.
 *
 * Clears the plant cohort list, resets the cohort count, and sets up empty per-PFT
 * seed pools and germination time counters in the RECRUITMENT object. Also zeros
 * all litter pool carbon and nitrogen quantities in the SOIL object that are populated
 * by the mortality module (surface green/brown litter, soil root litter, soil seed litter).
 *
 * @param community   Plant community; cohort list cleared, `totalNumberOfCohortsInCommunity` set to 0.
 * @param parameter   Read-only; provides `pftCount` for sizing per-PFT containers.
 * @param recruitment Recruitment state; `seedPool` and `seedGerminationTimeCounter` reset.
 * @param soil        Soil state; mortality-related litter C/N pools set to zero.
 */
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

    /* Mortality-related litter variables */
    soil.carbonContent_surfaceGreenLitterPool = 0;
    soil.carbonContent_surfaceBrownLitterPool = 0;
    soil.carbonContent_soilRootLitterPool = 0;
    soil.carbonContent_soilSeedLitterPool = 0;
    soil.nitrogenContent_surfaceGreenLitterPool = 0;
    soil.nitrogenContent_surfaceBrownLitterPool = 0;
    soil.nitrogenContent_soilRootLitterPool = 0;
    soil.nitrogenContent_soilSeedLitterPool = 0;
}

/**
 * @brief Resets all per-time-step vegetation process and output accumulator variables.
 *
 * Called during model initialisation (and at the start of each time step) to clear
 * variables that accumulate fluxes or summarise community state over a single day.
 * The following groups are reset:
 * - **Recruitment**: incoming/outgoing/germinated seed counts per PFT.
 * - **Interaction / light**: LAI and light-extinction arrays, community height, leaf area
 *   index accumulators, covered area, above-ground biomass, and litter biomass.
 * - **Carbon and nitrogen balances**: community-level respiration, NPP, seedling ingrowth,
 *   and ecosystem C/N balance accumulators.
 * - **Soil water demand/uptake**: total and per-layer demand and uptake.
 * - **Soil nitrogen demand/uptake**: total and per-layer demand, uptake, and competitor counts.
 * - **Output aggregates**: plant counts, yield variables, per-PFT biomass fractions,
 *   GPP/NPP/respiration summaries, and yield-per-PFT arrays.
 *
 * @param parameter   Read-only; provides `pftCount` and `numberOfSoilLayers` for sizing.
 * @param recruitment Recruitment state; per-PFT seed-flux counters zeroed.
 * @param community   Plant community; all accumulator and output fields reset.
 * @param interaction Interaction state; LAI and extinction arrays reset to zero.
 * @param soil        Soil state (passed by value; not modified — soil fluxes are reset
 *                    separately by resetSoilResourceProcessAndFluxVariables()).
 */
void INIT::resetVegetationProcessVariables(PARAMETER parameter, RECRUITMENT &recruitment, COMMUNITY &community, INTERACTION &interaction, SOIL soil)
{
    /// Process-related variables
    // 1. Recruitment
    recruitment.incomingSeeds.assign(parameter.pftCount, 0);
    recruitment.outgoingSeeds.assign(parameter.pftCount, 0);
    recruitment.successfullGerminatedSeeds.assign(parameter.pftCount, 0);

    // 2. Light availability, crowding (interaction)
    interaction.LAI.assign(MAXIMUM_HEIGHT_LAYER + 1, 0.0);
    interaction.LAIwithLightExtinction.assign(MAXIMUM_HEIGHT_LAYER + 1, 0.0);
    community.maximumHeightOfAllPlants = 0;
    community.totalLeafAreaIndexOfPlantsInCommunity = 0;
    community.greenleafAreaIndexOfPlantsInCommunity = 0;
    community.coveredAreaOfAllPlants = 0.0;
    community.coveredAreaOfAllPlantsPerHeightLayer.assign(MAXIMUM_HEIGHT_LAYER + 1, 0.0);
    community.abovegroundBiomassOfAllPlants = 0.0;
    community.abovegroundLitterBiomass = 0.0;

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
    community.totalSoilWaterDemandPerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
    community.totalSoilWaterUptakePerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);

    // 5. Soil nitrogen demand and uptake
    /* reset total nitrogen demand and competing plants per soil layer */
    community.totalSoilNitrogenDemand = 0;
    community.totalSoilNitrogenUptake = 0;
    community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.assign(parameter.numberOfSoilLayers, 0);
    community.totalSoilNitrogenDemandPerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
    community.totalSoilNitrogenUptakePerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);

    /// Output-related variables
    community.totalNumberOfPlantsInCommunity = 0;
    community.greenBiomassYield = 0.0;
    community.brownBiomassYield = 0.0;
    community.biomassYield = 0.0;

    community.pftComposition.assign(parameter.pftCount, 0);
    community.numberOfPlantsPerPFT.assign(parameter.pftCount, 0);

    community.coveredAreaOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.shootBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.brownShootBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.greenShootBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.clippedShootBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.rootBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.recruitmentBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.exudationBiomassOfPlantsPerPFT.assign(parameter.pftCount, 0.0);

    community.gppOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.nppOfPlantsPerPFT.assign(parameter.pftCount, 0.0);
    community.carbonRespirationOfPlantsPerPFT.assign(parameter.pftCount, 0.0);

    community.greenBiomassYieldPerPFT.assign(parameter.pftCount, 0.0);
    community.brownBiomassYieldPerPFT.assign(parameter.pftCount, 0.0);
    community.biomassYieldPerPFT.assign(parameter.pftCount, 0.0);
}

/**
 * @brief Initialises all soil resource state variables to their start-of-simulation values.
 *
 * Sets structural, metabolic, and microbial litter pools to zero; assigns a fixed
 * initial value to the soil microbial pool; and computes the initial sizes of the
 * soil active, slow, and passive C/N pools from mean annual weather conditions using
 * `calculateInitialCarbonContentOfAllSoilPools()`. Mineral nitrogen is set uniformly
 * across all soil layers, and water content is initialised at field capacity (internal
 * soil module) or NaN (external soil module). Snow stores and fertilisation/irrigation
 * inputs are zeroed. Finally, decisive C/N ratios for decomposition are computed for
 * all pool types.
 *
 * @param utils     Utility object for error handling.
 * @param soil      Soil state object; all pool sizes and auxiliary variables are set.
 * @param weather   Weather data used to derive mean annual precipitation and temperature
 *                  for the initial soil pool calculation.
 * @param parameter Read-only; provides soil module flags, layer count, and field capacity.
 *
 * @cite Soil dynamics have been adapted from the Century 4.0 soil organic matter model (https://www.soilcarbonsolutionscenter.com/century-agreement and https://www2.nrel.colostate.edu/projects/irc/#)
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

    /* decisive CN ratios */
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "surface_structural");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "soil_structural");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "surface_metabolic");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "soil_metabolic");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "microbes");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "active");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "slow");
    soil.calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, "passive");
}

/**
 * @brief Estimates the initial total carbon content of all soil pools from
 *        mean annual weather and soil texture.
 *
 * Uses an empirical regression equation derived from the Century 4.0 model to
 * compute a single aggregate carbon content value (g m⁻²) that is subsequently
 * split between the active (2 %), slow (64 %), and passive (34 %) soil pools.
 *
 * Input values are clamped to the valid regression range:
 * - Mean annual temperature capped at 23 °C.
 * - Annual precipitation capped at 120 cm.
 *
 * @param utils     Utility object for error handling.
 * @param weather   Weather data; annual precipitation and mean temperature are
 *                  derived for `parameter.firstYear`.
 * @param parameter Read-only; provides `firstYear` and the soil `siltContent`
 *                  and `clayContent` fractions (accessed via `soil`).
 * @param soil      Read-only; provides `siltContent` and `clayContent` fractions
 *                  used in the regression.
 * @return Estimated total initial soil carbon content (g C m⁻²).
 *
 * @cite Adapted from the Century 4.0 soil organic matter model (https://www.soilcarbonsolutionscenter.com/century-agreement and https://www2.nrel.colostate.edu/projects/irc/#)
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
                                          1000.0; // g per m²

    return (initCarbonContentOfSoilPools);
}

/**
 * @brief Resets all soil process and flux variables to zero at the start of each time step.
 *
 * Zeroes hydrological fluxes (interception, evaporation, run-off, per-layer downward
 * water fluxes), all inter-pool carbon and nitrogen transfer fluxes, carbon/nitrogen
 * respiratory losses from decomposition, gross and net nitrogen mineralisation,
 * volatilisation, nitrogen fixation, and the decomposition factor. Called once during
 * model initialisation and then again at the beginning of every daily time step.
 *
 * @param utils     Utility object (reserved for future error handling).
 * @param parameter Read-only; provides `numberOfSoilLayers` for vector sizing.
 * @param soil      Soil state object; all flux and process variables are set to zero
 *                  (or 1.0 for `decompositionFactor`).
 */
void INIT::resetSoilResourceProcessAndFluxVariables(UTILS utils, PARAMETER parameter, SOIL &soil)
{
    /* soil water fluxes */
    soil.interception = 0.0;  // interception [mm/day]
    soil.evaporation = 0.0;   // snow evaporation [mm/day]
    soil.surfaceRunOff = 0.0; // run-off above-ground [mm/day]
    soil.soilRunOff = 0.0;    // run-off below-ground [mm/day]
    soil.soilWaterFluxDownwardsOutOfSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);

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

    soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;

    soil.nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.nitrogenFlux_soilMicrobesPool_to_soilSlowPool = 0;

    soil.nitrogenFlux_soilActivePool_to_soilSlowPool = 0;
    soil.nitrogenFlux_soilActivePool_to_soilPassivePool = 0;

    soil.nitrogenFlux_soilSlowPool_to_soilActivePool = 0;
    soil.nitrogenFlux_soilSlowPool_to_soilPassivePool = 0;

    soil.nitrogenFlux_soilPassivePool_to_soilActivePool = 0;

    soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = 0;
    soil.nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = 0;
    soil.nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0;
    soil.nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = 0;

    soil.nitrogenFlux_soilMicrobesPool_to_soilSlowPool = 0;

    soil.nitrogenFlux_soilActivePool_to_soilPassivePool = 0;
    soil.nitrogenFlux_soilActivePool_to_soilSlowPool = 0;

    soil.nitrogenFlux_soilSlowPool_to_soilPassivePool = 0;
    soil.nitrogenFlux_soilSlowPool_to_soilActivePool = 0;

    soil.nitrogenFlux_soilPassivePool_to_soilActivePool = 0;

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
    soil.nitrogenFixationToSoil = 0;

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
