#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
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

   void runModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, OUTPUT &output);
   void doDayStepOfModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management);
   void saveSimulationResultsToBuffer(PARAMETER parameter, COMMUNITY community, OUTPUT &output);
};
