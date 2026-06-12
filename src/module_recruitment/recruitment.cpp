#include "recruitment.h"

RECRUITMENT::RECRUITMENT() {};
RECRUITMENT::~RECRUITMENT() {};

/**
 * @brief Orchestrates all plant recruitment processes for one simulation time step.
 *
 * Executes the following steps in order:
 * 1. getIncomingSeedsByPlantReproduction() — converts mature-plant recruitment
 *    biomass to seed counts.
 * 2. getIncomingSeedsBySowing() — adds seeds from scheduled sowing events.
 * 3. getIncomingSeedsByExternalInflux() — adds seeds from an external source
 *    (e.g. regional seed rain).
 * 4. saveIncomingSeedsInSeedPool() — moves the day's seed totals into the
 *    per-PFT seed pool with associated germination-time counters.
 * 5. calculateSeedGerminationToSeedlings() — decrements germination counters,
 *    germinates ready batches, applies seedling crowding mortality, creates
 *    new cohorts, and transfers failed seeds to litter.
 * 6. Updates `community.totalNumberOfCohortsInCommunity` after any new cohorts
 *    have been added.
 *
 * @param utils      Utility object for error handling.
 * @param parameter  Model parameters; provides PFT counts, seed properties, flags.
 * @param allometry  Allometric helper for initialising new seedling geometry.
 * @param community  Plant community; new cohorts appended to `allPlants`,
 *                   cohort counter updated.
 * @param management Management state; provides sowing dates and amounts.
 * @param soil       Soil state; litter pools updated with failed-germination seeds.
 */
void RECRUITMENT::doPlantRecruitment(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management, SOIL &soil)
{
    // 1. seed influx by different seed sources
    getIncomingSeedsByPlantReproduction(parameter, community);
    getIncomingSeedsBySowing(parameter, management);
    getIncomingSeedsByExternalInflux(parameter);

    // 2. storage of seed influx to local seed pool
    saveIncomingSeedsInSeedPool(parameter);

    // 3. seed germination from seed pool accounting for germination times and rates
    calculateSeedGerminationToSeedlings(utils, parameter, allometry, community, soil);

    // 4. Update number of cohorts in allPlants-vector
    community.totalNumberOfCohortsInCommunity = community.allPlants.size();
}

/**
 * @brief Adds seeds from an external regional seed source to `incomingSeeds`.
 *
 * Activated only when `parameter.externalSeedInfluxActivated` is `true` and
 * the current simulation day is at or after `parameter.dayOfExternalSeedInfluxStart`.
 * Adds `parameter.externalSeedInfluxNumber[pft]` seeds to `incomingSeeds[pft]`
 * for every PFT.
 *
 * @param parameter Read-only; provides the activation flag, start day, per-PFT
 *                  influx numbers, and `pftCount`.
 */
void RECRUITMENT::getIncomingSeedsByExternalInflux(PARAMETER parameter)
{
    if (parameter.externalSeedInfluxActivated && parameter.day >= parameter.dayOfExternalSeedInfluxStart)
    {
        for (int pft = 0; pft < parameter.pftCount; pft++)
        {
            incomingSeeds[pft] += parameter.externalSeedInfluxNumber[pft];
        }
    }
}

/**
 * @brief Adds seeds from scheduled sowing events to `incomingSeeds`.
 *
 * Scans `management.sowingDate` for entries matching the current simulation
 * day. On a match, adds `management.amountOfSownSeeds[pft][sowingDayIndex]`
 * to `incomingSeeds[pft]` for every PFT. Does nothing if no sowing dates are
 * scheduled.
 *
 * @param parameter  Read-only; provides `day` and `pftCount`.
 * @param management Read-only; provides `sowingDate` and `amountOfSownSeeds`.
 */
void RECRUITMENT::getIncomingSeedsBySowing(PARAMETER parameter, MANAGEMENT management)
{
    if (management.sowingDate.size() > 0)
    {
        /* for each sowing day from the management file */
        for (int sowingDayIndex = 0; sowingDayIndex < management.sowingDate.size(); sowingDayIndex++)
        {
            /* if the current day is exactly a sowing day */
            if (parameter.day == management.sowingDate[sowingDayIndex])
            {
                for (int pft = 0; pft < parameter.pftCount; pft++)
                {
                    incomingSeeds[pft] += management.amountOfSownSeeds[pft][sowingDayIndex];
                }
            }
        }
    }
}

