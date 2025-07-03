#include "mortality.h"

MORTALITY::MORTALITY() {};
MORTALITY::~MORTALITY() {};

/**
 * @brief Main mortality function for managing plant mortality within the community.
 *
 * This function iterates over all plants in the community and applies various
 * mortality processes including senescence, litter fall, thinning, and basic
 * mortality calculations based on defined parameters.
 *
 * @param parameter Reference to the PARAMETER object containing simulation settings.
 * @param community Reference to the COMMUNITY object representing the plant community.
 * @param utils Utility functions used for calculations and operations.
 *
 * @note The function assumes that the `community` contains valid plants in the `allPlants`
 *       vector. It performs checks to ensure that plant cohorts in the vector still have
 *       a minimum of one plant after applying the mortality processes.
 */
void MORTALITY::doPlantMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL soil)
{
   for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
   {
      int pft = community.allPlants[cohortIndex]->pft;

      // 1. Leaf and root senescence and litter fall
      doSenescenceAndLitterFall(utils, parameter, community, allometry, growth, interaction, soil, cohortIndex, pft);

      // 2. Crowding mortality
      if (parameter.crowdingMortalityActivated)
      {
         community.randomNumberIndex++;
         doPlantCrowding(parameter, utils, soil, community, cohortIndex, pft);
      }

      // 3. Basic mortality
      community.randomNumberIndex++;
      doBasicMortality(parameter, utils, soil, community, cohortIndex, pft);
   }

   // 4. Delete cohorts if no more plants are alive
   community.checkPlantsAreAliveInCommunity(utils);

   // 5. Update number of cohorts in allPlants-vector
   community.totalNumberOfCohortsInCommunity = community.allPlants.size();
}

/* Leaf and root senescence and litter fall */
void MORTALITY::doSenescenceAndLitterFall(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL soil, int cohortIndex, int pft)
{
   /// Leaf senescence
   double browningLeafBiomass = doLeafSenescence(community, parameter, growth, interaction, cohortIndex, pft);
   doNitrogenRelocation(utils, parameter, community, browningLeafBiomass, cohortIndex, pft);

   // Litter fall of senescent leaves & transfer to surface litter pool
   doLeafLitterFall(utils, community, allometry, parameter, soil, cohortIndex, pft);

   // Root senescence
   doRootSenescenceAndLitterFall(community, parameter, soil, cohortIndex, pft);
}

double MORTALITY::doLeafSenescence(COMMUNITY &community, PARAMETER parameter, GROWTH growth, INTERACTION interaction, int cohortIndex, int pft)
{
   double effectOfDayTimeTemperature = growth.calculateEffectOfAirTemperatureOnGPP(interaction.dayTimeAirTemperature);
   double browningLeafBiomass = effectOfDayTimeTemperature * (community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves / parameter.leafLifeSpan[pft]); // to be added: effect of community.allPlants.at(cohortIndex)->limitingFactorGppWater

   community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves += browningLeafBiomass;
   community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves -= browningLeafBiomass;
   // community.allPlants.at(cohortIndex)->shootBiomass remains unchanged here

   community.allPlants.at(cohortIndex)->shootCarbonBrownLeaves = community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves * carbonContentOdm;
   community.allPlants.at(cohortIndex)->shootCarbonGreenLeaves = community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves * carbonContentOdm;

   community.allPlants.at(cohortIndex)->shootNitrogenBrownLeaves = community.allPlants.at(cohortIndex)->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
   community.allPlants.at(cohortIndex)->shootNitrogenGreenLeaves = community.allPlants.at(cohortIndex)->shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];

   return (browningLeafBiomass);
}

void MORTALITY::doLeafLitterFall(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL soil, int cohortIndex, int pft)
{
   if (community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves > 0.0)
   {
      double fractionLeavesFalling = parameter.brownBiomassFractionFalling;
      (parameter.day % 365 == 0) ? (fractionLeavesFalling = 1) : (fractionLeavesFalling = fractionLeavesFalling);

      if (fractionLeavesFalling > 0)
      {
         double fallingLeafBiomass = fractionLeavesFalling * community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;

         community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves -= fallingLeafBiomass;
         community.allPlants.at(cohortIndex)->shootBiomass = community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves + community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;

         community.allPlants[cohortIndex]->shootCarbonBrownLeaves = community.allPlants[cohortIndex]->shootBiomassBrownLeaves * carbonContentOdm;
         community.allPlants[cohortIndex]->shootCarbon = community.allPlants[cohortIndex]->shootCarbonGreenLeaves + community.allPlants[cohortIndex]->shootCarbonBrownLeaves;

         community.allPlants[cohortIndex]->shootNitrogenBrownLeaves = community.allPlants[cohortIndex]->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
         community.allPlants[cohortIndex]->shootNitrogen = community.allPlants[cohortIndex]->shootNitrogenGreenLeaves + community.allPlants[cohortIndex]->shootNitrogenBrownLeaves;

         // community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves remains unchanged here

         soil.transferDyingPlantPartsToLitterPools(parameter, community.allPlants.at(cohortIndex)->amount, fallingLeafBiomass, 1, pft);
         updatePlantSize(utils, community, allometry, parameter, fractionLeavesFalling, cohortIndex, pft);
      }
   }
}

