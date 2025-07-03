#include "growth.h"

GROWTH::GROWTH() {};
GROWTH::~GROWTH() {};

/**
 * @brief Main function of plant growth
 * @cite Concept of plant NPP based on the carbon balance of photosynthesis and respiration
 *       based on the forest model FORMIND (www.formind.org)
 */
void GROWTH::doPlantGrowth(UTILS utils, PARAMETER parameter, COMMUNITY &community, INTERACTION interaction, ALLOMETRY allometry, SOIL &soil)
{
   /* Plant GPP (gross primary productivity) */
   doPlantPhotosynthesis(parameter, community, interaction);

   /* to be added: Limitation of plant GPP by unfavorable soil water conditions */
   // soil.doPlantGPPLimitationBySoilWaterConditions();

   /* Plant respiration */
   doPlantRespiration(community, parameter, interaction);

   /* Plant NPP (net primary productivity) */
   calculatePlantNPPFromGPPAndRespiration(community, parameter);

   /* to be added: Limitation of plant NPP by unfavorable soil nitrogen conditions */
   // soil.doPlantNPPLimitationBySoilNitrogenConditions();

   /* Plant allocation of NPP and according C and N parts */
   adjustAllocationRates(utils, parameter, community);
   doPlantNPPAllocation(utils, parameter, community, soil);

   /* Plant growth in size based on NPP allocation and aging */
   doPlantGrowthInSizeAndAging(utils, parameter, community, allometry);
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
void GROWTH::doPlantPhotosynthesis(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction)
{
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int pft = community.allPlants.at(cohortindex)->pft;
      double plantLAI = community.allPlants.at(cohortindex)->laiGreen;
      double plantCoveredArea = community.allPlants.at(cohortindex)->coveredArea;

      double plantRadiation = (24.0 / interaction.dayLength) * community.allPlants.at(cohortindex)->availableRadiation; // correct mean daily radiation by daylength hours for photosynthesis
      double effectOfDayTimeTemperature = calculateEffectOfAirTemperatureOnGPP(interaction.dayTimeAirTemperature);

      community.allPlants.at(cohortindex)->gpp = effectOfDayTimeTemperature * calculateGPPOfPlant(parameter, pft, plantLAI, plantCoveredArea, plantRadiation, interaction.dayLength);
   }
}

/**
 * @brief Calculates the daily gross primary productivity (GPP) for a single plant.
 *
 * This function estimates the amount of organic dry matter (ODM) produced via
 * photosynthesis based on radiation, plant traits (e.g. LAI, covered area),
 * and environmental conditions such as day length.
 *
 * The calculation involves:
 * - Estimating CO₂ uptake per second and square meter of leaf area.
 * - Converting CO₂ to organic dry matter using stoichiometric factors.
 * - Scaling uptake from square meters to the plant's actual covered area.
 * - Integrating uptake over the length of the day (in seconds).
 *
 * If plant radiation is zero, the function immediately returns zero.
 *
 * @param parameter             Struct with plant functional type-specific parameters.
 * @param pft                   Index of the plant functional type (PFT).
 * @param plantLAI              Leaf area index of the plant.
 * @param plantCoveredArea      Ground area covered by the plant (in cm²).
 * @param plantRadiation        Available radiation for the plant (in µmol(photons)/m²).
 * @param dayLength             Length of the day in hours.
 *
 * @return Gross primary productivity (GPP) in grams of organic dry matter (ODM) per day.
 *
 * @see calculateCO2UptakePerSecondAndSquareMeter()
 * @cite Concept of plant photosynthesis is based on the forest model FORMIND (www.formind.org)
 */
