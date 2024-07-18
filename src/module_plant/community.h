#pragma once
#include "plant.h"
#include "allometry.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <algorithm>

class COMMUNITY
{
public:
    COMMUNITY();
    ~COMMUNITY();

    int randomNumberIndex; // incremental integer variable starting from param.randomNumberSeed to create a random number (module_mortality)

    /* grassland community : vector of all living plants */
    std::vector<std::shared_ptr<PLANT>> allPlants;

    /* dynamic variables describing aggregated states of the vector allPlants */
    std::vector<double> pftComposition;
    int totalNumberOfPlantsInCommunity;

    void checkPlantsAreAliveInCommunity(UTILS utils);
    void updateCommunityStateVariables(PARAMETER parameter);
};