void MORTALITY::updatePlantSize(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int fractionLeavesFalling, int cohortIndex, int pft)
{
   // calculation of width & coveredArea only if fractionFalling < 1
   // width shall not be updated when all brown biomass falls off at once, but only height
   if (fractionLeavesFalling == 1)
   {
      community.allPlants.at(cohortIndex)->height = allometry.heightFromShootBiomassWidthShootCorrection(utils, community.allPlants.at(cohortIndex)->shootBiomass, community.allPlants.at(cohortIndex)->width,
                                                                                                         parameter.plantShootCorrectionFactor[pft]);
   }
   else
   {
      community.allPlants.at(cohortIndex)->width = allometry.widthFromShootBiomassByRatioAndShootCorrection(utils, community.allPlants.at(cohortIndex)->shootBiomass, parameter.plantHeightToWidthRatio[pft],
                                                                                                            parameter.plantShootCorrectionFactor[pft]);

      community.allPlants.at(cohortIndex)->height = allometry.heightFromWidthByRatio(community.allPlants.at(cohortIndex)->width, parameter.plantHeightToWidthRatio[pft]);
      community.allPlants.at(cohortIndex)->coveredArea = allometry.areaFromWidth(community.allPlants.at(cohortIndex)->width);
   }

   community.allPlants.at(cohortIndex)->laiGreen =
       allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, community.allPlants.at(cohortIndex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
   community.allPlants.at(cohortIndex)->laiBrown =
       allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, community.allPlants.at(cohortIndex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
   community.allPlants.at(cohortIndex)->lai = community.allPlants.at(cohortIndex)->laiBrown + community.allPlants.at(cohortIndex)->laiGreen;
}

void MORTALITY::doRootSenescenceAndLitterFall(COMMUNITY &community, PARAMETER parameter, SOIL soil, int cohortIndex, int pft)
{
   double dyingRootBiomass = community.allPlants.at(cohortIndex)->rootBiomass * (1.0 / parameter.rootLifeSpan[pft]);

   soil.transferDyingPlantPartsToLitterPools(parameter, community.allPlants.at(cohortIndex)->amount, dyingRootBiomass, 2, pft);
   community.allPlants.at(cohortIndex)->rootBiomass -= dyingRootBiomass;
   community.allPlants.at(cohortIndex)->rootCarbon = community.allPlants.at(cohortIndex)->rootBiomass * carbonContentOdm;
   community.allPlants.at(cohortIndex)->rootNitrogen = community.allPlants.at(cohortIndex)->rootCarbon / parameter.plantCNRatioRoots[pft];
}

void MORTALITY::doNitrogenRelocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, double browningLeafBiomass, int cohortIndex, int pft)
{
   double carbonContentBrowningLeaves = browningLeafBiomass * carbonContentOdm;
   double previousNitrogenContentBrowningLeaves = carbonContentBrowningLeaves / parameter.plantCNRatioGreenLeaves[pft];
   double currentNitrogenContentBrowningLeaves = carbonContentBrowningLeaves / parameter.plantCNRatioBrownLeaves[pft];

   double relocatedNitrogen = previousNitrogenContentBrowningLeaves - currentNitrogenContentBrowningLeaves;
   community.allPlants[cohortIndex]->nitrogenSurplus += relocatedNitrogen;
   community.allPlants[cohortIndex]->shootNitrogen -= relocatedNitrogen;

   if (abs((community.allPlants.at(cohortIndex)->shootNitrogenBrownLeaves + community.allPlants.at(cohortIndex)->shootNitrogenGreenLeaves) - community.allPlants.at(cohortIndex)->shootNitrogen) > tolerance)
   {
      utils.handleError("Error (mortality): relocated nitrogen due to senescence does not match with CN ratios.");
   }
}

/* Plant mortality due to thinning of the community */
// * @cite Concept of crowding mortality is derived from the forest model FORMIND (www.formind.org)
void MORTALITY::doPlantCrowding(PARAMETER parameter, UTILS utils, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft)
{
   if (community.allPlants[cohortIndex]->amount > 0)
   {
      if (community.coveredAreaOfAllPlants > 1.0)
      {
         std::uniform_real_distribution<> dis(0.0, 1.0);
         std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
         double randomNumber = dis(gen);

         double amountOfTooManyPlants = community.allPlants[cohortIndex]->amount * (1.0 - (1.0 / community.coveredAreaOfAllPlants));
         double letAnotherPlantDy = amountOfTooManyPlants - int(amountOfTooManyPlants);
         if (randomNumber <= letAnotherPlantDy)
         {
            amountOfTooManyPlants += 1.0;
         }

         if (community.allPlants[cohortIndex]->amount - amountOfTooManyPlants >= 0)
         {
            soil.transferDyingPlantPartsToLitterPools(parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, 0, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, 1, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->rootBiomass, 2, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->recruitmentBiomass, 3, pft);
            community.allPlants[cohortIndex]->amount -= amountOfTooManyPlants;
         }
         else
         {
            utils.handleError("Error (mortality): more plants shall die than are available in the cohort.");
         }
      }
   }
   else
   {
      utils.handleError("Error (mortality): no more plants available in the cohort to die.");
   }
}

/**
 * @brief Applies basic mortality to a plant in the community based on its intrinsic mortality rate.
 *
 * This function computes the probability of death for a given plant and
 * decreases the plant amount in the respective cohort if the
 * random number generated is less than or equal to the calculated mortality probability.
 *
 * @param parameter Reference to the PARAMETER object containing simulation settings.
 * @param utils Utility functions used for calculations and operations.
 * @param community Reference to the COMMUNITY object representing the plant community.
 * @param cohortIndex The index of the plant cohort within the community's list of plants.
 * @param pft The plant functional type (PFT) index of the plant being assessed.
 *
 * @note The function retrieves the mortality probability for the specified plant and
 *       checks if the plant amount in the cohort is greater than zero before decrementing it.
 *       If the amount is zero, an error is reported using the utility function.
 */
void MORTALITY::doBasicMortality(PARAMETER parameter, UTILS utils, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft)
{
   double mortalityProbability = getPlantMortalityProbability(parameter, community, cohortIndex, pft);

   for (int plantIndex = 0; plantIndex < community.allPlants[cohortIndex]->amount; plantIndex++)
   {
      if (community.allPlants[cohortIndex]->amount > 0)
      {
         std::uniform_real_distribution<> dis(0.0, 1.0);
         std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
         double randomNumber = dis(gen);

         /* let plants die according to the mortality probability */
         if (randomNumber <= mortalityProbability)
         {
            soil.transferDyingPlantPartsToLitterPools(parameter, 1, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, 0, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, 1, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, 1, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, 1, community.allPlants.at(cohortIndex)->rootBiomass, 2, pft);
            soil.transferDyingPlantPartsToLitterPools(parameter, 1, community.allPlants.at(cohortIndex)->recruitmentBiomass, 3, pft);
            community.allPlants[cohortIndex]->amount -= 1;
         }
      }
      else
      {
         utils.handleError("Error (mortality): no more plants available in the cohort to die.");
      }
   }
}

/**
 * @brief Retrieves the mortality probability for a plant based on its age and functional type.
 *
 * This function checks if the plant cohort is adult or a seedling and returns
 * the appropriate mortality probability from the parameter settings for the specified
 * plant functional type (PFT).
 *
 * @param parameter Reference to the PARAMETER object containing the mortality rates.
 * @param community Reference to the COMMUNITY object representing the plant community.
 * @param cohortIndex The index of the plant cohort within the community's list of plants.
 * @param pft The plant functional type (PFT) index of the plant cohort.
 *
 * @return double The mortality probability for the specified plant cohort.
 *
 * @note This function uses the boolean flag `isAdult` of the plant cohort to determine
 *       which mortality probability to return. If the plant is adult, it returns the
 *       adult mortality probability; otherwise, it returns the seedling mortality probability.
 */
double MORTALITY::getPlantMortalityProbability(PARAMETER parameter, COMMUNITY community, int cohortIndex, int pft)
{
   if (community.allPlants[cohortIndex]->age >= parameter.maturityAges[pft])
   {
      if (parameter.plantLifeSpan[pft] == "annual" && community.allPlants[cohortIndex]->age > 365)
      {
         return (1.0);
      }
      else
      {
         return (parameter.plantMortalityProbability[pft]);
      }
   }
   else
   {
      return (parameter.seedlingMortalityProbability[pft]);
   }
}
