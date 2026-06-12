#include "interaction.h"

INTERACTION::INTERACTION() {};
INTERACTION::~INTERACTION() {};

/**
 * @brief Orchestrates the full light-attenuation pipeline for a single time step.
 *
 * Executes three stages in order:
 * 1. calculateNumberOfHeightLayersFromLargestPlant() — determine the uppermost
 *    occupied height layer from the tallest plant in the community.
 * 2. calculateCumulativeLeafAreaIndexAcrossHeightLayers() — distribute plant leaf
 *    area across height layers and accumulate LAI from top to bottom.
 * 3. calculateLightAvailabilityForPlants() — apply the Beer-Lambert law to derive
 *    the radiation available to each plant cohort.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   PFT-specific parameters, including light extinction coefficients.
 * @param community   Plant community; `availableRadiation` and related fields are
 *                    updated in place for every cohort.
 * @param fullSunLight Incident photosynthetically active radiation above the canopy
 *                    (MJ m⁻² d⁻¹ or µmol photons m⁻² s⁻¹, consistent with weather input).
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::calculateLightAttenuationAndAvailabilityForPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community, double fullSunLight)
{
    /// 1. calculate height of largest plant in the community to discretisize the aboveground space vertically into height layers
    calculateNumberOfHeightLayersFromLargestPlant(utils, community);

    /// 2. calculate leaf area index of all plants in the community across height layers
    /// in a cumulative way from top to bottom
    calculateCumulativeLeafAreaIndexAcrossHeightLayers(utils, community, parameter);

    /// 3. calculate how much cumulative leaf area index is overtopping and shading a plant cohort
    /// 4. and calculation of available sunlight reaching this plant cohort
    /// based on Beer-Lambert law of light transmission and extinction
    calculateLightAvailabilityForPlants(utils, community, parameter, fullSunLight);
}

/**
 * @brief Finds the tallest plant in the community and sets the corresponding
 *        height-layer index for subsequent LAI calculations.
 *
 * Iterates over all cohorts to update `community.maximumHeightOfAllPlants`.
 * Raises an error if that value exceeds the hard-coded constant
 * `MAXIMUM_HEIGHT_LAYER` (defined in constants.h). Otherwise, stores the
 * index of the topmost occupied layer in `maximumHeightLayerIndexReachedByPlants`,
 * which limits the extent of subsequent height-layer loops.
 *
 * @param utils     Utility object for error handling.
 * @param community Plant community; `maximumHeightOfAllPlants` is updated in place.
 */
void INTERACTION::calculateNumberOfHeightLayersFromLargestPlant(UTILS utils, COMMUNITY &community)
{
    // maximumHeightOfAllPlants is initialized (with 0) in every day step in initAndResetProcessVariables()
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants[cohortindex]->height > community.maximumHeightOfAllPlants)
        {
            community.maximumHeightOfAllPlants = community.allPlants[cohortindex]->height;
        }
    }
    /// predefined maximum height layer (see module_init/constants.h)
    if (community.maximumHeightOfAllPlants > MAXIMUM_HEIGHT_LAYER)
    {
        utils.handleError("Maximum plant height is exceeding the constant parameter MAXIMUM_HEIGHT_LAYER. The parameter should be set up in module_init/constants.h!");
    }
    else
    {
        /// reduce for-loops by calculating maximum height layer reached by currently largest plant in community
        maximumHeightLayerIndexReachedByPlants = (int)std::floor(community.maximumHeightOfAllPlants / HEIGHT_LAYER_WIDTH + NUMERIC_TOLERANCE);
    }
}

