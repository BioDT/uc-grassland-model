#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"
#include <random>

class MORTALITY
{
public:
   MORTALITY();
   ~MORTALITY();

   void doPlantMortality(PARAMETER parameter, COMMUNITY &community, UTILS utils);
   void doSenescenceAndLitterFall();
   void doThinning();
   void doBasicMortality(PARAMETER parameter, UTILS utils, COMMUNITY &community, int plantIndex, int pft);
   double getPlantMortalityRate(PARAMETER parameter, COMMUNITY community, int plantIndex, int pft);
};