#include "recruitment.h"

RECRUITMENT::RECRUITMENT(){};
RECRUITMENT::~RECRUITMENT(){};

/* main function of plant recruitment */
void RECRUITMENT::doPlantRecruitment(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management)
{
   // TODO: germination rate, times, seed pool
   createSeedlingsByExternalInflux(parameter, allometry, community);
   createSeedlingsBySowing(parameter, allometry, community, management);
   createSeedlingsByPlantReproduction(parameter, allometry, community);
}

/* plant recruitment via seed influx from an outside area */
void RECRUITMENT::createSeedlingsByExternalInflux(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community)
{
   /* process is only done when activated and current day reached seed influx start (plant traits file parameter) */
   if (parameter.externalSeedInfluxActivated && parameter.day >= parameter.dayOfExternalSeedInfluxStart)
   {
      for (int pft = 0; pft < parameter.pftCount; pft++)
      {
         /* new plant cohorts are stored at the end of the community vector */
         community.allPlants.emplace_back(std::make_shared<PLANT>(parameter, allometry, pft, seedlingHeight, parameter.externalSeedInfluxNumber[pft]));
      }
   }
}

/* plant recruitment via seed sowing as management activity (management input file) */
void RECRUITMENT::createSeedlingsBySowing(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management)
{
   if (management.sowingDate.size() > 0)
   {
      /* for each sowing day from the management file */
      for (int sowingDayIndex = 0; sowingDayIndex < management.sowingDate.size(); sowingDayIndex++)
      {
         /* if the current day is exactly a sowing day */
         if (parameter.day == management.sowingDate[sowingDayIndex])
         {
            for (int pft = 0; pft < parameter.pftCount; pft++)
            {
               community.allPlants.emplace_back(std::make_shared<PLANT>(parameter, allometry, pft, seedlingHeight, management.amountOfSownSeeds[pft][sowingDayIndex]));
            }
         }
      }
   }
}

/* plant recruitment via seed production from mature plants */
void RECRUITMENT::createSeedlingsByPlantReproduction(PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community)
{
   int pft, numberOfSeeds;

   /* process is only done if activated (plant traits parameter file) */
   if (parameter.plantSeedProductionActivated)
   {
      for (int plantIndex = 0; plantIndex < community.allPlants.size(); plantIndex++)
      {
         /* new plant cohorts are stored at the end of the community vector */
         pft = community.allPlants[plantIndex]->pft;

         /* if plants have reached maturity, their recruitment biomass pool is used for seed production (based on PFT-specific seed mass) */
         if (community.allPlants[plantIndex]->age >= parameter.maturityAges[pft - 1])
         {
            if (community.allPlants[plantIndex]->recruitmentBiomass > 0)
            {
               numberOfSeeds = (int)floor((community.allPlants[plantIndex]->recruitmentBiomass / parameter.seedMasses[pft - 1]) + 0.5);
               community.allPlants.emplace_back(std::make_shared<PLANT>(parameter, allometry, pft, seedlingHeight, numberOfSeeds));
            }
         }
      }
   }
}