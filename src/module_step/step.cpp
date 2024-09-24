#include "step.h"

STEP::STEP() {};
STEP::~STEP() {};

/* Simulates the entire simulation period */
void STEP::runModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, OUTPUT &output)
{
   /* Perform daily plant processes for each day to be simulated */
   for (int day = 1; day <= parameter.simulationTimeInDays; day++)
   {
      parameter.day = day; // increase day according to for-loop
      doDayStepOfModelSimulation(utils, parameter, allometry, community, recruitment, mortality, growth, management);

      /* Writing of daily output of simulation results */
      saveSimulationResultsToBuffer(parameter, community, output);
   }
}

/* Performs one day step of all plant processes */
void STEP::doDayStepOfModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management)
{
   /* Plant recruitment */
   recruitment.doPlantRecruitment(parameter, allometry, community, management);

   /* Plant mortality */
   mortality.doPlantMortality(parameter, community, utils);

   /* Plant growth */
   growth.doPlantGrowth(parameter, community);

   /* Calculation of dynamic state variables of the community */
   community.updateCommunityStateVariables(parameter);
}

/* saves daily simulation results (state variables of the community) to a temporary buffer */
void STEP::saveSimulationResultsToBuffer(PARAMETER parameter, COMMUNITY community, OUTPUT &output)
{
   if (output.outputWritingDatesFileOpened)
   { /* results only at outputWritinDates are stored in buffer */
      for (auto day : output.outputWritingDates)
      {
         if (parameter.day == day)
         {
            for (int pft = 0; pft < parameter.pftCount; pft++)
            {
               output.buffer << parameter.day << "\t" << pft << "\t" << community.pftComposition[pft] << std::endl;
            }
         }
      }
   }
   else /* daily results stored in buffer */
   {
      for (int pft = 0; pft < parameter.pftCount; pft++)
      {
         output.buffer << parameter.day << "\t" << pft << "\t" << community.pftComposition[pft] << std::endl;
      }
   }
}