double GROWTH::calculateGPPOfPlant(PARAMETER parameter, int pft, double plantLAI, double plantCoveredArea, double plantRadiation, double dayLength)
{
   if (plantRadiation == 0)
   {
      return 0;
   }
   else
   {
      double CO2UptakePerSecondAndSquareMeter = calculateCO2UptakePerSecondAndSquareMeter(parameter, pft, plantRadiation, plantLAI);
      double OdmUptakePerSecondAndSquareMeter = CO2UptakePerSecondAndSquareMeter * CO2ConversionToOdm * molarMassOfCO2; // conversion from CO2 to Odm
      double OdmUptakePerSecondAndSquareCentimeter = OdmUptakePerSecondAndSquareMeter / (100.0 * 100.0);
      double secondsPerDay = dayLength * 60 * 60;                                                                  // scaling from seconds to day
      double plantPhotosynthesisPerDay = OdmUptakePerSecondAndSquareCentimeter * secondsPerDay * plantCoveredArea; // scaling to plant

      return plantPhotosynthesisPerDay; // g ODM per day and plant
   }
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
   const double alpha = parameter.initialSlopeOfLightResponseCurve.at(pft);
   const double k = parameter.lightExtinctionCoefficients.at(pft);
   const double pmax = parameter.maximumGrossLeafPhotosynthesisRate.at(pft);

   const double calcPart1 = alpha * k * plantRadiation;
   const double calcPart2 = pmax * (1 - lightTransmissionCoefficient);

   double CO2UptakePerSecondsAndSquareMeter = ((pmax / k) * log((calcPart1 + calcPart2) / (calcPart1 * exp(-k * plantLAI) + calcPart2)));
   return (CO2UptakePerSecondsAndSquareMeter);
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

   if (dayTimeAirTemperature > -5 && dayTimeAirTemperature <= 2)
   {
      reductionFactor = (0.02857 * dayTimeAirTemperature + 0.142);
   }

   if (dayTimeAirTemperature > 2 && dayTimeAirTemperature <= 10)
   {
      reductionFactor = (0.1 * dayTimeAirTemperature);
   }

   if (dayTimeAirTemperature > 10)
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
         community.allPlants.at(cohortindex)->shootCarbonGreenLeaves = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves * carbonContentOdm;
         community.allPlants.at(cohortindex)->shootCarbon = community.allPlants.at(cohortindex)->shootBiomass * carbonContentOdm;
         community.allPlants.at(cohortindex)->shootNitrogenGreenLeaves = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
         community.allPlants.at(cohortindex)->shootNitrogen = community.allPlants.at(cohortindex)->shootNitrogenGreenLeaves + community.allPlants.at(cohortindex)->shootNitrogenBrownLeaves;

         /// belowground root allocation
         community.allPlants.at(cohortindex)->rootBiomass += biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationRoot;
         community.allPlants.at(cohortindex)->rootCarbon = community.allPlants.at(cohortindex)->rootBiomass * carbonContentOdm;
         community.allPlants.at(cohortindex)->rootNitrogen = community.allPlants.at(cohortindex)->rootCarbon / parameter.plantCNRatioRoots[pft];

         /// plant biomass update
         community.allPlants.at(cohortindex)->plantBiomass = community.allPlants.at(cohortindex)->shootBiomass + community.allPlants.at(cohortindex)->rootBiomass;
         community.allPlants.at(cohortindex)->plantCarbon = community.allPlants.at(cohortindex)->plantBiomass * carbonContentOdm;
         community.allPlants.at(cohortindex)->plantNitrogen = community.allPlants.at(cohortindex)->shootNitrogen + community.allPlants.at(cohortindex)->rootNitrogen;

         /// allocation to recruitment biomass pool for seed production
         community.allPlants.at(cohortindex)->recruitmentBiomass += biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationRecruitment;
         community.allPlants.at(cohortindex)->recruitmentCarbon = community.allPlants.at(cohortindex)->recruitmentBiomass * carbonContentOdm;
         community.allPlants.at(cohortindex)->recruitmentNitrogen = community.allPlants.at(cohortindex)->recruitmentCarbon / parameter.plantCNRatioSeeds[pft];

         /// allocation to exudates
         community.allPlants.at(cohortindex)->exudationBiomass = biomassIncrementForAllocation * community.allPlants.at(cohortindex)->nppAllocationExudation;
         community.allPlants.at(cohortindex)->exudationCarbon = community.allPlants.at(cohortindex)->exudationBiomass * carbonContentOdm;
         community.allPlants.at(cohortindex)->exudationNitrogen = community.allPlants.at(cohortindex)->exudationCarbon / parameter.plantCNRatioExudates[pft];

         // to be added: transfer exudation biomass to soil pool
         // soil.CPool_Soil_active += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->exudationBiomass;
         // community.allPlants.at(cohortindex)->exudationBiomass = 0;
      }
   }
}

void GROWTH::doPlantGrowthInSizeAndAging(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry)
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
      community.allPlants.at(cohortindex)->numberOfSoilLayersRooting = std::ceil(community.allPlants.at(cohortindex)->rootingDepth / soilLayerWidth);
      community.allPlants.at(cohortindex)->laiGreen =
          allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortindex)->shootBiomassGreenLeaves, community.allPlants.at(cohortindex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
      community.allPlants.at(cohortindex)->laiBrown =
          allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortindex)->shootBiomassBrownLeaves, community.allPlants.at(cohortindex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
      community.allPlants.at(cohortindex)->lai = community.allPlants.at(cohortindex)->laiBrown + community.allPlants.at(cohortindex)->laiGreen;
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
              community.allPlants.at(cohortindex)->nppAllocationExudation - 1) > tolerance)
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