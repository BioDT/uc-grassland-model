#include "community.h"

COMMUNITY::COMMUNITY() {};
COMMUNITY::~COMMUNITY() {};

/**
 * @brief Checks if plant cohorts in the community vector are still alive and removes dead cohorts.
 *
 * This function iterates through all plant cohorts in the `allPlants` vector and checks
 * if each cohort's count is greater than zero. If a cohort's count is zero or negative,
 * it is considered dead. The indices of dead cohorts are stored and then removed
 * from the community vector.
 *
 * If any cohort has a negative count, an error is raised. Additionally,
 * if more cohorts are marked for deletion than exist in the community vector,
 * an error will also be triggered.
 *
 * @param utils A utility object used for error handling and reporting.
 */
void COMMUNITY::checkPlantsAreAliveInCommunity(UTILS utils)
{
   std::vector<int> idsOfDeadPlantCohorts;
   idsOfDeadPlantCohorts.clear();

   if (allPlants.size() > 0)
   {
      /* go through all plant cohorts in the community vector and save indices of dying cohorts */
      for (int cohortIndex = 0; cohortIndex < allPlants.size(); cohortIndex++)
      {
         if (allPlants[cohortIndex]->amount == 0)
         {
            idsOfDeadPlantCohorts.push_back(cohortIndex);
         }
         else if (allPlants[cohortIndex]->amount < 0)
         {
            utils.handleError("Error (allPlants vector): there is an invalid negative amount of plants within a cohort.");
         }
      }

      /* throw an error if more cohorts should die than existing in the community vector */
      if (idsOfDeadPlantCohorts.size() > allPlants.size())
      {
         utils.handleError("More plant cohorts shall be deleted than existing.");
      }
      else if (idsOfDeadPlantCohorts.size() > 0) /* delete the dying cohorts */
      {
         // reversed sorting of indicees to ensure a correct deleting of plants in community vector from back to front
         std::sort(idsOfDeadPlantCohorts.rbegin(), idsOfDeadPlantCohorts.rend());
         for (auto id : idsOfDeadPlantCohorts)
         {
            allPlants.erase(allPlants.begin() + id);
         }
      }
   }
}

/**
 * @brief Calculates aggregated state variables based on dynamic changes of the community vector.
 *
 * This function updates various state variables related to the plant community, including
 * PFT composition and total amount of plants. It iterates over all plant cohorts in the
 * `allPlants` vector, aggregating their amount into PFT-specific and community-wide totals values.
 *
 * After processing all cohorts, the function normalizes the PFT composition values to
 * reflect their proportions relative to the total amount of plants in the community.
 *
 * @param parameter A parameter object that provides information about the number of PFTs
 *                  (Plant Functional Types) in the simulation.
 */
void COMMUNITY::updateCommunityStateVariablesForOutput(PARAMETER parameter)
{
   if (allPlants.size() > 0)
   {
      for (int cohortindex = 0; cohortindex < allPlants.size(); cohortindex++)
      {
         // PFT-specific calculations
         pftComposition[allPlants[cohortindex]->pft] += allPlants[cohortindex]->amount;
         numberOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->amount;
         coveredAreaOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->coveredArea * allPlants[cohortindex]->amount;
         shootBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->shootBiomass * allPlants[cohortindex]->amount;
         greenShootBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->shootBiomassGreenLeaves * allPlants[cohortindex]->amount;
         brownShootBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->shootBiomassBrownLeaves * allPlants[cohortindex]->amount;

         if (allPlants.at(cohortindex)->height > parameter.clippingHeightOfBiomassMeasurement)
         {
            allPlants.at(cohortindex)->shootBiomassAboveClippingHeight =
                ((allPlants.at(cohortindex)->height - parameter.clippingHeightOfBiomassMeasurement) / allPlants.at(cohortindex)->height) * allPlants.at(cohortindex)->shootBiomass;
            clippedShootBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->shootBiomassAboveClippingHeight * allPlants[cohortindex]->amount;
         }

         rootBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->rootBiomass * allPlants[cohortindex]->amount;
         recruitmentBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->recruitmentBiomass * allPlants[cohortindex]->amount;
         exudationBiomassOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->exudationBiomass * allPlants[cohortindex]->amount;

         gppOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->gpp * allPlants[cohortindex]->amount;
         nppOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->npp * allPlants[cohortindex]->amount;
         respirationOfPlantsPerPFT[allPlants[cohortindex]->pft] += allPlants[cohortindex]->totalRespiration * allPlants[cohortindex]->amount;

         // Community-wide calculations
         totalNumberOfPlantsInCommunity += allPlants[cohortindex]->amount;
         leafAreaIndexOfPlantsInCommunity += allPlants[cohortindex]->lai * allPlants[cohortindex]->coveredArea * allPlants[cohortindex]->amount;
         coveredAreaOfAllPlants += allPlants[cohortindex]->coveredArea * parameter.plantShootOverlapFactors[allPlants[cohortindex]->pft];
      }

      // Normalizations
      if (totalNumberOfPlantsInCommunity > 0)
      {
         for (int pft = 0; pft < parameter.pftCount; pft++)
         {
            pftComposition[pft] *= 100.0 / totalNumberOfPlantsInCommunity;
         }
      }
   }
}