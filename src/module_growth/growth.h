#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"

class GROWTH
{
public:
   GROWTH();
   ~GROWTH();

   void doPlantGrowth(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction);

   void doPlantPhotosynthesis(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction);
   double calculateGPPOfPlant(PARAMETER parameter, int pft, double plantLAI, double plantCoveredAre, double plantRadiation, double dayLength);
   double calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI);

   void doPlantRespiration(COMMUNITY &community, PARAMETER parameter);
   void calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter);
   double calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature);
   double calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature);
};