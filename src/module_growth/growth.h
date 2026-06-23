/**
 * @file growth.h
 * @brief Declares the GROWTH class responsible for all plant growth processes.
 *
 * The GROWTH class implements a carbon-balance-based growth model for individual
 * plant cohorts. Starting from gross primary productivity (GPP) derived from a
 * light-response photosynthesis model, it applies respiration terms to obtain net
 * primary productivity (NPP), limits NPP by soil water and nitrogen availability,
 * and finally allocates the biomass increment to shoot, root, recruitment, and
 * exudate pools before updating allometric size variables.
 *
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"
#include "../module_soil/soil.h"
#include "../module_plant/allometry.h"
#include "../module_init/constants.h"
#include "../utils/utils.h"

/**
 * @class GROWTH
 * @brief Encapsulates all plant growth sub-processes for a single simulation time step.
 */
class GROWTH
{
public:
    GROWTH();
    ~GROWTH();

    /**
     * @brief Orchestrates all plant growth sub-processes for one simulation time step.
     */
    void doPlantGrowth(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community, INTERACTION interaction, ALLOMETRY allometry, SOIL &soil);

    /**
     * @brief Calculates gross primary productivity (GPP) for all plant cohorts.
     */
    void doPlantPhotosynthesis(UTILS utils, PARAMETER parameter, COMMUNITY &community, INTERACTION interaction);

    /**
     * @brief Calculates the CO₂ uptake rate for a single plant without community shading.
     */
    double calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI);

    /**
     * @brief Calculates CO₂ uptake for a single plant accounting for community shading
     *        across vertical height layers.
     */
    double calculateGPPOfPlantWithCommunityShading(UTILS utils, INTERACTION interaction, PARAMETER parameter, int pft, double plantHeight, double plantLAI);

    /**
     * @brief Calculates the CO₂ uptake rate for a single canopy height layer
     *        under community shading.
     */
    double calculateCO2UptakePerSecondAndSquareMeterWithCommunityShading(PARAMETER parameter, int pft, double lightExtinction, double photoactiveLai, double plantRadiation);

    /**
     * @brief Calculates maintenance respiration for all plant cohorts.
     */
    void doPlantRespiration(COMMUNITY &community, PARAMETER parameter, INTERACTION interaction);

    /**
     * @brief Derives NPP for each cohort as GPP minus maintenance and growth respiration.
     */
    void calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter);

    /**
     * @brief Returns a temperature-based reduction factor (0–1) for GPP.
     */
    double calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature);

    /**
     * @brief Returns a temperature-dependent scaling factor for maintenance respiration.
     */
    double calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature);

    /**
     * @brief Updates NPP allocation fractions (shoot, root, recruitment, exudates)
     *        for all cohorts based on current plant height and maturity status.
     */
    void adjustAllocationRates(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Distributes NPP among shoot, root, recruitment, and exudate biomass pools.
     */
    void doPlantNPPAllocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, SOIL &soil);

    /**
     * @brief Computes the fraction of NPP growth allocation directed to shoot biomass.
     */
    double calculateProportionalityFactorForAllocationDistributionToPlantParts(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft);

    /**
     * @brief Sets NPP allocation fractions for a mature plant cohort.
     */
    void adjustAllocationRatesForMaturePlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);

    /**
     * @brief Sets NPP allocation fractions for a regrowing (immature) plant cohort.
     */
    void adjustAllocationRatesForRegrowingPlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);

    /**
     * @brief Updates plant geometry and age for all cohorts after NPP allocation.
     */
    void doPlantGrowthInSizeAndAging(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, SOIL soil);
};