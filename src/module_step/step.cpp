#include "step.h"

STEP::STEP() {};
STEP::~STEP() {};

/**
 * @brief Simulates the entire simulation period.
 *
 * This function runs the model simulation for a specified number of days as defined
 * by the `parameter.simulationTimeInDays`. For each day in the simulation, it resets specific state variables,
 * performs daily plant processes, updates the community's dynamic state variables, and saves the simulation results.
 *
 * @param utils Utility functions for string manipulation and error handling.
 * @param parameter Reference to a `PARAMETER` object containing simulation parameters,
 *                  including the current day of the simulation.
 * @param init An `INIT` object used for initializing and resetting process variables.
 * @param allometry An `ALLOMETRY` object that contains functions related to allometric
 *                  calculations for plant growth and structure.
 * @param community Reference to a `COMMUNITY` object representing the plant community
 *                  being simulated.
 * @param recruitment Reference to a `RECRUITMENT` object handling the plant recruitment processes
 *                    within the community.
 * @param mortality A `MORTALITY` object that handles plant mortality processes in the community.
 * @param growth A `GROWTH` object that calculates plant growth processes of the community.
 * @param management A `MANAGEMENT` object that performs predefined management regimes.
 * @param soil Reference to a `SOIL` object representing soil characteristics and processes.
 * @param output Reference to an `OUTPUT` object for saving simulation results.
 */
void STEP::runModelSimulation(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output)
{
   /* Daily steps to be simulated */
   for (int day = 1; day <= parameter.simulationTimeInDays; day++)
   {
      parameter.day = day; // increase day according to for-loop

      /* Resetting of specific state / process variables of the community */
      init.initAndResetProcessVariables(parameter, recruitment, community, interaction);

      /* Environmental conditions of the day */
      interaction.getEnvironmentalConditionsOfDay(weather, soil, management, parameter.day);

      /* Calculation of ecological and plant processes */
      doDayStepOfModelSimulation(utils, parameter, allometry, community, recruitment, mortality, growth, interaction, management, soil);

      community.updateCommunityStateVariablesForOutput(parameter);

      /* Writing of daily output of simulation results */
      saveSimulationResultsToBuffer(utils, parameter, community, output);
   }
}

/**
 * @brief Performs one day step of all plant processes.
 *
 * This function executes the daily processes for plant dynamics, including recruitment,
 * mortality, and growth.
 *
 * @param utils Utility functions for various operations including error handling.
 * @param parameter Reference to a `PARAMETER` object containing simulation parameters.
 * @param allometry An `ALLOMETRY` object that handles allometric calculations related
 *                  to plant growth and structure.
 * @param community Reference to a `COMMUNITY` object representing the current state
 *                  of the plant community.
 * @param recruitment Reference to a `RECRUITMENT` object that manages the recruitment
 *                    of new plants into the community.
 * @param mortality A `MORTALITY` object that processes and calculates plant mortality
 *                  within the community.
 * @param growth A `GROWTH` object responsible for calculating the growth of plants
 *               in the community.
 * @param management A `MANAGEMENT` object that applies predefined management regimes
 *                   to the community.
 * @param soil Reference to a `SOIL` object that represents the soil characteristics
 *              affecting plant processes.
 */
void STEP::doDayStepOfModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, INTERACTION &interaction, MANAGEMENT management, SOIL &soil)
{
   /* Plant recruitment */
   recruitment.doPlantRecruitment(utils, parameter, allometry, community, management, soil);

   /* Plant mortality */
   mortality.doPlantMortality(parameter, community, utils);

   /* Calculate light conditions & plant shading */
   interaction.calculateLightAttenuationAndAvailabilityForPlants(utils, parameter, community, interaction.fullSunLight);

   // growth.doPlantGrowth(parameter, community, weather);
}

/**
 * @brief Saves the simulation results to a buffer for output.
 *
 * This function stores the simulation results of the current day into a buffer,
 * which can later be written to an output file. The results include information
 * about the plant functional types (PFTs) present in the community, including
 * their composition and the number of individual plants. The data is saved
 * conditionally based on whether specific output writing dates are configured.
 *
 * @param parameter A `PARAMETER` object containing simulation parameters such as
 *                  the current day and the number of plant functional types (PFTs).
 * @param community A `COMMUNITY` object that holds the current state of the plant
 *                  community, including its composition and number of plants per PFT.
 * @param output Reference to an `OUTPUT` object that manages the output buffer
 *               and handles writing the results to files.
 *
 * The function distinguishes between two cases:
 * - If the `outputWritingDatesFileOpened` is true, results are only saved for
 *   the days specified in `output.outputWritingDates`.
 * - Otherwise, results for every day of the simulation are stored directly in the
 *   buffer.
 */
void STEP::saveSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output)
{
   int day = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "day");
   int month = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "month");
   int year = utils.calculateDateFromDayCount(utils, parameter.day, parameter.referenceJulianDayStart, "year");

   std::string sDay, sMonth, date;
   sDay = (day < 10) ? ("0" + std::to_string(day)) : std::to_string(day);
   sMonth = (month < 10) ? ("0" + std::to_string(month)) : std::to_string(month);
   date = std::to_string(year) + "-" + sMonth + "-" + sDay;

   if (output.outputWritingDatesFileOpened)
   { /* results only at outputWritinDates are stored in buffer */
      for (auto day : output.outputWritingDates)
      {
         if (parameter.day == day)
         {
            for (int pft = 0; pft < parameter.pftCount; pft++)
            {
               output.bufferCommunity << date << "\t" << parameter.day << "\t" << pft << "\t" << community.pftComposition[pft] << "\t" << community.numberOfPlantsPerPFT[pft] << std::endl;
            }
         }
      }
   }
   else /* daily results stored in buffer */
   {
      for (int pft = 0; pft < parameter.pftCount; pft++)
      {
         output.bufferCommunity << date << "\t" << parameter.day << "\t" << pft << "\t" << community.pftComposition[pft] << "\t" << community.numberOfPlantsPerPFT[pft] << std::endl;
      }
   }
}
