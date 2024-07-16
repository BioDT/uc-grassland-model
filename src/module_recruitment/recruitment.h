#pragma once
#include "../module_parameter/parameter.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_step/state.h"

class RECRUITMENT
{
public:
    RECRUITMENT();
    ~RECRUITMENT();

    void doPlantRecruitment(PARAMETER param, ALLOMETRY allo, STATE &state, MANAGEMENT manage);
    void createSeedlingsByExternalInflux(PARAMETER param, ALLOMETRY allo, STATE &state);
    void createSeedlingsBySowing(PARAMETER param, ALLOMETRY allo, STATE &state, MANAGEMENT manage);
    void createSeedlingsByPlantReproduction(PARAMETER param, ALLOMETRY allo, STATE &state);
};