/**
 * @brief Distributes all plant cohorts' leaf area across vertical height layers
 *        and accumulates LAI from the top layer downwards.
 *
 * For each living cohort (amount > 0), the function:
 * 1. Computes the cohort's total leaf area (amount × covered area × LAI).
 * 2. Calls calculateWeightsOfPlantHeightContributionToHeightLayers() to obtain
 *    the proportional contribution of the plant to each layer it occupies.
 * 3. Calls addPlantLeafAreaToHeightLayers() to accumulate raw LAI and
 *    extinction-weighted LAI (`LAIwithLightExtinction`) per layer.
 *
 * After processing all cohorts, calls accumulateLeafAreaFromTopToBottomHeightLayers()
 * to produce the cumulative top-down extinction profile used in the Beer-Lambert
 * light calculation.
 *
 * @param utils     Utility object for error handling.
 * @param community Plant community; `LAI` and `LAIwithLightExtinction` arrays are
 *                  updated in place.
 * @param parameter PFT-specific parameters; provides `lightExtinctionCoefficients`.
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::calculateCumulativeLeafAreaIndexAcrossHeightLayers(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{
    /// go through all living plants in the community and add their leaf area to the respective height layers
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        double plantAmount = community.allPlants.at(cohortindex)->amount;
        int pft = community.allPlants.at(cohortindex)->pft;
        double plantHeight = community.allPlants.at(cohortindex)->height;
        double plantArea = community.allPlants.at(cohortindex)->coveredArea;
        double plantLeafAreaIndex = community.allPlants.at(cohortindex)->lai;
        double plantLightExtinctionCoefficient = parameter.lightExtinctionCoefficients.at(pft);
        double leafAreaOfPlantCohort = plantAmount * plantArea * plantLeafAreaIndex;

        if (plantAmount > 0)
        {
            /// search for height layer up to which the plant cohort is reaching to
            /// Note: floor is used because first height layer 0-1 cm has index 0
            int topHeightLayerIndexOfPlant = (int)std::floor((plantHeight / HEIGHT_LAYER_WIDTH) + NUMERIC_TOLERANCE);

            /// calculate weights for the plants' contribution to height layers based on plant height
            calculateWeightsOfPlantHeightContributionToHeightLayers(topHeightLayerIndexOfPlant, plantHeight);

            /// add plant cohort's leaf area to height layers according to the respective weights
            addPlantLeafAreaToHeightLayers(topHeightLayerIndexOfPlant, leafAreaOfPlantCohort, plantLightExtinctionCoefficient);
        }
    }

    /// accumulate the leaf area index across height layers from top to bottom
    accumulateLeafAreaFromTopToBottomHeightLayers(maximumHeightLayerIndexReachedByPlants);
}

/**
 * @brief Computes the proportional contribution of a plant to each height layer
 *        it occupies.
 *
 * For all full layers below the plant's top layer, the weight equals
 * `HEIGHT_LAYER_WIDTH / plantHeight`. For the topmost (partial) layer, the weight
 * is the remaining fractional height divided by `plantHeight`, ensuring that all
 * weights sum to 1. Results are stored in `weightsForPlantContributionToHeightLayer`.
 *
 * @param topHeightLayerIndexOfPlant Index of the uppermost height layer occupied
 *                                   by the plant (0-based, computed via `floor`).
 * @param plantHeight                Total plant height (cm).
 */
void INTERACTION::calculateWeightsOfPlantHeightContributionToHeightLayers(int topHeightLayerIndexOfPlant, double plantHeight)
{
    double plantPartInHeightLayer;
    weightsForPlantContributionToHeightLayer.clear();

    for (int layerindex = 0; layerindex < topHeightLayerIndexOfPlant; layerindex++)
    {
        /// plant parts fully cover the respective height layer
        plantPartInHeightLayer = HEIGHT_LAYER_WIDTH / plantHeight;
        weightsForPlantContributionToHeightLayer.push_back(plantPartInHeightLayer);
    }

    /// plant parts at top layer (topHeightLayerIndexOfPlant) may not fully cover the entire height layer
    /// downward correction is required
    plantPartInHeightLayer = (plantHeight - std::floor(plantHeight / HEIGHT_LAYER_WIDTH + NUMERIC_TOLERANCE) * HEIGHT_LAYER_WIDTH) / plantHeight;
    weightsForPlantContributionToHeightLayer.push_back(plantPartInHeightLayer);
}

/**
 * @brief Adds a plant cohort's leaf area contribution to each height layer it
 *        occupies, weighted by `weightsForPlantContributionToHeightLayer`.
 *
 * Updates two parallel arrays normalised by `SIMULATION_AREA`:
 * - `LAI`: raw leaf area index per layer.
 * - `LAIwithLightExtinction`: extinction-weighted LAI per layer
 *   (leaf area × PFT-specific light extinction coefficient).
 *
 * Must be called after calculateWeightsOfPlantHeightContributionToHeightLayers()
 * has populated `weightsForPlantContributionToHeightLayer`.
 *
 * @param topHeightLayerIndexOfPlant      Index of the uppermost layer occupied by
 *                                        the cohort.
 * @param leafAreaOfPlantCohort           Total leaf area of the cohort
 *                                        (amount × covered area × LAI, cm²).
 * @param plantLightExtinctionCoefficient PFT-specific light extinction coefficient
 *                                        (dimensionless).
 */
