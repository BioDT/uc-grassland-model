#include "growth.h"

GROWTH::GROWTH() {};
GROWTH::~GROWTH() {};

/**
 * @brief Main function of plant growth
 * @cite Concept of plant NPP based on the carbon balance of photosynthesis and respiration
 *       based on the forest model FORMIND (www.formind.org)
 */
void GROWTH::doPlantGrowth(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community, INTERACTION interaction, ALLOMETRY allometry, SOIL &soil)
{
    /* Plant GPP (gross primary productivity) */
    doPlantPhotosynthesis(utils, parameter, community, interaction);

    /* Plant soil water demand, uptake and limitation of plant GPP by unfavorable soil water conditions */
    soil.doPlantSoilWaterUptakeAndGppLimitationBySoilWaterConditions(utils, parameter, weather, community);

    /* Plant respiration */
    doPlantRespiration(community, parameter, interaction);

    /* Plant NPP (net primary productivity) */
    calculatePlantNPPFromGPPAndRespiration(community, parameter);

    /* Plant soil nitrogen demand, uptake and limitation of plant GPP by unfavorable soil nitrogen conditions */
    soil.doPlantSoilNitrogenUptakeAndNppLimitationBySoilNitrogenConditions(utils, parameter, community);

    /* Plant allocation of NPP and according C and N parts */
    adjustAllocationRates(utils, parameter, community);
    doPlantNPPAllocation(utils, parameter, community, soil);

    /* Plant growth in size based on NPP allocation and aging */
    doPlantGrowthInSizeAndAging(utils, parameter, community, allometry, soil);
}

/**
 * @brief Performs photosynthesis calculation for all plant cohorts in the community.
 *
 * This function loops through each plant cohort in the community and calculates
 * the gross primary productivity (GPP) based on individual plant traits and environmental
 * conditions such as radiation and air temperature.
 *
 * The daily radiation is adjusted for the actual day length, and the effect of
 * daytime temperature on photosynthesis is taken into account. The resulting GPP
 * is stored directly in each plant instance.
 *
 * @param parameter     Struct containing species- or PFT-specific physiological parameters.
 * @param community     Reference to the COMMUNITY object containing all plant cohorts.
 * @param interaction   Struct holding current environmental conditions such as radiation, air temperature, and day length, potentially modified through plant interactions.
 *
 * @see calculateGPPOfPlant()
 * @see calculateEffectOfAirTemperatureOnGPP()
 * @cite Concept of plant photosynthesis is based on the forest model FORMIND (www.formind.org)
 */
void GROWTH::doPlantPhotosynthesis(UTILS utils, PARAMETER parameter, COMMUNITY &community, INTERACTION interaction)
{
    double CO2UptakePerSecondAndSquareMeter = 0.0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        double plantLAI = community.allPlants.at(cohortindex)->laiGreen;
        double plantHeight = community.allPlants.at(cohortindex)->height;
        double plantCoveredArea = community.allPlants.at(cohortindex)->coveredArea;
        double effectOfDayTimeTemperature = calculateEffectOfAirTemperatureOnGPP(interaction.dayTimeAirTemperature);

        if (parameter.communityShadingInGppCalculation)
        {
            CO2UptakePerSecondAndSquareMeter = calculateGPPOfPlantWithCommunityShading(utils, interaction, parameter, pft, plantHeight, plantLAI);
        }
        else
        {
            double plantRadiation = (24.0 / interaction.dayLength) * community.allPlants.at(cohortindex)->availableRadiation; // correct mean daily radiation by daylength hours for photosynthesis
            CO2UptakePerSecondAndSquareMeter = calculateCO2UptakePerSecondAndSquareMeter(parameter, pft, plantRadiation, plantLAI);
        }

        // conversion
        double OdmUptakePerSecondAndSquareMeter = CO2UptakePerSecondAndSquareMeter * CO2_CONVERSION_TO_ODM * MOLAR_MASS_OF_CO2; // conversion from CO2 to Odm
        double OdmUptakePerSecondAndSquareCentimeter = OdmUptakePerSecondAndSquareMeter / (100.0 * 100.0);
        double secondsPerDay = interaction.dayLength * 60 * 60;                                                      // scaling from seconds to day
        double plantPhotosynthesisPerDay = OdmUptakePerSecondAndSquareCentimeter * secondsPerDay * plantCoveredArea; // scaling to plant

        community.allPlants.at(cohortindex)->gpp = effectOfDayTimeTemperature * plantPhotosynthesisPerDay; // g ODM per day and plant
    }
}

