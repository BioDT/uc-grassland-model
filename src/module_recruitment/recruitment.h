#pragma once
#include "../module_parameter/parameter.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_plant/community.h"

class RECRUITMENT
{
public:
    RECRUITMENT();
    ~RECRUITMENT();

    void doPlantRecruitment(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management);
    void createSeedlingsByExternalInflux(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community);
    void createSeedlingsBySowing(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management);
    void createSeedlingsByPlantReproduction(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community);
};