/**
 * @brief Converts mature-plant recruitment biomass to seed counts and adds
 *        them to `incomingSeeds`.
 *
 * Iterates over all cohorts. For each cohort that has reached maturity
 * (`height >= maturityHeights[pft]`) and has a positive `recruitmentBiomass`,
 * computes the number of seeds as:
 * @f[
 *   N_{\text{seeds}} = \left\lfloor
 *     \frac{\text{amount} \times B_{\text{recruitment}}}{m_{\text{seed}}}
 *     + 0.5 \right\rfloor
 * @f]
 * If `parameter.seedsFromMaturePlantsActivated` is `true`, adds the count to
 * `incomingSeeds[pft]`; otherwise records them in `outgoingSeeds[pft]`
 * (dispersed away from the plot). In both cases, `recruitmentBiomass`,
 * `recruitmentCarbon`, and `recruitmentNitrogen` are reset to zero.
 *
 * @param parameter Read-only; provides `seedsFromMaturePlantsActivated`,
 *                  `maturityHeights`, `seedMasses`, and `pftCount`.
 * @param community Plant community; `recruitmentBiomass` and derived C/N fields
 *                  reset to zero for each producing cohort.
 */
void RECRUITMENT::getIncomingSeedsByPlantReproduction(PARAMETER parameter, COMMUNITY &community)
{
    int pft, numberOfSeeds;

    for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
    {
        /* new plant cohorts are stored at the end of the community vector */
        pft = community.allPlants[cohortIndex]->pft;

        /* if plants have reached maturity, their recruitment biomass pool is used for seed production (based on PFT-specific seed mass) */
        if (community.allPlants[cohortIndex]->height >= parameter.maturityHeights[pft])
        {
            if (community.allPlants[cohortIndex]->recruitmentBiomass > 0)
            {
                numberOfSeeds = (int)floor((community.allPlants[cohortIndex]->amount * community.allPlants[cohortIndex]->recruitmentBiomass / parameter.seedMasses[pft]) + 0.5);
                if (parameter.seedsFromMaturePlantsActivated)
                {
                    incomingSeeds[pft] += numberOfSeeds;
                    outgoingSeeds[pft] += 0;
                }
                else
                {
                    incomingSeeds[pft] += 0;
                    outgoingSeeds[pft] += numberOfSeeds;
                }
                community.allPlants[cohortIndex]->recruitmentBiomass = 0;
                community.allPlants[cohortIndex]->recruitmentCarbon = 0;
                community.allPlants[cohortIndex]->recruitmentNitrogen = 0;
            }
        }
    }
}

/**
 * @brief Moves the day's incoming seed counts into the per-PFT seed pool.
 *
 * For each PFT with at least one incoming seed, appends the seed count to
 * `seedPool[pft]` and appends the corresponding germination wait time
 * (`parameter.seedGerminationTimes[pft]`) to `seedGerminationTimeCounter[pft]`.
 * Each entry in these parallel vectors represents one "seed cohort batch"
 * that will germinate after the required number of days.
 *
 * @param parameter Read-only; provides `pftCount` and `seedGerminationTimes`.
 */
void RECRUITMENT::saveIncomingSeedsInSeedPool(PARAMETER parameter)
{
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        if (incomingSeeds[pft] > 0)
        {
            seedPool[pft].push_back(incomingSeeds[pft]);
            seedGerminationTimeCounter[pft].push_back(parameter.seedGerminationTimes[pft]);
        }
    }
}

