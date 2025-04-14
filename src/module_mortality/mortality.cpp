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
      // community.randomNumberIndex++;
      // TODO: decide weather to keep or remove crowding
      // doThinning();

      // 3. Basic mortality
      community.randomNumberIndex++;
      doBasicMortality(parameter, utils, community, cohortIndex, pft);
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
   double carbonContentBrowningLeaves = browningLeafBiomass * carbonContentOdm;
   // doNitrogenRelocation(utils, community, parameter,carbonContentBrowningLeaves);

   // Litter fall of senescent leaves & transfer to surface litter pool
   doLeafLitterFall(utils, community, allometry, parameter, soil, cohortIndex, pft);

   // Root senescence
   doRootSenescenceAndLitterFall(community, parameter, soil, cohortIndex, pft);
}

double MORTALITY::doLeafSenescence(COMMUNITY &community, PARAMETER parameter, GROWTH growth, INTERACTION interaction, int cohortIndex, int pft)
{
   // TODO: search for appropriate temperature and soil moisture functions (or others) for leaf aging
   // For now: same functions as for GPP is used (just for testing)
   double effectOfDayTimeTemperature = growth.calculateEffectOfAirTemperatureOnGPP(interaction.dayTimeAirTemperature);
   double browningLeafBiomass = effectOfDayTimeTemperature * (community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves / parameter.leafLifeSpan[pft]); // * (1.0 - community.allPlants.at(cohortIndex)->limitingFactorGppWater);
   community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves += browningLeafBiomass;
   community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves -= browningLeafBiomass;

   return (browningLeafBiomass);
}

void MORTALITY::doLeafLitterFall(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL soil, int cohortIndex, int pft)
{
   if (community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves > 0.0)
   {
      double fractionLeavesFalling = parameter.brownBiomassFractionFalling;
      (parameter.day % 365 == 0) ? (fractionLeavesFalling = 1) : (fractionLeavesFalling = fractionLeavesFalling);
      // TODO: add running date parallel to day to account for leap years, so that we do not have to use modulo 365 hard coded

      if (fractionLeavesFalling > 0)
      {
         double fallingLeafBiomass = fractionLeavesFalling * community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;
         community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves -= fallingLeafBiomass;
         community.allPlants.at(cohortIndex)->shootBiomass = community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves + community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;

         //  double carbonContentFallingBiomass = fallingLeafBiomass * carbonContentOdm;
         //  double nitrogenContentFallingBiomass = carbonContentFallingBiomass / parameter.plantCNRatioBrownLeaves[pft];
         //  community.allPlants.at(cohortIndex)->nitrogenContentShoot -= nitrogenContentFallingBiomass;
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
   // double carbonContentDyingRootBiomass = dyingRootBiomass * carbonContentOdm;
   // double nitrogenContentDyingRootBiomass = carbonContentDyingRootBiomass / parameter.plantCNRatioRoots[pft];

   soil.transferDyingPlantPartsToLitterPools(parameter, community.allPlants.at(cohortIndex)->amount, dyingRootBiomass, 2, pft);

   /* TODO: add landtrans code of Matthes
   if (par.externalLandtransSoilModel == 1)
   {
      calcDyingRootPerLayerForLandtrans(community.allPlants.at(cohortIndex)->amount, dyingRootBiomass, nitrogenContentDyingRootBiomass, community.allPlants.at(cohortIndex)->numberOfSoilLayersRooting);
   }*/

   community.allPlants.at(cohortIndex)->rootBiomass -= dyingRootBiomass;
   // community.allPlants.at(cohortIndex)->nitrogenContentRoot -= nitrogenContentDyingRootBiomass;
}

void MORTALITY::doNitrogenRelocation(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{

   // double startNitrogenContentBrowning = carbonContentBrowning / parameter.cnRatioGreenLeaves[pft];
   // double endNitrogenContentBrowning = carbonContentBrowning / parameter.cnRatioBrownLeaves[pft];
   /*double relocatedNitrogen = startNitrogenContentBrowning - endNitrogenContentBrowning;
   plant->nitrogenSurplus += relocatedNitrogen;
   plant->nitrogenContentShoot -= relocatedNitrogen;

   double multiplier = 1000;
   nitrogenBalance = plant->nitrogenContentShoot -
                     plant->biomassGreen * ODM_TO_C / par.cnRatioGreenLeaves[pft] -
                     plant->biomassBrown * ODM_TO_C / par.cnRatioBrownLeaves[pft];
   if (abs(nitrogenBalance) > multiplier * tolerance)
   {
      std::cerr << "Wrong nitrogen amount in plant shoot after browning!" << std::endl;
      std::cerr << "N difference 'shoot' - 'biomass': " << nitrogenBalance << std::endl;
      std::cerr << "pft: " << pft << std::endl;
      std::cerr << "timestep: " << parameter.day << std::endl;
   }
   */
}

/* Plant mortality due to thinning of the community */
void MORTALITY::doThinning()
{
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
void MORTALITY::doBasicMortality(PARAMETER parameter, UTILS utils, COMMUNITY &community, int cohortIndex, int pft)
{
   double mortalityProbability = getPlantMortalityProbability(parameter, community, cohortIndex, pft);

   std::uniform_real_distribution<> dis(0.0, 1.0);
   std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
   double randomNumber = dis(gen);

   /* let plants die according to the mortality probability */
   if (randomNumber <= mortalityProbability)
   {
      if (community.allPlants[cohortIndex]->amount > 0)
      {
         community.allPlants[cohortIndex]->amount -= 1;
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
   // TODO: remove seedling mortality & maturityAges and add environmental conditions to germination rate
   /// if (community.allPlants[cohortIndex]->age >= parameter.maturityAges[pft])
   //{
   if (parameter.plantLifeSpan[pft] == "annual" && community.allPlants[cohortIndex]->age > 365)
   {
      return (1.0);
   }
   else
   {
      return (parameter.plantMortalityProbability[pft]);
   }
   /*}
   else
   {
      return (parameter.seedlingMortalityProbability[pft]);
   }*/
}
