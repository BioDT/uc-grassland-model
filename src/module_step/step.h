#pragma once
#include "state.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/plant.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_recruitment/recruitment.h"
#include "../module_mortality/mortality.h"
#include "../module_growth/growth.h"
#include "../module_output/output.h"
#include "../utils/utils.h"
#include <random>

class STEP
{
public:
    STEP();
    ~STEP();

    void runModelSimulation(UTILS ut, PARAMETER &param, ALLOMETRY allo, STATE &state, RECRUITMENT recruit, MORTALITY death, GROWTH grow, MANAGEMENT manage, OUTPUT &out);
    void doDayStepOfModelSimulation(UTILS ut, PARAMETER &param, ALLOMETRY allo, STATE &state, RECRUITMENT recruit, MORTALITY death, GROWTH grow, MANAGEMENT manage);
};
