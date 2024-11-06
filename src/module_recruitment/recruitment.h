#pragma once
#include "../module_parameter/parameter.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_plant/community.h"
#include "../module_soil/soil.h"
#include "../utils/utils.h"
#include <random>

/**
 * @brief Class representing the recruitment processes of plants.
 *
 * The `RECRUITMENT` class manages the processes related to plant recruitment,
 * including the inflow of seeds from various sources, seed germination, and
 * the addition of germinated seedlings to the plant community. It tracks
 * incoming seeds, their germination times, and maintains a seed pool.
 */
class RECRUITMENT
{
public:
   RECRUITMENT();
   ~RECRUITMENT();

   /// Vector storing the number of incoming seeds.
   std::vector<int> incomingSeeds;

   /// 2D vector representing the seed pool, categorized by plant functional types (PFTs).
   std::vector<std::vector<int>> seedPool;

   /// 2D vector for tracking the germination time counters of seeds in the seed pool.
   std::vector<std::vector<int>> seedGerminationTimeCounter;

   /// vector that keeps track of successfully germinated seeds at each time step for each PFT
   std::vector<int> successfullGerminatedSeeds;

   void doPlantRecruitment(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management, SOIL &soil);
   void getIncomingSeedsByExternalInflux(PARAMETER parameter);
   void getIncomingSeedsBySowing(PARAMETER parameter, MANAGEMENT management);
   void getIncomingSeedsByPlantReproduction(PARAMETER parameter, COMMUNITY &community);
   void saveIncomingSeedsInSeedPool(PARAMETER parameter);
   void calculateSeedGerminationToSeedlings(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, SOIL &soil);
   void calculateNumberOfGerminatingSeeds(UTILS utils, PARAMETER parameter, COMMUNITY &community, int pft, int cohortindex);
   void transferFailedToGerminateSeedsToLitterPool(UTILS utils, PARAMETER parameter, SOIL &soil, int pft, int cohortindex);
   void updateSeedPool(int pft, int cohortindex);
   void addGerminatedSeedlingsToCommunity(PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, int pft);
};