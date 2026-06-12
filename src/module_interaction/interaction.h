/**
 * @file interaction.h
 * @brief Declares the INTERACTION class, which computes light attenuation through
 *        the canopy and provides daily environmental conditions to the model.
 *
 * Light attenuation is modelled by discretising the canopy into horizontal height
 * layers of fixed width (HEIGHT_LAYER_WIDTH, defined in constants.h). Leaf area
 * is distributed across those layers, accumulated top-down, and the Beer-Lambert
 * law is applied to derive the radiation available to each plant cohort.
 *
 * The class also retrieves the relevant daily weather variables (radiation,
 * temperatures, day length) and estimates soil surface temperature from an
 * empirical biomass-insulation model.
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_weather/weather.h"
#include "../utils/utils.h"
#include <vector>
#include <iostream>

/**
 * @class INTERACTION
 * @brief Manages plant–environment interaction, specifically canopy light
 *        attenuation and daily environmental state.
 */
class INTERACTION
{
public:
    INTERACTION();
    ~INTERACTION();

    /** @brief Photosynthetically active radiation above the canopy for the current day
     *         (MJ m⁻² d⁻¹ or µmol photons m⁻² s⁻¹, consistent with weather input). */
    double fullSunLight;

    /** @brief Length of the photoperiod for the current day (hours). */
    double dayLength;

    /** @brief Mean full-day air temperature for the current day (°C). */
    double fullDayAirTemperature;

    /** @brief Mean daytime air temperature for the current day (°C). */
    double dayTimeAirTemperature;

    /** @brief Estimated soil surface temperature for the current day (°C).
     *         Derived from air temperature and aboveground biomass insulation. */
    double soilTemperature;

    /**
     * @brief Index of the topmost height layer currently occupied by at least
     *        one plant cohort (0-based, in units of HEIGHT_LAYER_WIDTH).
     */
    int maximumHeightLayerIndexReachedByPlants;

    /**
     * @brief Raw leaf area index per height layer (m² m⁻²), accumulated top-down.
     * After accumulateLeafAreaFromTopToBottomHeightLayers(), `LAI[i]` contains
     * the total LAI of all canopy layers at and above layer `i`.
     * Sized to MAXIMUM_HEIGHT_LAYER + 1; reset to zero each time step.
     */
    std::vector<double> LAI;

    /**
     * @brief Extinction-weighted cumulative LAI per height layer (dimensionless).
     * Each entry equals the sum of (leaf area × PFT extinction coefficient) for
     * the layer and all layers above it, normalised by SIMULATION_AREA.
     * Used directly in the Beer-Lambert calculation.
     * Sized to MAXIMUM_HEIGHT_LAYER + 1; reset to zero each time step.
     */
    std::vector<double> LAIwithLightExtinction;

    /**
     * @brief Temporary per-cohort weights describing what fraction of a plant's
     *        total height falls within each height layer. All weights sum to 1.
     */
    std::vector<double> weightsForPlantContributionToHeightLayer;

    /**
     * @brief Reads the current day's environmental variables from the weather
     *        time series and computes soil temperature.
     */
    void getEnvironmentalConditionsOfDay(WEATHER weather, COMMUNITY community, int day, double abovegroundLitterBiomass);

    /**
     * @brief Estimates soil surface temperature from air temperature and
     *        aboveground biomass insulation.
     */
    double calculateSoilTemperature(COMMUNITY community, double fullDayAirTemperature, double abovegroundLitterBiomass);

    /**
     * @brief Orchestrates the full canopy light-attenuation pipeline.
     */
    void calculateLightAttenuationAndAvailabilityForPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community, double fullSunLight);

    /**
     * @brief Scans all cohorts to find the tallest plant and sets
     *        `maximumHeightLayerIndexReachedByPlants`.
     */
    void calculateNumberOfHeightLayersFromLargestPlant(UTILS utils, COMMUNITY &community);

    /**
     * @brief Distributes all cohorts' leaf area across height layers and
     *        accumulates LAI top-down.
     */
    void calculateCumulativeLeafAreaIndexAcrossHeightLayers(UTILS utils, COMMUNITY &community, PARAMETER parameter);

    /**
     * @brief Computes the proportional weight of a plant's contribution to each
     *        height layer it occupies. All weights sum to 1.
     */
    void calculateWeightsOfPlantHeightContributionToHeightLayers(int topHeightLayerIndexOfPlant, double plantHeight);

    /**
     * @brief Adds a cohort's leaf area to `LAI` and `LAIwithLightExtinction` for
     *        every height layer it occupies.
     */
    void addPlantLeafAreaToHeightLayers(int topHeightLayerIndexOfPlant, double leafAreaOfPlantCohort, double plantLightExtinctionCoefficient);

    /**
     * @brief Converts the per-layer LAI arrays to cumulative top-down values.
     */
    void accumulateLeafAreaFromTopToBottomHeightLayers(int maximumHeightLayerIndexReachedByPlants);

    /**
     * @brief Computes the radiation available to each living plant cohort via
     *        the Beer-Lambert law.
     */
    void calculateLightAvailabilityForPlants(UTILS utils, COMMUNITY &community, PARAMETER parameter, double fullSunLight);

    /**
     * @brief Reads `LAIwithLightExtinction[layerindex]` into the cohort's
     *        `cumulativeOvertoppingCommunityLAI` field.
     */
    void getOvertoppingCumulativeLeafAreaIndexOfPlant(COMMUNITY &community, int cohortindex, int top);

    /**
     * @brief Applies the Beer-Lambert law to derive the radiation available to a
     *        specific cohort and stores it in `availableRadiation`.
     */
    void calculateAvailableLightReachingAPlant(PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight);

    /**
     * @brief Computes and stores a dimensionless shading indicator (0–1) for a
     *        cohort.
     */
    void calculateShadingIndicatorOfPlantForOutput(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight);

    /**
     * @brief Applies the Beer-Lambert extinction law to compute transmitted radiation.
     */
    double getRadiationByLightExtinctionLaw(double cumulativeLAIAboveAndAtHeightLayer, double fullSunLight);
};