#include "state.h"

STATE::STATE(){};
STATE::~STATE(){};

/* Check if plant cohorts in the community vector are still alive (N>0) and if not, deleting them */
void STATE::checkPlantsAreAliveInCommunity(UTILS ut)
{
    std::vector<int> idsOfDeadPlantCohorts;
    idsOfDeadPlantCohorts.clear();

    if (community.size() > 0)
    {
        /* go through all plant cohorts in the community vector and save indices of dying cohorts */
        for (int plantIndex = 0; plantIndex < community.size(); plantIndex++)
        {
            if (community[plantIndex]->N <= 0)
            {
                idsOfDeadPlantCohorts.push_back(plantIndex);
            }
        }

        /* throw an error if more cohorts should die than existing in the community vector */
        if (idsOfDeadPlantCohorts.size() > community.size())
        {
            ut.handleError("More plant cohorts shall be deleted than existing.");
        }
        else if (idsOfDeadPlantCohorts.size() > 0) /* delete the dying cohorts */
        {
            // reversed sorting of indicees to ensure a correct deleting of plants in community vector from back to front
            std::sort(idsOfDeadPlantCohorts.rbegin(), idsOfDeadPlantCohorts.rend());
            for (auto id : idsOfDeadPlantCohorts)
            {
                community.erase(community.begin() + id);
            }
        }
    }
}

/* Calculate aggregated state variables (for output) based on dynamic changes of the community vector */
void STATE::updateCommunityStateVariables(PARAMETER param)
{
    int pftNumber;
    totalNumberOfPlantsInCommunity = 0;
    for (int pft = 0; pft < param.numberOfSpecies; pft++)
    {
        pftComposition[pft] = 0;
    }

    if (community.size() > 0)
    {

        for (int plantIndex = 0; plantIndex < community.size(); plantIndex++)
        {
            // PFT-specific calculations
            pftNumber = community[plantIndex]->pft;
            pftComposition[pftNumber] += community[plantIndex]->N;

            // Community-wide calculations
            totalNumberOfPlantsInCommunity += community[plantIndex]->N;
        }

        // Normalizations
        if (totalNumberOfPlantsInCommunity > 0)
        {
            for (int pft = 0; pft < param.numberOfSpecies; pft++)
            {
                pftComposition[pft] *= 100.0 / totalNumberOfPlantsInCommunity;
            }
        }
    }
}