double GROWTH::calculateGPPOfPlantWithCommunityShading(UTILS utils, INTERACTION interaction, PARAMETER parameter, int pft, double plantHeight, double plantLAI)
{
    double CO2UptakePerSecondAndSquareMeter = 0.0;
    double plantGreenLaiContributionToLayer;
    double communityLightExtinctionExponentInLayer;

    int topHeightLayerIndexOfPlant = (int)std::floor((plantHeight / HEIGHT_LAYER_WIDTH) + NUMERIC_TOLERANCE);

    for (int heightLayerIndex = 0; heightLayerIndex <= topHeightLayerIndexOfPlant; heightLayerIndex++)
    {
        // TODO: clarify suitable naming
        double extinctionExponentTopOfHeightLayer = interaction.LAIwithLightExtinction.at(heightLayerIndex + 1);
        double incomingRadiationTopOfHeightLayer = interaction.getRadiationByLightExtinctionLaw(extinctionExponentTopOfHeightLayer, interaction.fullSunLight);
        incomingRadiationTopOfHeightLayer *= (24.0 / interaction.dayLength);

        // remove this part as it is only for the output but less meaningful here
        /*if (heightLayerIndex == topHeightLayerIndexOfPlant)
        {
            community.allPlants.at(cohortindex)->incomingRadiation = incomingRadiationTopOfHeightLayer;
            // relevant for output: plant->limitingFactorLightShading (now with different (less useful) meaning)
        }*/

        
        if (heightLayerIndex == topHeightLayerIndexOfPlant)
        {
            plantGreenLaiContributionToLayer = plantLAI / plantHeight * (plantHeight - topHeightLayerIndexOfPlant * HEIGHT_LAYER_WIDTH);
        }
        else
        {
            plantGreenLaiContributionToLayer = plantLAI / plantHeight * HEIGHT_LAYER_WIDTH;
        }

        communityLightExtinctionExponentInLayer = interaction.LAIwithLightExtinction.at(heightLayerIndex) - interaction.LAIwithLightExtinction.at(heightLayerIndex + 1);
        if (!(plantHeight - heightLayerIndex * HEIGHT_LAYER_WIDTH < NUMERIC_TOLERANCE))
        {
            if (communityLightExtinctionExponentInLayer == 0)
            {
                utils.handleError("Light extinction is zero while plant LAI is not!");
            }

            // calculate photosynthesis [mumolCO2 s-1 m-2]
            CO2UptakePerSecondAndSquareMeter += calculateCO2UptakePerSecondAndSquareMeterWithCommunityShading(parameter, pft, communityLightExtinctionExponentInLayer, plantGreenLaiContributionToLayer,
                                                                                                              incomingRadiationTopOfHeightLayer);
        }
    }

    return CO2UptakePerSecondAndSquareMeter;
}

/**
 * @brief Calculates the CO₂ uptake rate per second and per square meter of leaf area.
 *
 * This function implements a light response model of photosynthesis for a plant functional
 * type (PFT), using the Beer-Lambert law and a non-rectangular hyperbola to estimate
 * the carbon assimilation rate based on absorbed light.
 *
 * Key components:
 * - Uses the initial slope of the light response curve (`alpha`),
 * - A light extinction coefficient (`k`), and
 * - The maximum gross photosynthetic rate of a single leaf (`pmax`).
 *
 * The equation integrates light attenuation through the canopy (via LAI) and calculates
 * how efficiently a plant converts available light into carbon gain.
 *
 * @param parameter       Struct containing PFT-specific physiological parameters.
 * @param pft             Index of the plant functional type (PFT).
 * @param plantRadiation  Incoming radiation (MJ/m²/day) available to the plant.
 * @param plantLAI        Leaf Area Index of the plant (unitless).
 *
 * @return CO₂ uptake rate in mol CO₂ per second per square meter of leaf area.
 * @cite Concept of plant CO2 uptake is based on the forest model FORMIND (www.formind.org)
 */
