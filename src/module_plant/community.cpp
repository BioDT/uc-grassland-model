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
         if (allPlants[cohortIndex]->amount < 0)
         {
            utils.handleError("Error (allPlants vector): there is an invalid negative amount of plants within a cohort.");
         }
         else if (allPlants[cohortIndex]->amount <= 0.000001)
         {
            idsOfDeadPlantCohorts.push_back(cohortIndex);
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
