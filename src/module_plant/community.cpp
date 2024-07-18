#include "community.h"

COMMUNITY::COMMUNITY(){};
COMMUNITY::~COMMUNITY(){};

/* Check if plant cohorts in the community vector are still alive (N>0) and if not, deleting them */
void COMMUNITY::checkPlantsAreAliveInCommunity(UTILS utils)
{
    std::vector<int> idsOfDeadPlantCohorts;
    idsOfDeadPlantCohorts.clear();

    if (allPlants.size() > 0)
    {
        /* go through all plant cohorts in the community vector and save indices of dying cohorts */
        for (int plantIndex = 0; plantIndex < allPlants.size(); plantIndex++)
        {
            if (allPlants[plantIndex]->N <= 0)
            {
                idsOfDeadPlantCohorts.push_back(plantIndex);
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

/* Calculate aggregated state variables (for output) based on dynamic changes of the community vector */
void COMMUNITY::updateCommunityStateVariables(PARAMETER parameter)
{
    int pftNumber;
    totalNumberOfPlantsInCommunity = 0;
    for (int pft = 0; pft < parameter.numberOfSpecies; pft++)
    {
        pftComposition[pft] = 0;
    }

    if (allPlants.size() > 0)
    {

        for (int plantIndex = 0; plantIndex < allPlants.size(); plantIndex++)
        {
            // PFT-specific calculations
            pftNumber = allPlants[plantIndex]->pft;
            pftComposition[pftNumber] += allPlants[plantIndex]->N;

            // Community-wide calculations
            totalNumberOfPlantsInCommunity += allPlants[plantIndex]->N;
        }

        // Normalizations
        if (totalNumberOfPlantsInCommunity > 0)
        {
            for (int pft = 0; pft < parameter.numberOfSpecies; pft++)
            {
                pftComposition[pft] *= 100.0 / totalNumberOfPlantsInCommunity;
            }
        }
    }
}