double GROWTH::calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI)
{
    if (plantRadiation == 0)
    {
        return 0;
    }
    else
    {
        const double alpha = parameter.initialSlopeOfLightResponseCurve.at(pft);
        const double k = parameter.lightExtinctionCoefficients.at(pft);
        const double pmax = parameter.maximumGrossLeafPhotosynthesisRate.at(pft);

        const double calcPart1 = alpha * k * plantRadiation;
        const double calcPart2 = pmax * (1 - LIGHT_TRANSMISSION_COEFFICIENT);

        double CO2UptakePerSecondsAndSquareMeter = ((pmax / k) * log((calcPart1 + calcPart2) / (calcPart1 * exp(-k * plantLAI) + calcPart2)));
        return (CO2UptakePerSecondsAndSquareMeter);
    }
}

double GROWTH::calculateCO2UptakePerSecondAndSquareMeterWithCommunityShading(PARAMETER parameter, int pft, double lightExtinctionExponent, double photoactiveLai, double plantRadiation)
{
    if (plantRadiation == 0)
    {
        return 0;
    }
    else
    {
        const double alpha = parameter.initialSlopeOfLightResponseCurve.at(pft);
        const double k = parameter.lightExtinctionCoefficients.at(pft);
        const double pmax = parameter.maximumGrossLeafPhotosynthesisRate.at(pft);

        const double calcPart1 = alpha * k * plantRadiation;
        const double calcPart2 = pmax * (1 - LIGHT_TRANSMISSION_COEFFICIENT);
        double CO2UptakePerSecondsAndSquareMeter = (photoactiveLai / lightExtinctionExponent * pmax * log((calcPart1 + calcPart2) / (calcPart1 * exp(-lightExtinctionExponent) + calcPart2)));

        return CO2UptakePerSecondsAndSquareMeter;
    }
}

/**
 * @brief Calculates the temperature-based reduction factor for gross primary productivity (GPP).
 *
 * This function determines how daytime air temperature influences the plant's ability
 * to photosynthesize. It returns a dimensionless reduction factor between 0 and 1
 * that scales GPP accordingly.
 *
 * The relationship is piecewise linear and defined as follows:
 * - **For temperatures ≤ -5°C**:      no photosynthesis (factor = 0)
 * - **Between -5°C and 2°C**:         linear increase from 0 to ~0.2
 * - **Between 2°C and 10°C**:         linear increase from ~0.2 to 1.0
 * - **Above 10°C**:                   no limitation (factor = 1.0)
 *
 * @param dayTimeAirTemperature  Daytime air temperature in degrees Celsius.
 *
 * @return Reduction factor (0.0 – 1.0) for GPP based on air temperature.
 * @cite Temperature effects are based on publication:
 *       Schippers & Kropff 2001, Functional Ecology 15, 155–164
 */
double GROWTH::calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature)
{
    double reductionFactor = 0;
    int day;

    if (dayTimeAirTemperature <= -5)
    {
        reductionFactor = 0;
    }
    else if (dayTimeAirTemperature <= 2)
    {
        reductionFactor = (0.02857 * dayTimeAirTemperature + 0.142);
    }
    else if (dayTimeAirTemperature <= 10)
    {
        reductionFactor = (0.1 * dayTimeAirTemperature);
    }
    else
    {
        reductionFactor = 1.0;
    }

    return reductionFactor;
}

/**
 * @brief Calculates maintenance respiration for all plant cohorts in the community.
 *
 * This function iterates over each plant cohort in the community and computes
 * their daily maintenance respiration based on biomass and temperature.
 *
 * The respiration rate is determined by:
 * - The sum of green shoot and root biomass,
 * - A PFT-specific base respiration rate,
 * - A temperature-dependent correction factor from
 *   `calculateEffectOfAirTemperatureOnRespiration()`.
 *
 * @note Here, only maintenance respiration is considered. Growth respiration is calculated later on.
 *
 * @param community   Reference to the plant community containing all cohorts.
 * @param parameter   Struct with physiological parameters, including respiration rates per PFT.
 * @param interaction Struct holding environmental data, such as full-day air temperature.
 *
 * @see calculateEffectOfAirTemperatureOnRespiration()
 */
void GROWTH::doPlantRespiration(COMMUNITY &community, PARAMETER parameter, INTERACTION interaction)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        double greenShootBiomass = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves;
        double rootBiomass = community.allPlants.at(cohortindex)->rootBiomass;
        double effectOfTemperature = calculateEffectOfAirTemperatureOnRespiration(parameter, interaction.fullDayAirTemperature);
        community.allPlants.at(cohortindex)->maintenanceRespiration = effectOfTemperature * parameter.maintenanceRespirationRate * (greenShootBiomass + rootBiomass);
    }
}

