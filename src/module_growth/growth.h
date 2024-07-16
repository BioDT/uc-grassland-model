#pragma once
#include "../module_step/state.h"
#include "../module_parameter/parameter.h"

class GROWTH
{
public:
    GROWTH();
    ~GROWTH();

    void doPlantGrowth(PARAMETER param, STATE &state);
};