/**
 * @brief Processes the germination of all seed cohort batches that are ready
 *        to germinate and adds the resulting seedlings to the community.
 *
 * For each PFT and each stored seed-cohort batch, decrements the germination
 * counter by 1. When the counter reaches zero the batch is ready:
 * 1. calculateNumberOfGerminatingSeeds() — draws the number of successfully
 *    germinated seeds from the batch (stochastic rounding if enabled).
 * 2. seedlingCrowdingMortality() — reduces seedling counts if the plot is
 *    already full (only when crowding mortality is active).
 * 3. addGerminatedSeedlingsToCommunity() — creates a new PLANT cohort and
 *    appends it to `community.allPlants`.
 * 4. transferFailedToGerminateSeedsToLitterPool() — sends the non-germinating
 *    seeds' C/N to the soil seed litter pool.
 * 5. updateSeedPool() — removes the processed batch from the pool vectors.
 *
 * @param utils      Utility object for error handling and random numbers.
 * @param parameter  Model parameters; provides PFT counts, germination rates,
 *                   crowding flag, and stochastic-mode flag.
 * @param allometry  Allometric helper for initialising seedling geometry.
 * @param community  Plant community; new cohorts appended to `allPlants`.
 * @param soil       Soil state; failed-germination seed C/N transferred to litter.
 */
void RECRUITMENT::calculateSeedGerminationToSeedlings(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, SOIL &soil)
{
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        for (int seedCohortIndex = 0; seedCohortIndex < seedGerminationTimeCounter[pft].size(); seedCohortIndex++)
        {
            seedGerminationTimeCounter[pft].at(seedCohortIndex) -= 1;     // account for this day for germination by decreasing counter by one
            if (seedGerminationTimeCounter[pft].at(seedCohortIndex) == 0) /* only if counter is 0 seeds are now ready to germinate as seedlings */
            {
                // calculate number of successful germinated seeds from seedpool
                calculateNumberOfGerminatingSeeds(utils, parameter, community, pft, seedCohortIndex);
                // check if there is enough space left for all seedlings to establish

                if (parameter.crowdingMortalityActivated)
                {
                    seedlingCrowdingMortality(utils, parameter, community, allometry);
                }
                addGerminatedSeedlingsToCommunity(utils, parameter, community, allometry, soil, pft);

                // calculate number of failed germinated seeds from seedpool and transfer to litter pool
                transferFailedToGerminateSeedsToLitterPool(utils, parameter, soil, pft, seedCohortIndex);

                // update seed pool after germination
                updateSeedPool(pft, seedCohortIndex);
            }
        }
    }
}

/**
 * @brief Computes the number of successfully germinated seeds from a batch.
 *
 * Multiplies `seedPool[pft][seedCohortIndex]` by `parameter.seedGerminationRates[pft]`
 * to obtain a potentially fractional count. In stochastic mode the fractional
 * part is resolved probabilistically via a uniform random draw:
 * - If `rand ≤ fractional part` → ceiling is used.
 * - Otherwise → floor is used.
 * In deterministic mode the raw (possibly fractional) value is stored directly.
 *
 * The result is written to `successfullGerminatedSeeds[pft]`. Raises an error
 * if the result is negative.
 *
 * @param utils          Utility object for error handling.
 * @param parameter      Read-only; provides `seedGerminationRates`, `pftCount`,
 *                       and `stochasticSimulation` flag.
 * @param community      Plant community; `randomNumberIndex` incremented for the
 *                       stochastic draw.
 * @param pft            PFT index of the batch being processed.
 * @param seedCohortIndex Index of the seed-cohort batch in `seedPool[pft]`.
 */
void RECRUITMENT::calculateNumberOfGerminatingSeeds(UTILS utils, PARAMETER parameter, COMMUNITY &community, int pft, int seedCohortIndex)
{
    int integerPartOfCalculatedNumberOfSeeds;
    double calculatedNumberOfSeeds;
    calculatedNumberOfSeeds = seedPool[pft].at(seedCohortIndex) * parameter.seedGerminationRates[pft];
    integerPartOfCalculatedNumberOfSeeds = std::floor(calculatedNumberOfSeeds);

    // stochasticity in ceiling or flooring of the calculated number of germinating seeds if not integer
    if (parameter.stochasticSimulation && ((calculatedNumberOfSeeds - integerPartOfCalculatedNumberOfSeeds) > 0))
    {
        double randomNumber = -1;
        community.randomNumberIndex++;

        std::uniform_real_distribution<> dis(0.0, 1.0);
        std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
        randomNumber = dis(gen);
        if (randomNumber <= (calculatedNumberOfSeeds - integerPartOfCalculatedNumberOfSeeds))
        {
            successfullGerminatedSeeds.at(pft) = integerPartOfCalculatedNumberOfSeeds + 1;
        }
        else
        {
            successfullGerminatedSeeds.at(pft) = integerPartOfCalculatedNumberOfSeeds;
        }
    }
    else
    {
        successfullGerminatedSeeds.at(pft) = calculatedNumberOfSeeds;
    }

    if (successfullGerminatedSeeds.at(pft) < 0)
    {
        utils.handleError("Calculation of germinating seeds return a negative value!");
    }
}

