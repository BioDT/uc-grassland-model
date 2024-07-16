#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/plant.h"
#include "../module_plant/allometry.h"
#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <algorithm>

class STATE
{
public:
    STATE();
    ~STATE();

    int randomNumberIndex; // incremental integer variable starting from param.randomNumberSeed to create a random number (module_mortality)

    /* grassland community : vector of living plants */
    std::vector<std::shared_ptr<PLANT>> community;

    /* dynamic variables describing aggregated states of the community vector */
    std::vector<double> pftComposition;
    int totalNumberOfPlantsInCommunity;

    void checkPlantsAreAliveInCommunity(UTILS ut);
    void updateCommunityStateVariables(PARAMETER param);
};
