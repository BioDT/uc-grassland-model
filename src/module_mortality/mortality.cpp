#include "mortality.h"

MORTALITY::MORTALITY(){};
MORTALITY::~MORTALITY(){};

/* main mortality function */
void MORTALITY::doPlantMortality(PARAMETER parameter, COMMUNITY &community, UTILS utils)
{
   int pft;

   if (community.allPlants.size() > 0)
   {
      for (int plantIndex = 0; plantIndex < community.allPlants.size(); plantIndex++)
      {
         pft = community.allPlants[plantIndex]->pft;
         // TODO: add crowding and senescence / litter fall
         doSenescenceAndLitterFall();

         community.randomNumberIndex++;
         doThinning();

         community.randomNumberIndex++;
         doBasicMortality(parameter, utils, community, plantIndex, pft);
      }
   }

   community.checkPlantsAreAliveInCommunity(utils);
}

/* Leaf and root senescence and litter fall */
void MORTALITY::doSenescenceAndLitterFall()
{
}

/* Plant mortality due to thinning of the community */
void MORTALITY::doThinning()
{
}

/* Plant mortality due to instrinsic mortality rate */
void MORTALITY::doBasicMortality(PARAMETER parameter, UTILS utils, COMMUNITY &community, int plantIndex, int pft)
{
   double mortalityRate = getPlantMortalityRate(parameter, community, plantIndex, pft);

   std::uniform_real_distribution<> dis(0.0, 1.0);
   std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
   double randomNumber = dis(gen);

   /* let plants die according to the mortality rate */
   if (randomNumber <= mortalityRate)
   {
      if (community.allPlants[plantIndex]->N > 0)
      {
         community.allPlants[plantIndex]->N -= 1;
      }
      else
      {
         utils.handleError("Error (mortality): no more plants available in the cohort to die.");
      }
   }
}

double MORTALITY::getPlantMortalityRate(PARAMETER parameter, COMMUNITY community, int plantIndex, int pft)
{
   /* check which age plants have and get the correct mortality rate */
   if (community.allPlants[plantIndex]->isAdult)
   {
      return (parameter.plantMortalityRates[pft]);
   }
   else
   {
      return (parameter.seedlingMortalityRates[pft]);
   }
}