void INTERACTION::addPlantLeafAreaToHeightLayers(int topHeightLayerIndexOfPlant, double leafAreaOfPlantCohort, double plantLightExtinctionCoefficient)
{
    for (int layerindex = 0; layerindex <= topHeightLayerIndexOfPlant; layerindex++)
    {
        LAI.at(layerindex) += (leafAreaOfPlantCohort * weightsForPlantContributionToHeightLayer.at(layerindex) / SIMULATION_AREA);
        LAIwithLightExtinction.at(layerindex) += (leafAreaOfPlantCohort * plantLightExtinctionCoefficient * weightsForPlantContributionToHeightLayer.at(layerindex) / SIMULATION_AREA);
    }
}

/**
 * @brief Converts the per-layer LAI and LAIwithLightExtinction arrays from
 *        layer-local to cumulative top-down values.
 *
 * Starting from the layer just below the topmost occupied layer and working
 * downward, each layer's value is incremented by the value of the layer above.
 * After this pass, `LAI[i]` and `LAIwithLightExtinction[i]` represent the total
 * leaf area above and including layer `i`, which is required by the Beer-Lambert
 * light extinction law.
 *
 * @param maximumHeightLayerIndexReachedByPlants Index of the topmost occupied
 *                                               height layer (inclusive upper bound
 *                                               of the accumulation loop).
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::accumulateLeafAreaFromTopToBottomHeightLayers(int maximumHeightLayerIndexReachedByPlants)
{
    for (int layerindex = maximumHeightLayerIndexReachedByPlants - 1; layerindex >= 0; layerindex--)
    {
        LAI.at(layerindex) += LAI.at(layerindex + 1);
        LAIwithLightExtinction.at(layerindex) += LAIwithLightExtinction.at(layerindex + 1);
    }
}

/**
 * @brief Calculates the radiation available to each plant cohort using the
 *        Beer-Lambert light extinction law.
 *
 * For every living cohort, the function:
 * 1. Determines the lowest height-layer index that is fully above the plant
 *    top (`ceil(height / HEIGHT_LAYER_WIDTH)`), i.e. the layer from which
 *    overtopping shading is read.
 * 2. Reads the cumulative overtopping LAI for that layer via
 *    getOvertoppingCumulativeLeafAreaIndexOfPlant().
 * 3. Converts the cumulative LAI to available radiation via
 *    calculateAvailableLightReachingAPlant().
 * 4. Computes a dimensionless shading indicator (0–1) for output via
 *    calculateShadingIndicatorOfPlantForOutput().
 *
 * @param utils       Utility object for warnings when full sun light is zero.
 * @param community   Plant community; `availableRadiation`, `cumulativeOvertoppingCommunityLAI`,
 *                    and `shadingIndicator` are updated for every living cohort.
 * @param parameter   PFT-specific parameters (passed through to shading indicator).
 * @param fullSunLight Incident radiation above the canopy.
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::calculateLightAvailabilityForPlants(UTILS utils, COMMUNITY &community, PARAMETER parameter, double fullSunLight)
{
    int lowestOvertoppingHeightLayerIndexOfPlant;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        double plantAmount = community.allPlants.at(cohortindex)->amount;
        int pft = community.allPlants.at(cohortindex)->pft;
        double plantHeight = community.allPlants.at(cohortindex)->height;

        if (plantAmount > 0)
        {
            lowestOvertoppingHeightLayerIndexOfPlant = (int)std::ceil(plantHeight / HEIGHT_LAYER_WIDTH - NUMERIC_TOLERANCE);

            getOvertoppingCumulativeLeafAreaIndexOfPlant(community, cohortindex, lowestOvertoppingHeightLayerIndexOfPlant);
            calculateAvailableLightReachingAPlant(parameter, community, cohortindex, fullSunLight);
            calculateShadingIndicatorOfPlantForOutput(utils, parameter, community, cohortindex, fullSunLight);
        }
    }
}

/**
 * @brief Reads and stores the cumulative overtopping LAI for a given plant cohort.
 *
 * Copies `LAIwithLightExtinction[layerindex]` — the extinction-weighted cumulative
 * LAI from all layers at and above `layerindex` — into the cohort's
 * `cumulativeOvertoppingCommunityLAI` field. This value is subsequently used by
 * calculateAvailableLightReachingAPlant() to attenuate incoming radiation.
 *
 * @param community   Plant community; `cumulativeOvertoppingCommunityLAI` is written
 *                    for the cohort at `cohortindex`.
 * @param cohortindex Index of the target plant cohort.
 * @param layerindex  Index of the lowest height layer fully above the plant top
 *                    (i.e. the layer whose cumulative LAI represents full overtopping).
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::getOvertoppingCumulativeLeafAreaIndexOfPlant(COMMUNITY &community, int cohortindex, int layerindex)
{
    community.allPlants.at(cohortindex)->cumulativeOvertoppingCommunityLAI = LAIwithLightExtinction.at(layerindex);
}

/**
 * @brief Applies the Beer-Lambert law to compute the radiation available to a
 *        specific plant cohort from its overtopping cumulative LAI.
 *
 * Reads `cumulativeOvertoppingCommunityLAI` from the cohort and calls
 * getRadiationByLightExtinctionLaw() to derive the attenuated radiation, which
 * is then stored in `availableRadiation`.
 *
 * @param parameter   Passed through to getRadiationByLightExtinctionLaw() (unused
 *                    directly here; reserved for future parameterisation).
 * @param community   Plant community; `availableRadiation` is updated for the cohort
 *                    at `cohortindex`.
 * @param cohortindex Index of the target plant cohort.
 * @param fullSunLight Incident radiation above the canopy.
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
void INTERACTION::calculateAvailableLightReachingAPlant(PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight)
{
    double shadingCommunityLeafAreaIndex = community.allPlants.at(cohortindex)->cumulativeOvertoppingCommunityLAI;
    community.allPlants.at(cohortindex)->availableRadiation = getRadiationByLightExtinctionLaw(shadingCommunityLeafAreaIndex, fullSunLight);
}

/**
 * @brief Computes a dimensionless shading indicator (0–1) for a plant cohort
 *        and stores it for output.
 *
 * Defined as `availableRadiation / fullSunLight`. A value of 1 means the plant
 * receives full sunlight; a value approaching 0 means it is almost completely
 * shaded. If `fullSunLight` is zero (e.g. polar night), the indicator is set to
 * −1 and a warning is issued.
 *
 * @param utils       Utility object; issues a warning when `fullSunLight == 0`.
 * @param parameter   Reserved for future use.
 * @param community   Plant community; `shadingIndicator` is written for the cohort
 *                    at `cohortindex`.
 * @param cohortindex Index of the target plant cohort.
 * @param fullSunLight Incident radiation above the canopy.
 */
