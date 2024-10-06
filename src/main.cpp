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
#include "utils/utils.h"

#include "module_recruitment/recruitment.h"
#include "module_mortality/mortality.h"
#include "module_plant/allometry.h"
#include "module_growth/growth.h"

int main(int argc, char *argv[])
{
   /* Read in command line arguments */
   std::vector<std::string> commandLineInput;
   for (int it = 0; it < argc; it++)
   {
      commandLineInput.push_back(argv[it]);
   }
   std::string path = commandLineInput.at(1);

   /* Start tracking of computational runtime */
   std::time_t startRunTime = std::time(nullptr);

   /* Create instances of required classes */
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

   /* Read data from input files (weather, soil, management, plant traits parameters) */
   input.getInputData(path, utils, parameter, weather, soil, management);

   /* Prepare simulation output files */
   output.prepareModelOutput(path, utils, parameter);

   /* Initialization of variables and initial conditions for the simulation */
   init.initModelSimulation(parameter, community);

   /* Provide a summary printed to std.out or console */
   output.printSimulationSettingsToConsole(parameter, input);

   /* Running each day of the model simulation */
   step.runModelSimulation(utils, parameter, allometry, community, recruitment, mortality, growth, management, output);

   /* Writing of daily output of simulation results */
   output.writeSimulationResultsToOutputFiles(parameter, utils, community);

   /* Closing of output files */
   output.closeOutputFiles(utils);

   /* Stop tracking of computational runtime */
   std::time_t stopRunTime = std::time(nullptr);

   /* Printing information on console output window */
   std::time_t runTimeRemainSeconds = 0;
   std::time_t runTimeSeconds = 0;
   std::time_t runTimeMinutes = 0;
   std::time_t runTimeHours = 0;

   runTimeHours = (stopRunTime - startRunTime) / (time_t)(60.0 * 60.0);
   runTimeRemainSeconds = (stopRunTime - startRunTime) % (time_t)(60.0 * 60.0);
   runTimeMinutes = runTimeRemainSeconds / (time_t)60.0;
   runTimeSeconds = runTimeRemainSeconds % (time_t)60.0;

   std::cout << "********* Successful simulation run ********" << std::endl;
   std::cout << std::endl;
   std::cout << "Computational runtime: " << runTimeHours << "h " << runTimeMinutes << "m " << runTimeSeconds << "s " << std::endl;
   std::cout << std::endl;
   std::cout << "********************************************" << std::endl;

   return 0;
}