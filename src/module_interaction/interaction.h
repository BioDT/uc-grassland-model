#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_weather/weather.h"
#include "../module_soil/soil.h"
#include "../module_management/management.h"
#include "../utils/utils.h"
#include <vector>
#include <iostream>

class INTERACTION
{
public:
   INTERACTION();
   ~INTERACTION();

   double fullSunLight;
   double dayLength;
   double fullDayAirTemperature;
   double dayTimeAirTemperature;

   int maximumHeightLayerIndexReachedByPlants;
   std::vector<double> LAI;
   std::vector<double> LAIwithLightExtinction;
   std::vector<double> weightsForPlantContributionToHeightLayer;

   void getEnvironmentalConditionsOfDay(WEATHER weather, SOIL soil, MANAGEMENT management, int day);
   void calculateLightAttenuationAndAvailabilityForPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community, double fullSunLight);

   void calculateNumberOfHeightLayersFromLargestPlant(UTILS utils, COMMUNITY &community);
   void calculateCumulativeLeafAreaIndexAcrossHeightLayers(UTILS utils, COMMUNITY &community, PARAMETER parameter);
   void calculateWeightsOfPlantHeightContributionToHeightLayers(int topHeightLayerIndexOfPlant, double plantHeight);
   void addPlantLeafAreaToHeightLayers(int topHeightLayerIndexOfPlant, double leafAreaOfPlantCohort, double plantLightExtinctionCoefficient);
   void accumulateLeafAreaFromTopToBottomHeightLayers(int maximumHeightLayerReachedByPlants);

   void calculateLightAvailabilityForPlants(UTILS utils, COMMUNITY &community, PARAMETER parameter, double fullSunLight);
   void getOvertoppingCumulativeLeafAreaIndexOfPlant(COMMUNITY &community, int cohortindex, int top);
   void calculateAvailableLightReachingAPlant(PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight);
   void calculateShadingIndicatorOfPlantForOutput(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight);
   double getRadiationByLightExtinctionLaw(double cumulativeLAIAboveAndAtHeightLayer, double fullSunLight);
};