void INTERACTION::calculateShadingIndicatorOfPlantForOutput(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight)
{
    double sunLightReachingPlant = community.allPlants.at(cohortindex)->availableRadiation;
    if (fullSunLight > 0)
    {
        community.allPlants.at(cohortindex)->shadingIndicator = sunLightReachingPlant / fullSunLight;
    }
    else
    {
        community.allPlants.at(cohortindex)->shadingIndicator = -1.0;
        utils.handleWarning("There is no sunlight today. The shading factor is set to default value of -1.");
    }
}

/**
 * @brief Applies the Beer-Lambert light extinction law to compute transmitted
 *        radiation.
 *
 * @f[ I = I_0 \cdot e^{-\text{LAI}_{\text{cum}}} @f]
 *
 * where @f$I_0@f$ is the full-sun radiation above the canopy and
 * @f$\text{LAI}_{\text{cum}}@f$ is the cumulative extinction-weighted LAI
 * of all layers at and above the reference height. The result represents
 * the radiation reaching the lower boundary of the layer with that
 * cumulative LAI.
 *
 * @param cumulativeLAIAboveAndAtHeightLayer Extinction-weighted cumulative LAI
 *                                          from the top of the canopy down to
 *                                          (and including) the target layer
 *                                          (dimensionless).
 * @param fullSunLight                      Incident radiation above the canopy.
 * @return Radiation at the lower boundary of the specified layer (same units as
 *         `fullSunLight`).
 * @cite Concept based on the forest model FORMIND (www.formind.org)
 */
