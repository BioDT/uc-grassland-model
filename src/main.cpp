#include <iostream>
#include <ctime>
#include <vector>

#include "module_input/input.h"
#include "module_output/output.h"
#include "module_parameter/parameter.h"
#include "module_weather/weather.h"
#include "module_soil/soil.h"
#include "module_management/management.h"
#include "module_init/init.h"
#include "module_step/step.h"
#include "module_plant/community.h"
#include "module_interaction/interaction.h"
#include "utils/utils.h"

#include "module_recruitment/recruitment.h"
#include "module_mortality/mortality.h"
#include "module_plant/allometry.h"
#include "module_growth/growth.h"

/**
 * @brief Main function to execute the model simulation.
 *
 * This function processes the command line arguments, initializes all necessary
 * objects and variables, reads input data, runs the model simulation, and writes
 * the output results. It further tracks the computational runtime and prints the total
 * runtime at the end of the simulation.
 *
 * @param argc The number of command line arguments.
 * @param argv An array of command line argument strings.
 * @return int Returns 0 upon successful execution.
 *
 * The main steps of the simulation include:
 *  - Reading input data (weather, soil, management, plant trait parameters).
 *  - Initializing the model simulation.
 *  - Running the simulation for each day.
 *  - Writing the results to output files.
 *  - Closing output files and printing runtime information.
 */
int main(int argc, char *argv[])
{
   /**
    * @brief Reads in command line arguments and stores them in a vector.
    */
   std::vector<std::string> commandLineInput;
   for (int it = 0; it < argc; it++)
   {
      commandLineInput.push_back(argv[it]);
   }
   std::string path = commandLineInput.at(1);

   /**
    * @brief Starts tracking the computational runtime.
    */
   std::time_t startRunTime = std::time(nullptr);

   /**
    * @brief Create instances of the required classes.
    */
   OUTPUT output;
   INPUT input;
   UTILS utils;
   PARAMETER parameter;
   WEATHER weather;
   SOIL soil;
   MANAGEMENT management;
   ALLOMETRY allometry;
   INIT init;
   STEP step;
   COMMUNITY community;
   RECRUITMENT recruitment;
   MORTALITY mortality;
   GROWTH growth;
   INTERACTION interaction;

   /**
    * @brief Reads data from input files (weather, soil, management, plant traits).
    *
    * @param path The path to the input data files.
    * @param utils Utility class for helper functions.
    * @param parameter Contains the parameters for the simulation.
    * @param weather Weather data required for the simulation.
    * @param soil Soil data for the simulation.
    * @param management Management actions to be simulated.
    */
   input.getInputData(path, utils, parameter, weather, soil, management);

   /**
    * @brief Initializes variables and sets up initial conditions for the simulation.
    */
   init.initModelSimulation(parameter, community, recruitment, soil, interaction);

   /**
    * @brief Prepares output files for writing simulation results.
    */
   output.prepareModelOutput(path, utils, parameter);

   /**
    * @brief Prints a summary of the simulation settings to the console.
    */
   output.printSimulationSettingsToConsole(parameter, input);

   /**
    * @brief Runs the model simulation for each day.
    */
   step.runModelSimulation(utils, parameter, init, allometry, community, recruitment, mortality, growth, management, soil, weather, interaction, output);

   /**
    * @brief Writes the daily simulation results to output files.
    */
   output.writeSimulationResultsToOutputFiles(utils);

   /**
    * @brief Closes the output files after writing the results.
    */
   output.closeOutputFiles(utils);

   /**
    * @brief Stops tracking the computational runtime.
    */
   std::time_t stopRunTime = std::time(nullptr);

   /**
    * @brief Calculates and prints the total runtime of the simulation.
    */
   std::time_t runTimeRemainSeconds = 0;
   std::time_t runTimeSeconds = 0;
   std::time_t runTimeMinutes = 0;
   std::time_t runTimeHours = 0;

   runTimeHours = (stopRunTime - startRunTime) / (time_t)(60.0 * 60.0);
   runTimeRemainSeconds = (stopRunTime - startRunTime) % (time_t)(60.0 * 60.0);
   runTimeMinutes = runTimeRemainSeconds / (time_t)60.0;
   runTimeSeconds = runTimeRemainSeconds % (time_t)60.0;

   /**
    * @brief Writes the runtime information to the console.
    */
   std::cout << "********* Successful simulation run ********" << std::endl;
   std::cout << std::endl;
   std::cout << "Computational runtime: " << runTimeHours << "h " << runTimeMinutes << "m " << runTimeSeconds << "s " << std::endl;
   std::cout << std::endl;
   std::cout << "********************************************" << std::endl;

   return 0;
}