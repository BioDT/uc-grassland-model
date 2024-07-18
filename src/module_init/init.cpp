#include "init.h"

INIT::INIT(){};
INIT::~INIT(){};

/* main function to initialize state variables of the simulation start */
void INIT::initModelSimulation(PARAMETER &parameter, COMMUNITY &community)
{
   /* init time variables */
   initTimeVariables(parameter);

   /* init random number generator seed */
   initRandomNumberGeneratorSeed(parameter, community);

   /* init state variables of community */
   initCommunityStateVariables(community, parameter);
}

/* initialization of state variables of time */
void INIT::initTimeVariables(PARAMETER &parameter)
{
   parameter.day = 1;                    // start with the first day
   parameter.numberOfDaysToSimulate = 0; // calculate number of days to simulate considering leap years
   for (int year = parameter.firstYear; year <= parameter.lastYear; year++)
   {
      if (year % 4)
      {
         parameter.numberOfDaysToSimulate += 366;
      }
      else
      {
         parameter.numberOfDaysToSimulate += 365;
      }
   }
}

/* initialization of random number generator seed */
void INIT::initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community)
{
   if (parameter.randomNumberGeneratorSeed == -9999)
   {
      std::random_device rd; // seed generator
      parameter.randomNumberGeneratorSeed = rd();
      // written to stdout.txt (printSimulationSettingsToConsole)
   }

   community.randomNumberIndex = parameter.randomNumberGeneratorSeed;
}

/* initialization of the community vector and grassland state variables */
void INIT::initCommunityStateVariables(COMMUNITY &community, PARAMETER parameter)
{
   community.allPlants.clear();
   community.totalNumberOfPlantsInCommunity = 0;
   community.pftComposition.clear();
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      community.pftComposition.push_back(0);
   }
}