double INTERACTION::getRadiationByLightExtinctionLaw(double cumulativeLAIAboveAndAtHeightLayer, double fullSunLight)
{
    /// the resulting radiationByExtinction is the remaining sun light that reaches the lower boundary of a height layer
    /// whose cumulativeLAI (i.e. LAIwithLightExtinction) equals the leaf area at this layer and of all layers above
    double radiationByExtinction = exp(-cumulativeLAIAboveAndAtHeightLayer) * fullSunLight;
    return radiationByExtinction;
}

/**
 * @brief Reads the current day's environmental conditions from the weather
 *        time series and stores them in the INTERACTION struct.
 *
 * Copies the following fields for `day` (1-based) from the WEATHER vectors:
 * - `fullSunLight` — photosynthetically active radiation.
 * - `dayLength` — length of the photoperiod (hours).
 * - `dayTimeAirTemperature` — mean daytime air temperature (°C).
 * - `fullDayAirTemperature` — mean full-day air temperature (°C).
 * - `soilTemperature` — estimated via calculateSoilTemperature() from air
 *   temperature and aboveground biomass/litter.
 *
 * @param weather                 Weather struct containing the full simulation
 *                                time series (0-based index = day − 1).
 * @param community               Plant community; shoot biomass summed for the
 *                                soil temperature calculation.
 * @param day                     Current simulation day (1-based).
 * @param abovegroundLitterBiomass Total aboveground litter biomass (g m⁻²), used
 *                                 in the soil temperature insulation model.
 * @cite Concept of soil temperature caculation based on CENTURY 4.0 model.
 */
void INTERACTION::getEnvironmentalConditionsOfDay(WEATHER weather, COMMUNITY community, int day, double abovegroundLitterBiomass)
{
    fullSunLight = weather.photosyntheticPhotonFluxDensity.at(day - 1); // parameter.day starts at 1, but vectors start with index 0
    dayLength = weather.dayLength.at(day - 1);
    dayTimeAirTemperature = weather.dayTimeAirTemperature.at(day - 1);
    fullDayAirTemperature = weather.fullDayAirTemperature.at(day - 1);
    soilTemperature = calculateSoilTemperature(community, fullDayAirTemperature, abovegroundLitterBiomass);
}

/**
 * @brief Estimates the soil surface temperature from air temperature and
 *        aboveground biomass insulation.
 *
 * Combines two temperature ranges from an empirical model (based on aboveground
 * biomass including a 40 % contribution from litter, capped at 600 g m⁻²):
 * - **Upper range**: accounts for solar heating, reduced by a dense canopy.
 * - **Lower range**: a linear correction for biomass-driven insulation.
 *
 * Currently, the soil temperature is set to the lower range only (the mean
 * of both ranges is available but commented out).
 *
 * @param community               Plant community; used to sum total aboveground
 *                                shoot biomass weighted by cohort amounts.
 * @param fullDayAirTemperature   Mean full-day air temperature (°C).
 * @param abovegroundLitterBiomass Total aboveground litter biomass (g m⁻²).
 * @return Estimated soil surface temperature (°C).
 * @cite Concept of soil temperature caculation based on CENTURY 4.0 model.
 */
double INTERACTION::calculateSoilTemperature(COMMUNITY community, double fullDayAirTemperature, double abovegroundLitterBiomass)
{
    double abovegroundBiomassOfAllPlants = 0;
    if (community.allPlants.size() > 0)
    {
        for (int cohortindex = 0; cohortindex < community.allPlants.size(); cohortindex++)
        {
            abovegroundBiomassOfAllPlants += community.allPlants[cohortindex]->shootBiomass * community.allPlants[cohortindex]->amount;
        }
    }

    double abovegroundBiomass = abovegroundBiomassOfAllPlants + 0.4 * abovegroundLitterBiomass;
    abovegroundBiomass = std::min(abovegroundBiomass, 600.0);

    double upperRange = fullDayAirTemperature + (25.4 / (1.0 + 18.0 * exp(-0.20 * fullDayAirTemperature))) * (exp(-0.0035 * abovegroundBiomass) - 0.13);
    double lowerRange = fullDayAirTemperature + 0.004 * abovegroundBiomass - 1.78;

    // alternative calculation: double soilTemperature = (upperRange + lowerRange) / 2.0;
    double soilTemperature = lowerRange;

    return soilTemperature;
}