/**
 * @brief Reduces seedling counts if the combined covered area of the existing
 *        community plus newly germinated seedlings exceeds SIMULATION_AREA.
 *
 * Computes the canopy area required by all germinating seedlings
 * (summed over PFTs, weighted by overlap factors), adds it to the current
 * community covered area, and checks whether the total exceeds SIMULATION_AREA.
 * If so, scales all per-PFT seedling counts down by:
 * @f[
 *   f = \frac{\text{available space}}{\text{required seedling space}}
 * @f]
 * (floored to the nearest integer per PFT).
 *
 * Called by calculateSeedGerminationToSeedlings() only when
 * `parameter.crowdingMortalityActivated` is `true`.
 *
 * @param utils     Utility object for error handling in allometric calls.
 * @param parameter Read-only; provides `pftCount`, overlap factors, allometric
 *                  parameters, and seed masses.
 * @param community Read-only; provides `allPlants` for the current covered-area
 *                  sum (the community accumulator is not yet updated at this
 *                  point in the time step).
 * @param allometry Allometric helper for computing seedling covered area.
 */
void RECRUITMENT::seedlingCrowdingMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry)
{
    double requiredSpaceForNewSeedlings = 0.0;
    double newCoveredAreaOfAllPlants = 0.0;
    double availableSpaceForNewSeedlings = 0.0;
    double reductionOfNewSeedlingsByCrowding = 1.0;

    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        double seedlingHeight = allometry.heightFromPlantBiomassShootCorrectionAndByRatios(utils, parameter.seedMasses[pft], parameter.plantHeightToWidthRatio[pft], parameter.plantShootCorrectionFactor[pft], parameter.plantShootRootRatio[pft]);
        double seedlingWidth = allometry.widthFromHeightByRatio(utils, seedlingHeight, parameter.plantHeightToWidthRatio[pft]);
        double seedlingCoveredArea = allometry.areaFromWidth(seedlingWidth);

        requiredSpaceForNewSeedlings += (successfullGerminatedSeeds.at(pft) * parameter.plantShootOverlapFactors[pft] * seedlingCoveredArea);
    }
    // calculate covered area of all plants (community variable is not calculated in this timestep yet)
    double coveredAreaOfAllPlants = 0.0;
    for (int cohortIndex = 0; cohortIndex < community.allPlants.size(); cohortIndex++)
    {
        int pft = community.allPlants[cohortIndex]->pft;
        coveredAreaOfAllPlants += community.allPlants[cohortIndex]->coveredArea * parameter.plantShootOverlapFactors[pft] * community.allPlants[cohortIndex]->amount;
    }
    newCoveredAreaOfAllPlants = coveredAreaOfAllPlants + requiredSpaceForNewSeedlings;

    if (newCoveredAreaOfAllPlants > SIMULATION_AREA)
    {
        availableSpaceForNewSeedlings = std::max(SIMULATION_AREA - community.coveredAreaOfAllPlants, 0.0);
        reductionOfNewSeedlingsByCrowding = availableSpaceForNewSeedlings / requiredSpaceForNewSeedlings;

        for (int pft = 0; pft < parameter.pftCount; pft++)
        {
            successfullGerminatedSeeds.at(pft) = std::floor(successfullGerminatedSeeds.at(pft) * reductionOfNewSeedlingsByCrowding);
        }
    }
}

