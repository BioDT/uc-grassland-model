#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"
#include "../module_plant/allometry.h"
#include "../utils/utils.h"

class GROWTH
{
public:
   GROWTH();
   ~GROWTH();

   void doPlantGrowth(UTILS utils, PARAMETER parameter, COMMUNITY &community, INTERACTION interaction, ALLOMETRY allometry);

   void doPlantPhotosynthesis(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction);
   double calculateGPPOfPlant(PARAMETER parameter, int pft, double plantLAI, double plantCoveredAre, double plantRadiation, double dayLength);
   double calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI);

   void doPlantRespiration(COMMUNITY &community, PARAMETER parameter, INTERACTION interaction);
   void calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter);
   double calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature);
   double calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature);
   void adjustAllocationRates(UTILS utils, PARAMETER parameter, COMMUNITY &community);
   void doPlantNPPAllocation(UTILS utils, COMMUNITY &community);
   double calculateProportionalityFactorForAllocationDistributionToPlantParts(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft);
   void adjustAllocationRatesForMaturePlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);
   void adjustAllocationRatesForRegrowingPlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);
   void doPlantGrowthInSizeAndAging(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry);
};