/**
 * @brief Calculates a temperature-dependent scaling factor for plant respiration.
 *
 * This function estimates how air temperature affects the rate of maintenance
 * respiration using a Q₁₀ temperature response model. The result is a dimensionless
 * factor used to scale base respiration rates.
 *
 * The temperature response is modeled as follows:
 * - **For T > 15°C**: Q₁₀ model is applied where `Q10` is the base rate and `T_ref` is the reference temperature.
 * - **For T ≤ 0°C**: Respiration is fully suppressed (factor = 0).
 * - **For 0°C < T ≤ 15°C**: A linear interpolation from 0 to ~0.5.
 *
 * @param parameter      Struct containing plant functional type-specific parameters,
 *                       including `plantResponseToTemperatureQ10Base` and reference temperature.
 * @param airTemperature Full-day mean air temperature in degrees Celsius.
 *
 * @return Temperature-dependent reduction factor for maintenance respiration (unitless).
 * @cite Temperature effect is based on publication:
 *       Schippers & Kropff 2001, Functional Ecology 15, 155–164
 */
double GROWTH::calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature)
{
    double reductionFactor = 0;

    if (airTemperature > 15)
    {
        double tExponent = (airTemperature - parameter.plantResponseToTemperatureQ10Reference) / 10.0;
        reductionFactor = std::pow(parameter.plantResponseToTemperatureQ10Base, tExponent);
    }
    else if (airTemperature <= 0)
        reductionFactor = 0;
    else
        reductionFactor = 0.03333 * airTemperature;

    return reductionFactor;
}

/**
 * @brief Calculates Net Primary Production (NPP) for each plant cohort based on GPP and respiration.
 *
 * This function computes the Net Primary Production (NPP) by subtracting both
 * maintenance and growth respiration from the gross primary productivity (GPP)
 * for each plant cohort in the community.
 *
 * @note **Growth respiration** is calculated only if GPP exceeds maintenance respiration. If GPP is less than maintenance respiration, all photosynthesis is allocated to maintenance respiration, and NPP is set to 0.
 *
 * @param community  Reference to the COMMUNITY object containing all plant cohorts.
 * @param parameter  Struct with plant functional type-specific parameters, including the growth respiration fraction.
 *
 * @note A buffer mechanism (`nppBuffer`) for carrying over negative NPP is included but currently commented out.
 * @see doPlantRespiration(), doPlantPhotosynthesis()
 * @cite Concept of plant NPP based on the carbon balance of photosynthesis and respiration including buffer is
 *       based on the forest model FORMIND (www.formind.org)
 */
void GROWTH::calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        double plantGPP = community.allPlants.at(cohortindex)->gpp;
        double maintenanceRespiration = community.allPlants.at(cohortindex)->maintenanceRespiration;
        double growthRespiration = 0;

        if (plantGPP > maintenanceRespiration)
        {
            growthRespiration = parameter.growthRespirationFraction * (plantGPP - maintenanceRespiration);
            community.allPlants.at(cohortindex)->growthRespiration = growthRespiration;
        }
        else
        {
            growthRespiration = 0;
            community.allPlants.at(cohortindex)->growthRespiration = 0;
        }

        community.allPlants.at(cohortindex)->totalRespiration = maintenanceRespiration + growthRespiration;

        double biomassIncrement = plantGPP - community.allPlants.at(cohortindex)->totalRespiration + community.allPlants.at(cohortindex)->nppBuffer;

        // ==== reset buffer for next year ====
        community.allPlants.at(cohortindex)->nppBuffer = 0.0; // reset for current year
        if (biomassIncrement < 0)
        {
            community.allPlants.at(cohortindex)->nppBuffer = biomassIncrement;      // add negative NPP to buffer and adjust balance
            community.allPlants.at(cohortindex)->maintenanceRespiration = plantGPP; // all GPP is used for respiration
            community.allPlants.at(cohortindex)->growthRespiration = 0;
            community.allPlants.at(cohortindex)->totalRespiration = maintenanceRespiration;
            biomassIncrement = 0;
        }

        community.allPlants.at(cohortindex)->npp = biomassIncrement;
    }
}

