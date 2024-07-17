#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"

class GROWTH
{
public:
    GROWTH();
    ~GROWTH();

    void doPlantGrowth(PARAMETER parameter, COMMUNITY &community);
};