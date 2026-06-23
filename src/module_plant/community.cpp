#include "community.h"

COMMUNITY::COMMUNITY() {};
COMMUNITY::~COMMUNITY() {};

/**
 * @brief Scans all plant cohorts for dead individuals and removes dead cohorts
 *        from the community vector.
 *
 * Iterates over `allPlants` and collects the indices of cohorts whose `amount`
 * is at or below the floating-point threshold (0.000001), treating them as
 * effectively empty. Collected indices are sorted in descending order so that
 * erasing elements by index from the back of the vector does not invalidate
 * earlier indices, then each dead cohort is erased from `allPlants`.
 *
 * Error conditions handled via `utils.handleError()`:
 * - A cohort with a strictly negative `amount` is a model inconsistency and
 *   raises an error immediately.
 * - If more cohorts are flagged for deletion than actually exist in `allPlants`,
 *   an error is raised (guard against logic bugs).
 *
 * @note Called by MORTALITY::doPlantMortality() after all mortality processes
 *       have been applied for the current time step.
 *
 * @param utils Utility object for error handling and reporting.
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