/**
 * @brief Allocates Net Primary Production (NPP) among various biomass pools
 *        for each plant cohort in the community.
 *
 * This function iterates through all plant cohorts in the community and
 * allocates biomass increments to aboveground shoots, belowground roots,
 * recruitment biomass for seed production, and exudates based on the
 * respective allocation fractions.
 *
 * @param utils A reference to a UTILS object that provides utility functions,
 *              including error handling.
 * @param community A reference to a COMMUNITY object that contains all the
 *                  plant cohorts and their associated data.
 *
 * This function performs the following operations for each plant cohort:
 * - Updates green leaf biomass based on NPP allocation.
 * - Updates total shoot biomass and calculates the fractions of green and
 *   brown biomass.
 * - Allocates biomass to belowground root systems.
 * - Updates total plant biomass as the sum of shoot and root biomass.
 * - Allocates biomass for recruitment (seed production) and exudates.
 *
 * @warning This function assumes that the NPP and allocation fractions
 *          are properly initialized and valid.
 * @throws std::runtime_error if the sum of green and brown biomass fractions
 *         does not equal 1, indicating an inconsistency in biomass allocation.
 */
void GROWTH::doPlantNPPAllocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, SOIL &soil)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        double biomassIncrementForAllocation = community.allPlants.at(cohortindex)->npp;

        if (biomassIncrementForAllocation > 0)
        {
            /// aboveground shoot allocation
            community.allPlants.at(cohortindex)->shootBiomassGreenLeaves += biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationShoot;
            community.allPlants.at(cohortindex)->shootBiomass = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves + community.allPlants.at(cohortindex)->shootBiomassBrownLeaves;
            community.allPlants.at(cohortindex)->shootCarbonGreenLeaves = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->shootCarbon = community.allPlants.at(cohortindex)->shootBiomass * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->shootNitrogenGreenLeaves = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
            community.allPlants.at(cohortindex)->shootNitrogen = community.allPlants.at(cohortindex)->shootNitrogenGreenLeaves + community.allPlants.at(cohortindex)->shootNitrogenBrownLeaves;

            /// belowground root allocation
            community.allPlants.at(cohortindex)->rootBiomass += biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationRoot;
            community.allPlants.at(cohortindex)->rootCarbon = community.allPlants.at(cohortindex)->rootBiomass * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->rootNitrogen = community.allPlants.at(cohortindex)->rootCarbon / parameter.plantCNRatioRoots[pft];

            /// plant biomass update
            community.allPlants.at(cohortindex)->plantBiomass = community.allPlants.at(cohortindex)->shootBiomass + community.allPlants.at(cohortindex)->rootBiomass;
            community.allPlants.at(cohortindex)->plantCarbon = community.allPlants.at(cohortindex)->plantBiomass * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->plantNitrogen = community.allPlants.at(cohortindex)->shootNitrogen + community.allPlants.at(cohortindex)->rootNitrogen;

            /// allocation to recruitment biomass pool for seed production
            community.allPlants.at(cohortindex)->recruitmentBiomass += biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationRecruitment;
            community.allPlants.at(cohortindex)->recruitmentCarbon = community.allPlants.at(cohortindex)->recruitmentBiomass * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->recruitmentNitrogen = community.allPlants.at(cohortindex)->recruitmentCarbon / parameter.plantCNRatioSeeds[pft];

            /// allocation to exudates
            community.allPlants.at(cohortindex)->exudationBiomass = biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationExudation;
            community.allPlants.at(cohortindex)->exudationCarbon = community.allPlants.at(cohortindex)->exudationBiomass * CARBON_CONTENT_ODM;
            community.allPlants.at(cohortindex)->exudationNitrogen = community.allPlants.at(cohortindex)->exudationCarbon / parameter.plantCNRatioExudates[pft];

            // transfer exudation to interfaces to external soil module
            if (parameter.useExternalSoilModule_BODIUM)
            {
                std::for_each(soil.couplingInterface_rootExudatesCarbon.begin(), soil.couplingInterface_rootExudatesCarbon.begin() + community.allPlants.at(cohortindex)->numberOfSoilLayersRooting, [&community, cohortindex](double &x)
                              { x += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->exudationCarbon; });
                std::for_each(soil.couplingInterface_rootExudatesNitrogen.begin(), soil.couplingInterface_rootExudatesNitrogen.begin() + community.allPlants.at(cohortindex)->numberOfSoilLayersRooting, [&community, cohortindex](double &x)
                              { x += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->exudationNitrogen; });
            }

            // to be added: transfer exudation biomass to soil pool
            // soil.CPool_Soil_active += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->exudationBiomass;
            // community.allPlants.at(cohortindex)->exudationBiomass = 0;
        }
    }
}

