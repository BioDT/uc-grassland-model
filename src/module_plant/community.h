/**
 * @file community.h
 * @brief Declares the COMMUNITY class, which holds all plant cohorts and
 *        community-level state variables for a grassland simulation plot.
 *
 * The COMMUNITY class is a central data hub. Its members are read and written
 * by nearly every module:
 * - **allPlants** — the cohort vector managed by recruitment and mortality.
 * - **Accumulator fields** — community-wide and per-PFT aggregates populated
 *   by interaction, growth, soil, management, recruitment, and output modules
 *   and reset each time step by INIT::resetVegetationProcessVariables().
 *
 * The only method is checkPlantsAreAliveInCommunity(), called by the mortality
 * module after all death processes to purge empty cohorts from `allPlants`.
 */
#pragma once
#include "plant.h"
#include "allometry.h"
#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @class COMMUNITY
 * @brief Stores the collection of plant cohorts and all community-level and
 *        PFT-level state variable accumulators for one simulation plot.
 *
 * All accumulator fields are reset to zero each time step by
 * INIT::resetVegetationProcessVariables() before the respective modules
 * recompute them. Per-PFT vectors are sized to `parameter.pftCount` at
 * initialisation.
 */
class COMMUNITY
{
public:
    COMMUNITY();
    ~COMMUNITY();

    /**
     * @brief Seed for the random number sequence shared by all stochastic
     *        processes within a time step.
     *
     * Initialised from `parameter.randomNumberGeneratorSeed` and incremented
     * by 1 before each stochastic draw (crowding and basic mortality) so that
     * every draw uses a different but reproducible sub-sequence.
     */
    int randomNumberIndex;

    /**
     * @brief Container of all living plant cohorts in the community.
     *
     * Each element is a `std::shared_ptr<PLANT>` representing one cohort. Cohorts
     * are added by the recruitment module and removed by
     * checkPlantsAreAliveInCommunity() when their `amount` drops to zero.
     */
    std::vector<std::shared_ptr<PLANT>> allPlants;

    // =========================================================================
    // Community / ecosystem state variables — process calculations and output
    // =========================================================================

    /**
     * @brief Number of cohorts currently in `allPlants` (= `allPlants.size()`).
     *
     * Used as the loop bound throughout the model. Updated by MORTALITY and
     * RECRUITMENT after cohorts are added or removed.
     */
    int totalNumberOfCohortsInCommunity;

    /**
     * @brief Total number of individual plants across all cohorts (sum of `amount`).
     */
    int totalNumberOfPlantsInCommunity;

    /**
     * @brief Height of the tallest plant in the community (cm).
     */
    double maximumHeightOfAllPlants;

    /**
     * @brief Community-level green leaf area index, normalised by SIMULATION_AREA
     *        (m² leaf m⁻² ground).
     */
    double greenleafAreaIndexOfPlantsInCommunity;

    /**
     * @brief Community-level total (green + brown) leaf area index, normalised by
     *        SIMULATION_AREA (m² leaf m⁻² ground).
     */
    double totalLeafAreaIndexOfPlantsInCommunity;

    /**
     * @brief Total aboveground (shoot) biomass of all cohorts weighted by their
     *        amount (g ODM).
     */
    double abovegroundBiomassOfAllPlants;

    /**
     * @brief Total aboveground litter biomass derived from surface litter C pools
     *        (g ODM).
     */
    double abovegroundLitterBiomass;

    /**
     * @brief Community-level covered area (cm²), summed over all cohorts weighted
     *        by `plantShootOverlapFactors` and `amount`.
     */
    double coveredAreaOfAllPlants;

    /**
     * @brief Per-height-layer covered area (cm²), used for layer-aware crowding
     *        mortality when `parameter.crowdingCalculationFromPlantTopLayer` is set.
     */
    std::vector<double> coveredAreaOfAllPlantsPerHeightLayer;

    /**
     * @brief Total carbon lost to plant maintenance + growth respiration across all
     *        cohorts (g C d⁻¹), weighted by `amount`.
     */
    double carbonRespirationOfAllPlants;

    /**
     * @brief Total plant net primary productivity across all cohorts (g C d⁻¹),
     *        weighted by `amount`.
     */
    double carbonNPPOfAllPlants;

    /**
     * @brief Total carbon entering the community via seedling ingrowth on the
     *        current day (g C d⁻¹).
     */
    double carbonSeedlingIngrowthOfAllPlants;

    /**
     * @brief Daily ecosystem nitrogen balance (g N m⁻² d⁻¹).
     */
    double ecosystemNitrogenBalance;

    /**
     * @brief Daily ecosystem carbon balance (g C m⁻² d⁻¹).
     */
    double ecosystemCarbonBalance;

    /**
     * @brief Total ecosystem carbon respiration (plant + litter + soil pools,
     *        g C m⁻² d⁻¹). Computed by OUTPUT.
     */
    double ecosystemCarbonRespiration;

