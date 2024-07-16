#pragma once
#include "../module_step/state.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"
#include <random>

class MORTALITY
{
public:
    MORTALITY();
    ~MORTALITY();

    void doPlantMortality(PARAMETER param, STATE &state, UTILS ut);
    void doSenescenceAndLitterFall();
    void doThinning();
    void doBasicMortality(PARAMETER param, UTILS ut, STATE &state, int plantIndex, int pft);
    double getPlantMortalityRate(PARAMETER param, STATE state, int plantIndex, int pft);
};