/**
 * @brief Transfers the biomass C/N of seeds that failed to germinate to the
 *        soil seed litter pool.
 *
 * Computes the number of failed seeds as:
 * @f[ N_{\text{failed}} = \text{seedPool}[pft][seedCohortIndex]
 *     - \text{successfullGerminatedSeeds}[pft] @f]
 * and delegates to `SOIL::transferDyingPlantPartsToLitterPools()` using
 * `parameter.seedMasses[pft]` as the per-seed biomass.
 *
 * Issues a warning via `utils.handleWarning()` if the sum of germinated and
 * failed seeds deviates from the cohort total by more than TOLERANCE (numerical
 * rounding guard).
 *
 * @param utils           Utility object for warnings.
 * @param parameter       Read-only; provides `seedMasses` and `pftCount`.
 * @param soil            Soil state; seed litter C/N pool incremented.
 * @param pft             PFT index of the batch being processed.
 * @param seedCohortIndex Index of the seed-cohort batch in `seedPool[pft]`.
 */
void RECRUITMENT::transferFailedToGerminateSeedsToLitterPool(UTILS utils, PARAMETER parameter, SOIL &soil, int pft, int seedCohortIndex)
{
    /* calculate number of failed germinated seeds from seedpool */
    double failedToGerminateSeeds;
    failedToGerminateSeeds = seedPool[pft].at(seedCohortIndex) - successfullGerminatedSeeds.at(pft);

    /* check for consistency */
    if (std::abs((successfullGerminatedSeeds.at(pft) + failedToGerminateSeeds) - seedPool[pft].at(seedCohortIndex)) >= TOLERANCE)
    {
        utils.handleWarning("There is more numerical variation in the rounding of germinated / non-germinated seeds than expected.");
    }

    /* transfer carbon and nitrogen content of failed seeds to the respective litter pools for decomposition */
    soil.transferDyingPlantPartsToLitterPools(utils, parameter, failedToGerminateSeeds, parameter.seedMasses[pft], "soil_seed", pft);
}

/**
 * @brief Removes a processed seed-cohort batch from the pool vectors.
 *
 * Erases the entry at `seedCohortIndex` from both `seedPool[pft]` and
 * `seedGerminationTimeCounter[pft]`. Must be called after
 * transferFailedToGerminateSeedsToLitterPool() so that the batch data is
 * no longer needed.
 *
 * @param pft             PFT index whose vectors are updated.
 * @param seedCohortIndex Index of the batch to remove.
 */
void RECRUITMENT::updateSeedPool(int pft, int seedCohortIndex)
{
    // delete entry in both vectors (as germination process is now completed for those seeds from the seedpool)
    seedGerminationTimeCounter[pft].erase(seedGerminationTimeCounter[pft].begin() + seedCohortIndex);
    seedPool[pft].erase(seedPool[pft].begin() + seedCohortIndex);
}

/**
 * @brief Creates a new PLANT cohort from successfully germinated seedlings
 *        and appends it to the community.
 *
 * If `successfullGerminatedSeeds[pft] > 0`, constructs a new PLANT object
 * via `std::make_shared<PLANT>(utils, parameter, allometry, pft,
 * successfullGerminatedSeeds[pft])` and emplaces it at the end of
 * `community.allPlants`. The new cohort is initialised at seedling stage
 * with biomass equal to `parameter.seedMasses[pft]`.
 *
 * @param utils     Utility object for allometric error handling inside PLANT
 *                  constructor.
 * @param parameter Model parameters passed to the PLANT constructor.
 * @param community Plant community; new cohort appended to `allPlants`.
 * @param allometry Allometric helper passed to the PLANT constructor.
 * @param soil      Passed through (currently unused inside this function;
 *                  reserved for future direct soil initialisation).
 * @param pft       PFT index of the newly created cohort.
 */
void RECRUITMENT::addGerminatedSeedlingsToCommunity(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, SOIL soil, int pft)
{
    if (successfullGerminatedSeeds.at(pft) > 0)
    {
        community.allPlants.emplace_back(std::make_shared<PLANT>(utils, parameter, allometry, pft, successfullGerminatedSeeds.at(pft)));
    }
}
