#pragma once
#include "plant.h"
#include "allometry.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @brief Represents a community of plants in a grassland ecosystem.
 *
 * The `COMMUNITY` class maintains a collection of plant cohorts, tracks their state,
 * and provides methods for managing and updating the community dynamics. It includes
 * functionalities to check for plant viability and calculate aggregated state variables
 * based on the current plant composition.
 */
class COMMUNITY
{
public:
   COMMUNITY();
   ~COMMUNITY();

   int randomNumberIndex;

   std::vector<std::shared_ptr<PLANT>> allPlants;

   int totalNumberOfPlantsInCommunity;
   int totalNumberOfCohortsInCommunity;

   double maximumHeightOfAllPlants;         // cm
   double leafAreaIndexOfPlantsInCommunity; // cm per cm
   double coveredAreaOfAllPlants;
   double greenBiomassYield;
   double brownBiomassYield;
   double biomassYield;

   std::vector<double> pftComposition;
   std::vector<double> numberOfPlantsPerPFT;
   std::vector<double> coveredAreaOfPlantsPerPFT;
   std::vector<double> shootBiomassOfPlantsPerPFT;
   std::vector<double> greenShootBiomassOfPlantsPerPFT;
   std::vector<double> brownShootBiomassOfPlantsPerPFT;
   std::vector<double> clippedShootBiomassOfPlantsPerPFT;
   std::vector<double> rootBiomassOfPlantsPerPFT;
   std::vector<double> recruitmentBiomassOfPlantsPerPFT;
   std::vector<double> exudationBiomassOfPlantsPerPFT;

   std::vector<double> gppOfPlantsPerPFT;
   std::vector<double> nppOfPlantsPerPFT;
   std::vector<double> respirationOfPlantsPerPFT;

   std::vector<double> greenBiomassYieldPerPFT;
   std::vector<double> brownBiomassYieldPerPFT;
   std::vector<double> biomassYieldPerPFT;

   void checkPlantsAreAliveInCommunity(UTILS utils);
   void updateCommunityStateVariablesForOutput(PARAMETER parameter);
};
