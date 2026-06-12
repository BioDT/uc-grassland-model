/**
 * @file mortality.h
 * @brief Declares the MORTALITY class, which handles all plant mortality processes
 *        in the grassland model.
 *
 * Mortality is split into four conceptual sub-processes applied each day:
 * 1. **Senescence and litter fall** — leaf browning, nitrogen retranslocation,
 *    brown-leaf litter transfer, and root turnover.
 * 2. **Crowding mortality** — density-dependent thinning when total covered area
 *    exceeds the simulation plot area.
 * 3. **Basic (background) mortality** — intrinsic daily death probability,
 *    differentiated by plant age (seedling vs. adult) and life span (annual vs. perennial).
 * 4. **Cohort cleanup** — removal of cohorts with no surviving individuals.
 *
 * Biomass of dying plant parts is always transferred to the appropriate soil litter
 * pool via `SOIL::transferDyingPlantPartsToLitterPools()` before cohort amounts
 * are decremented.
 */
#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_soil/soil.h"
#include "../module_growth/growth.h"
#include "../utils/utils.h"
#include <random>

/**
 * @class MORTALITY
 * @brief Applies all plant mortality sub-processes for a single simulation time step.
 *
 * The single public entry point is doPlantMortality(), which calls the specialised
 * sub-functions in the required order. Stochastic and deterministic execution paths
 * are supported for both crowding and basic mortality.
 *
 * @cite Crowding mortality concept derived from the forest model FORMIND
 *       (www.formind.org).
 */
class MORTALITY
{
public:
    MORTALITY();
    ~MORTALITY();

    /**
     * @brief Orchestrates all mortality sub-processes for the current time step.
     */
    void doPlantMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL &soil);

    /**
     * @brief Applies leaf browning, nitrogen relocation, brown-leaf litter fall,
     *        and root senescence for a single cohort.
     */
    void doSenescenceAndLitterFall(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL &soil, int cohortIndex, int pft);

    /**
     * @brief Computes the daily leaf browning rate and moves biomass from the green
     *        to the brown leaf pool.
     */
    double doLeafSenescence(COMMUNITY &community, PARAMETER parameter, GROWTH growth, INTERACTION interaction, int cohortIndex, int pft);

    /**
     * @brief Transfers a daily fraction of brown leaf biomass to the surface litter
     *        pool and updates plant geometry via updatePlantSize().
     */
    void doLeafLitterFall(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL &soil, int cohortIndex, int pft);

    /**
     * @brief Recomputes plant height, width, covered area, and LAI after litter fall.
     */
    void updatePlantSize(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int fractionLeavesFalling, int cohortIndex, int pft);

    /**
     * @brief Retranslocates nitrogen released during leaf browning to the plant's
     *        internal nitrogen surplus.
     */
    void doNitrogenRelocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, double browningLeafBiomass, int cohortIndex, int pft);

    /**
     * @brief Applies daily root turnover and transfers the dying root biomass to
     *        the soil root litter pool.
     */
    void doRootSenescenceAndLitterFall(UTILS utils, COMMUNITY &community, PARAMETER parameter, SOIL &soil, int cohortIndex, int pft);

    /**
     * @brief Accumulates the total covered area of all living cohorts, weighted by
     *        the PFT-specific shoot overlap factor.
     */
    void updateCoveredAreaOfAllPlants(PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Applies density-dependent crowding mortality to a single cohort when
     *        total covered area exceeds the simulation area.
     */
    void doPlantCrowding(PARAMETER parameter, UTILS utils, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft);

    /**
     * @brief Applies intrinsic (background) daily mortality to a single cohort.
     */
    void doBasicMortality(UTILS utils, PARAMETER parameter, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft);

    /**
     * @brief Returns the daily mortality probability for a given plant cohort.
     */
    double getPlantMortalityProbability(PARAMETER parameter, COMMUNITY community, int cohortIndex, int pft);
};