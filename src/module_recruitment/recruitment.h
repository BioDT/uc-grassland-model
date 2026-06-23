/**
 * @file recruitment.h
 * @brief Declares the RECRUITMENT class, which manages all plant recruitment
 *        processes in the grassland model.
 *
 * Recruitment covers the full seed-to-seedling pipeline:
 * 1. Seed influx from three sources: mature-plant reproduction, scheduled
 *    sowing (management file), and an optional external regional seed rain.
 * 2. Per-PFT seed pool management with per-batch germination-time tracking.
 * 3. Seed germination with stochastic or deterministic rounding.
 * 4. Seedling crowding mortality before cohort creation.
 * 5. Transfer of non-germinating seeds to the soil litter pool.
 * 6. Creation of new PLANT cohorts and their addition to COMMUNITY::allPlants.
 *
 * All per-PFT vectors (incomingSeeds, outgoingSeeds, seedPool,
 * seedGerminationTimeCounter, successfullGerminatedSeeds) are sized to
 * `parameter.pftCount` at model initialisation and reset each time step
 * by INIT::resetVegetationProcessVariables().
 */
#pragma once
#include "../module_parameter/parameter.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_plant/community.h"
#include "../module_soil/soil.h"
#include "../utils/utils.h"
#include <random>

/**
 * @class RECRUITMENT
 * @brief Orchestrates the full seed-to-seedling pipeline for all PFTs each
 *        simulation time step.
 *
 * The main entry point is doPlantRecruitment(), which calls the sub-functions
 * in the correct order. Internal state (seed pool, germination counters) persists
 * across time steps; all other fields are reset at the start of each step.
 */
class RECRUITMENT
{
public:
    RECRUITMENT();
    ~RECRUITMENT();

    /**
     * @brief Number of seeds arriving from all sources today, per PFT.
     */
    std::vector<int> incomingSeeds;

    /**
     * @brief Number of seeds produced by mature plants but dispersed away from
     *        the plot (not added to the local seed pool), per PFT.
     */
    std::vector<int> outgoingSeeds;

    /**
     * @brief Per-PFT seed pool; each inner vector holds one batch of seeds
     *        waiting to germinate.
     */
    std::vector<std::vector<int>> seedPool;

    /**
     * @brief Per-PFT germination countdown counters, parallel to `seedPool`.
     */
    std::vector<std::vector<int>> seedGerminationTimeCounter;

    /**
     * @brief Number of seeds that successfully germinated from the current
     *        batch, per PFT.
     */
    std::vector<double> successfullGerminatedSeeds;

    /**
     * @brief Orchestrates all plant recruitment processes for one time step.
     */
    void doPlantRecruitment(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management, SOIL &soil);

    /**
     * @brief Adds seeds from an external regional seed source to `incomingSeeds`.
     */
    void getIncomingSeedsByExternalInflux(PARAMETER parameter);

    /**
     * @brief Adds seeds from scheduled sowing events to `incomingSeeds`.
     */
    void getIncomingSeedsBySowing(PARAMETER parameter, MANAGEMENT management);

    /**
     * @brief Converts mature-plant recruitment biomass to seed counts and adds
     *        them to `incomingSeeds` (or `outgoingSeeds` if influx is off).
     */
    void getIncomingSeedsByPlantReproduction(PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Moves today's incoming seed counts into the per-PFT seed pool.
     */
    void saveIncomingSeedsInSeedPool(PARAMETER parameter);

    /**
     * @brief Decrements germination counters and processes batches that are
     *        ready to germinate.
     */
    void calculateSeedGerminationToSeedlings(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, SOIL &soil);

    /**
     * @brief Computes the number of successfully germinating seeds for one batch.
     */
    void calculateNumberOfGerminatingSeeds(UTILS utils, PARAMETER parameter, COMMUNITY &community, int pft, int seedCohortIndex);

    /**
     * @brief Transfers biomass C/N of non-germinating seeds to the soil litter pool.
     */
    void transferFailedToGerminateSeedsToLitterPool(UTILS utils, PARAMETER parameter, SOIL &soil, int pft, int seedCohortIndex);

    /**
     * @brief Removes a processed seed-cohort batch from `seedPool` and
     *        `seedGerminationTimeCounter`.
     */
    void updateSeedPool(int pft, int seedCohortIndex);

    /**
     * @brief Creates a new PLANT cohort from successfully germinated seedlings
     *        and appends it to `community.allPlants`.
     */
    void addGerminatedSeedlingsToCommunity(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, SOIL soil, int pft);

    /**
     * @brief Reduces `successfullGerminatedSeeds` counts when the plot would
     *        become over-crowded by the new seedlings.
     */
    void seedlingCrowdingMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry);
};