void GROWTH::doPlantGrowthInSizeAndAging(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, SOIL soil)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        community.allPlants.at(cohortindex)->age += 1;

        double heightBeforeGrowth = community.allPlants.at(cohortindex)->height; // possibly not matching widthBeforeGrowth in case of mowing
        double widthBeforeGrowth = community.allPlants.at(cohortindex)->width;
        double heightMatchingWidthByRatio = allometry.heightFromWidthByRatio(widthBeforeGrowth, parameter.plantHeightToWidthRatio[pft]);

        if (heightBeforeGrowth < heightMatchingWidthByRatio)
        { /// regrowing (e.g. after mowing): all biomass increment is put only into height growth, until height-width-ratio is reached again
            double newHeightByGrowthOnlyInHeight = allometry.heightFromShootBiomassWidthShootCorrection(utils, community.allPlants.at(cohortindex)->shootBiomass, widthBeforeGrowth, parameter.plantShootCorrectionFactor[pft]);
            /// update new height based on biomass increment
            community.allPlants.at(cohortindex)->height = newHeightByGrowthOnlyInHeight;
        }
        else
        { /// normal geometry calculation can be used again: growth in width and height proportionally
            double newWidthByGrowthInHeightAndWidth = allometry.widthFromShootBiomassByRatioAndShootCorrection(utils, community.allPlants.at(cohortindex)->shootBiomass, parameter.plantHeightToWidthRatio[pft], parameter.plantShootCorrectionFactor[pft]);
            double newHeightByGrowthInHeightAndWidth = allometry.heightFromWidthByRatio(newWidthByGrowthInHeightAndWidth, parameter.plantHeightToWidthRatio[pft]);

            community.allPlants.at(cohortindex)->width = newWidthByGrowthInHeightAndWidth;
            community.allPlants.at(cohortindex)->height = newHeightByGrowthInHeightAndWidth;
            community.allPlants.at(cohortindex)->coveredArea = allometry.areaFromWidth(newWidthByGrowthInHeightAndWidth);
        }

        /// update all other geometric size variables of the plants
        community.allPlants.at(cohortindex)->rootingDepth = allometry.rootDepthFromRootBiomassParametersRatioAndShootCorrection(utils, community.allPlants.at(cohortindex)->rootBiomass, parameter.plantRootDepthParamIntercept[pft], parameter.plantRootDepthParamExponent[pft], parameter.plantShootRootRatio[pft], parameter.plantShootCorrectionFactor[pft]);
        community.allPlants.at(cohortindex)->numberOfSoilLayersRooting = allometry.calculateNumberOfRootingSoillayer(parameter.soilLayerWidth, community.allPlants.at(cohortindex)->rootingDepth);

        if (community.allPlants.at(cohortindex)->numberOfSoilLayersRooting > parameter.numberOfSoilLayers)
        {
            community.allPlants.at(cohortindex)->numberOfSoilLayersRooting = parameter.numberOfSoilLayers;
        }

        community.allPlants.at(cohortindex)->laiGreen =
            allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortindex)->shootBiomassGreenLeaves, community.allPlants.at(cohortindex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
        community.allPlants.at(cohortindex)->laiBrown =
            allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortindex)->shootBiomassBrownLeaves, community.allPlants.at(cohortindex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
        community.allPlants.at(cohortindex)->lai = community.allPlants.at(cohortindex)->laiBrown + community.allPlants.at(cohortindex)->laiGreen;

        // state variable updates for soil evaporation
        community.greenleafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->amount * community.allPlants[cohortindex]->laiGreen * community.allPlants[cohortindex]->coveredArea/SIMULATION_AREA;
        community.totalLeafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->lai * community.allPlants[cohortindex]->coveredArea * community.allPlants[cohortindex]->amount/SIMULATION_AREA;
    }
}