    /**
     * @brief Community-level total soil water demand (mm d⁻¹).
     */
    double totalSoilWaterDemand;

    /**
     * @brief Per-soil-layer total water demand (mm d⁻¹ per layer).
     */
    std::vector<double> totalSoilWaterDemandPerSoilLayer;

    /**
     * @brief Community-level total soil water uptake (mm d⁻¹).
     */
    double totalSoilWaterUptake;

    /**
     * @brief Per-soil-layer total water uptake (mm d⁻¹ per layer).
     */
    std::vector<double> totalSoilWaterUptakePerSoilLayer;

    /**
     * @brief Community-level total mineral nitrogen demand (g N m⁻² d⁻¹).
     */
    double totalSoilNitrogenDemand;

    /**
     * @brief Per-soil-layer total nitrogen demand (g N m⁻² d⁻¹ per layer).
     */
    std::vector<double> totalSoilNitrogenDemandPerSoilLayer;

    /**
     * @brief Number of cohorts with roots in each soil layer, used to distribute
     *        available nitrogen equally among competitors.
     */
    std::vector<int> numberOfPlantsCompetingForSoilNitrogenPerSoilLayer;

    /**
     * @brief Community-level total mineral nitrogen uptake (g N m⁻² d⁻¹).
     */
    double totalSoilNitrogenUptake;

    /**
     * @brief Per-soil-layer total nitrogen uptake (g N m⁻² d⁻¹ per layer).
     */
    std::vector<double> totalSoilNitrogenUptakePerSoilLayer;

    /**
     * @brief Total green shoot biomass harvested on the current mowing day
     *        (g ODM; sum over all cohorts weighted by `amount`).
     */
    double greenBiomassYield;

    /**
     * @brief Total brown shoot biomass harvested on the current mowing day
     *        (g ODM; sum over all cohorts weighted by `amount`).
     */
    double brownBiomassYield;

    /**
     * @brief Total shoot biomass (green + brown) harvested on the current mowing
     *        day (g ODM; sum over all cohorts weighted by `amount`).
     */
    double biomassYield;

    // =========================================================================
    // Per-PFT state variables — output only
    // =========================================================================

    /**
     * @brief Percentage composition of each PFT (% of total plant count).
     */
    std::vector<double> pftComposition;

    /** @brief Total number of individual plants per PFT (weighted by `amount`).
     *         Sized to `parameter.pftCount`. */
    std::vector<double> numberOfPlantsPerPFT;

    /** @brief Total covered area per PFT (cm²), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> coveredAreaOfPlantsPerPFT;

    /** @brief Total shoot biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> shootBiomassOfPlantsPerPFT;

    /** @brief Total green shoot biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> greenShootBiomassOfPlantsPerPFT;

    /** @brief Total brown shoot biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> brownShootBiomassOfPlantsPerPFT;

    /**
     * @brief Total shoot biomass above the clipping height per PFT (g ODM),
     *        weighted by `amount`.
     */
    std::vector<double> clippedShootBiomassOfPlantsPerPFT;

    /** @brief Total root biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> rootBiomassOfPlantsPerPFT;

    /** @brief Total recruitment (seed) biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> recruitmentBiomassOfPlantsPerPFT;

    /** @brief Total root exudate biomass per PFT (g ODM), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> exudationBiomassOfPlantsPerPFT;

    /** @brief Total gross primary productivity per PFT (g ODM d⁻¹), weighted by
     *         `amount`. Sized to `parameter.pftCount`. */
    std::vector<double> gppOfPlantsPerPFT;

    /** @brief Total net primary productivity per PFT (g ODM d⁻¹), weighted by
     *         `amount`. Sized to `parameter.pftCount`. */
    std::vector<double> nppOfPlantsPerPFT;

    /** @brief Total carbon respiration per PFT (g C d⁻¹), weighted by `amount`.
     *         Sized to `parameter.pftCount`. */
    std::vector<double> carbonRespirationOfPlantsPerPFT;

    /** @brief Total green shoot biomass harvested per PFT on the current mowing
     *         day (g ODM). Sized to `parameter.pftCount`. */
    std::vector<double> greenBiomassYieldPerPFT;

    /** @brief Total brown shoot biomass harvested per PFT on the current mowing
     *         day (g ODM). Sized to `parameter.pftCount`. */
    std::vector<double> brownBiomassYieldPerPFT;

    /** @brief Total shoot biomass (green + brown) harvested per PFT on the current
     *         mowing day (g ODM). Sized to `parameter.pftCount`. */
    std::vector<double> biomassYieldPerPFT;

    /**
     * @brief Removes dead cohorts from `allPlants` after all mortality processes.
     */
    void checkPlantsAreAliveInCommunity(UTILS utils);
};
