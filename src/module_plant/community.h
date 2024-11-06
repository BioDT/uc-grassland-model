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

   /** @brief Incremental integer variable for generating random numbers.
    *
    * This variable starts from a seed defined in the simulation parameters
    * and is used to create random numbers for processes such as plant mortality.
    */
   int randomNumberIndex;

   /** @brief Vector of all living plants in the grassland community.
    *
    * This vector holds shared pointers to `PLANT` objects representing
    * the various cohorts within the community.
    */
   std::vector<std::shared_ptr<PLANT>> allPlants;

   /** @brief Vector describing the composition of Plant Functional Types (PFTs).
    *
    * This vector tracks the relative abundance of each PFT within the community.
    */
   std::vector<double> pftComposition;

   /** @brief Total number of plants in the community.
    *
    * This variable holds the aggregated amount of all living plants present
    * in the `allPlants` vector.
    */
   int totalNumberOfPlantsInCommunity;

   /** @brief Vector that tracks the number of plants per each PFT.
    *
    * This vector records the amount of plants associated with each PFT,
    * allowing for PFT-specific assessments.
    */
   std::vector<double> numberOfPlantsPerPFT;
   double maximumHeightOfAllPlants; // cm
   int totalNumberOfCohortsInCommunity;

   void checkPlantsAreAliveInCommunity(UTILS utils);
   void updateCommunityStateVariablesForOutput(PARAMETER parameter);
};
