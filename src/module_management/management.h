#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/allometry.h"
#include "../utils/utils.h"
#include <vector>
#include <iostream>

class MANAGEMENT
{
public:
   MANAGEMENT();
   ~MANAGEMENT();

   std::vector<int> mowingDate;
   std::vector<double> mowingHeight;

   std::vector<int> fertilizationDate;
   std::vector<double> fertilizerAmount;

   std::vector<int> irrigationDate;
   std::vector<double> irrigationAmount;

   std::vector<int> sowingDate;
   std::vector<std::vector<int>> amountOfSownSeeds; // dynamic 2D vector of pft and sowing events with elements being the event-specific seed numbers sown

   void applyManagementRegime(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter);
   void initializeYieldVariables(COMMUNITY &community, PARAMETER parameter);
   void checkIfTodayAndDoMowing(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter);
   void cutPlantsAndTrackYieldAndUpdatePlantAttributes(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int cohortIndex, int pft, double heightToCutPlantsDownTo);
};