/**
 * @brief Adjusts allocation rates for NPP among plant growth components
 *        based on plant height and specified parameters.
 *
 * This function iterates through each plant cohort in the community,
 * calculating and adjusting the allocation rates for shoot, root,
 * recruitment, and exudates. The adjustments are made based on the
 * height of the plant relative to its maturity height.
 *
 * @param utils A reference to a UTILS object that provides utility functions,
 *              including error handling.
 * @param parameter A PARAMETER object that contains the configuration settings
 *                  used for determining allocation rates.
 * @param community A reference to a COMMUNITY object that contains all the
 *                  plant cohorts and their associated data.
 *
 * This function performs the following operations for each plant cohort:
 * - Calculates the proportionality factor for allocation distribution to plant parts.
 * - Adjusts the allocation rates based on whether the plant is mature or regrowing.
 * - Validates that the sum of allocation rates for shoot, root, recruitment,
 *   and exudates equals 1, raising an error if the sum is inconsistent.
 *
 * @warning This function assumes that the parameters and community data
 *          are properly initialized and valid.
 * @throws std::runtime_error if the sum of allocation rates does not equal 1,
 *         indicating an inconsistency in allocation rates.
 */
void GROWTH::adjustAllocationRates(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    double proportionOfNppAllocationToPlantGrowthToShoot;
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int pft = community.allPlants.at(cohortindex)->pft;
        proportionOfNppAllocationToPlantGrowthToShoot = calculateProportionalityFactorForAllocationDistributionToPlantParts(parameter, community, cohortindex, pft);

        // adjusting allocations depending on plant height, possibly changing back and forth due to mowing
        if (community.allPlants.at(cohortindex)->height >= parameter.maturityHeights[pft])
        {
            adjustAllocationRatesForMaturePlants(parameter, community, cohortindex, pft, proportionOfNppAllocationToPlantGrowthToShoot);
        }
        else
        {
            adjustAllocationRatesForRegrowingPlants(parameter, community, cohortindex, pft, proportionOfNppAllocationToPlantGrowthToShoot);
        }

        if (abs(community.allPlants.at(cohortindex)->nppAllocationShoot + community.allPlants.at(cohortindex)->nppAllocationRoot + community.allPlants.at(cohortindex)->nppAllocationRecruitment +
                community.allPlants.at(cohortindex)->nppAllocationExudation - 1) > TOLERANCE)
        {
            utils.handleError("Sum of alloction rates (shoot, root, recruitment, exudates does not equal one as required!");
        }
    }
}

/**
 * @brief Calculates the proportionality factor for allocation distribution
 *        between shoot and root biomass for a specific plant cohort.
 *
 * This function determines the proportion of Gross Primary Production (GPP)
 * that should be allocated to shoot biomass based on the shoot-root ratio
 * defined in the parameters. The calculation varies depending on whether
 * static allocation rates are used or dynamic rates that adjust the
 * shoot-root ratio towards a specified target.
 *
 * @param parameter A PARAMETER object containing settings for the shoot-root
 *                  allocation ratios and other configuration values.
 * @param community A reference to a COMMUNITY object that contains all the
 *                  plant cohorts and their associated data.
 * @param cohortindex The index of the plant cohort within the community to
 *                    which the calculation is applied.
 * @param pft The plant functional type (PFT) of the current cohort, used to
 *            retrieve the shoot-root ratio.
 *
 * @return The proportion of GPP to be allocated to shoot biomass,
 *         constrained to be between 0 and 1.
 *
 * @note If static shoot-root allocation rates are used, the function returns
 *       the ratio of shoot to total biomass directly. Otherwise, it computes
 *       the dynamic allocation based on the current GPP and the existing
 *       biomass of shoots and roots.
 */
double GROWTH::calculateProportionalityFactorForAllocationDistributionToPlantParts(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft)
{
    double shootRootRatio = parameter.plantShootRootRatio[pft];
    double gpp = community.allPlants.at(cohortindex)->gpp;

    if (parameter.useStaticShootRootAllocationRates)
    {
        return (shootRootRatio / (shootRootRatio + 1));
    }
    else
    {
        // enforce shoot-root ratio
        // distribute gpp such that actual shoot_root ratio converge towards par value
        // calc proportion of Gpp that must go into Shoot in order to restore shoot-root ratio
        // use proportionOfGppToShoot as proportionOfNppAllocationToPlantGrowthToShoot
        double proportionOfGppToShoot = (shootRootRatio * (community.allPlants.at(cohortindex)->rootBiomass + gpp) - community.allPlants.at(cohortindex)->shootBiomass) / ((1 + shootRootRatio) * gpp);
        (proportionOfGppToShoot > 1) ? (proportionOfGppToShoot = 1) : ((proportionOfGppToShoot < 0) ? (proportionOfGppToShoot = 0) : (proportionOfGppToShoot = proportionOfGppToShoot));
        return (proportionOfGppToShoot);
    }
}

