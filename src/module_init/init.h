#pragma once
#include "../module_parameter/parameter.h"
#include "../module_step/state.h"
#include <iostream>
#include <vector>
#include <random>

class INIT
{
public:
    INIT();
    ~INIT();

    void initModelSimulation(PARAMETER &param, STATE &state);
    void initTimeVariables(PARAMETER &param);
    void initRandomNumberGeneratorSeed(PARAMETER &param, STATE &state);
    void initCommunityStateVariables(STATE &state, PARAMETER param);
};