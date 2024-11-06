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
void MORTALITY::doPlantMortality(PARAMETER parameter, COMMUNITY &community, UTILS utils)
{
   for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
   {
      int pft = community.allPlants[cohortIndex]->pft;

      // 1. Leaf and root senescence and litter fall
      // TODO: add crowding and senescence / litter fall
      doSenescenceAndLitterFall();

      // 2. Crowding mortality
      community.randomNumberIndex++;
      doThinning();

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
void MORTALITY::doSenescenceAndLitterFall()
{
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
   if (community.allPlants[cohortIndex]->isAdult)
   {
      return (parameter.plantMortalityRates[pft]);
   }
   else
   {
      return (parameter.seedlingMortalityRates[pft]);
   }
}