/**
 * @brief Adjusts allocation rates for Net Primary Production (NPP)
 *        for mature plants based on specified parameters.
 *
 * This function calculates and assigns the allocation of NPP to the
 * shoot, root, exudates, and seed production for a mature plant cohort.
 * It distributes the NPP according to the proportions defined in the
 * parameters, adjusting the allocation rates based on the input
 * proportion of NPP allocation to plant growth directed to shoot.
 *
 * @param parameter A PARAMETER object containing settings for NPP
 *                  allocation ratios for different plant functional types (PFTs).
 * @param community A reference to a COMMUNITY object that contains all the
 *                  plant cohorts and their associated data.
 * @param cohortindex The index of the plant cohort within the community
 *                    to which the allocation rates are applied.
 * @param pft The plant functional type (PFT) of the current cohort, used
 *            to retrieve the appropriate allocation settings.
 * @param proportionOfNppAllocationToPlantGrowthToShoot The proportion of
 *        NPP allocated to the shoot relative to the total NPP for growth.
 *
 * This function performs the following operations:
 * - Allocates a portion of NPP to shoot biomass based on the proportion of
 *   NPP directed to plant growth.
 * - Allocates the remaining portion of NPP to root biomass.
 * - Sets the exudation allocation based on parameter values.
 * - Calculates the recruitment allocation as the remaining NPP after
 *   allocating to growth and exudation.
 */
void GROWTH::adjustAllocationRatesForMaturePlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot)
{
    /// NPP distributed to shoot, root, exudates with remaining part allocated to seed production
    community.allPlants.at(cohortindex)->nppAllocationShoot =
        parameter.plantNppAllocationGrowth[pft] * proportionOfNppAllocationToPlantGrowthToShoot;
    community.allPlants.at(cohortindex)->nppAllocationRoot =
        parameter.plantNppAllocationGrowth[pft] * (1 - proportionOfNppAllocationToPlantGrowthToShoot);
    community.allPlants.at(cohortindex)->nppAllocationExudation = parameter.plantNppAllocationExudation[pft];
    community.allPlants.at(cohortindex)->nppAllocationRecruitment =
        1 - parameter.plantNppAllocationGrowth[pft] - parameter.plantNppAllocationExudation[pft];
}

/**
 * @brief Adjusts allocation rates for Net Primary Production (NPP)
 *        for regrowing plants based on specified parameters.
 *
 * This function calculates and assigns the allocation of NPP to the
 * shoot, root, and exudates for a regrowing plant cohort. Since
 * recruitment is not possible during this phase, the function sets
 * the recruitment allocation to zero and distributes NPP according to
 * the input proportion of NPP allocation to plant growth directed to
 * shoot.
 *
 * @param parameter A PARAMETER object containing settings for NPP
 *                  allocation ratios for different plant functional types (PFTs).
 * @param community A reference to a COMMUNITY object that contains all the
 *                  plant cohorts and their associated data.
 * @param cohortindex The index of the plant cohort within the community
 *                    to which the allocation rates are applied.
 * @param pft The plant functional type (PFT) of the current cohort, used
 *            to retrieve the appropriate allocation settings.
 * @param proportionOfNppAllocationToPlantGrowthToShoot The proportion of
 *        NPP allocated to the shoot relative to the total NPP for growth.
 *
 * This function performs the following operations:
 * - Allocates a portion of NPP to shoot biomass based on the proportion of
 *   NPP directed to plant growth.
 * - Allocates the remaining portion of NPP to root biomass.
 * - Sets the exudation allocation based on parameter values.
 * - Sets the recruitment allocation to zero, as no recruitment is possible
 *   during the regrowing phase.
 */
void GROWTH::adjustAllocationRatesForRegrowingPlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot)
{
    /// NPP distributed to shoot, root and exudates (no recruitment possible)
    community.allPlants.at(cohortindex)->nppAllocationShoot =
        (1 - parameter.plantNppAllocationExudation[pft]) * proportionOfNppAllocationToPlantGrowthToShoot;
    community.allPlants.at(cohortindex)->nppAllocationRoot =
        (1 - parameter.plantNppAllocationExudation[pft]) * (1 - proportionOfNppAllocationToPlantGrowthToShoot);
    community.allPlants.at(cohortindex)->nppAllocationRecruitment = 0;
    community.allPlants.at(cohortindex)->nppAllocationExudation = parameter.plantNppAllocationExudation[pft];
}