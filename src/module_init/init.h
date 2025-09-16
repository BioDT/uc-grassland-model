#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_recruitment/recruitment.h"
#include "../module_soil/soil.h"
#include "../module_interaction/interaction.h"
#include <iostream>
#include <vector>
#include <random>
#include <limits>

class INIT
{
public:
   INIT();
   ~INIT();

   void initModelSimulation(PARAMETER &parameter, COMMUNITY &community, RECRUITMENT &recruitment, SOIL &soil, INTERACTION &interaction, WEATHER weather);
   void initTimeVariables(PARAMETER &parameter);
   void initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community);
   void initVegetationStateVariables(COMMUNITY &community, PARAMETER parameter, RECRUITMENT &recruitment, SOIL &soil);
   void resetVegetationProcessVariables(PARAMETER parameter, RECRUITMENT &recruitment, COMMUNITY &community, INTERACTION &interaction);
   void initSoilCarbonNitrogenWaterStateVariables(UTILS utils, SOIL &soil, WEATHER weather, PARAMETER parameter);
   double calculateInitialCarbonContentOfAllSoilPools(UTILS utils, WEATHER weather, PARAMETER parameter);
   void resetSoilResourceProcessAndFluxVariables(SOIL &soil);
};