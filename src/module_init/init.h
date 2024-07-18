#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include <iostream>
#include <vector>
#include <random>

class INIT
{
public:
   INIT();
   ~INIT();

   void initModelSimulation(PARAMETER &parameter, COMMUNITY &community);
   void initTimeVariables(PARAMETER &parameter);
   void initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community);
   void initCommunityStateVariables(COMMUNITY &community, PARAMETER parameter);
};