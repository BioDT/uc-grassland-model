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
#include "module_step/state.h"
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
    OUTPUT out;
    INPUT in;
    UTILS ut;
    PARAMETER param;
    WEATHER weather;
    SOIL soil;
    MANAGEMENT manage;
    ALLOMETRY allo;
    INIT ini;
    STEP step;
    STATE state;
    RECRUITMENT recruit;
    MORTALITY death;
    GROWTH grow;

    /* Read data from input files (weather, soil, management, species parameter) */
    in.getInputData(path, ut, param, weather, soil, manage);
    out.printSimulationSettingsToConsole(param);

    /* Prepare simulation output files */
    out.prepareModelOutput(path, ut, param);

    /* Initialization of variables and initial conditions for the simulation */
    ini.initModelSimulation(param, state);

    /* Running each day of the model simulation */
    step.runModelSimulation(ut, param, allo, state, recruit, death, grow, manage, out);

    /* Closing of output files */
    out.closeOutputFiles(ut);

    /* Stop tracking of computational runtime */
    std::time_t stopRunTime = std::time(nullptr);

    /* Printing information on console output window */
    std::time_t runTimeDifferenceSeconds = (stopRunTime - startRunTime);
    std::time_t runTimeDifferenceMinutes = (stopRunTime - startRunTime) / (time_t)60.0;
    std::time_t runTimeDifferenceHours = (stopRunTime - startRunTime) / (time_t)(60.0 * 60.0);

    std::cout << "********* Successful simulation run ********" << std::endl;
    std::cout << std::endl;
    std::cout << "Computational runtime: " << runTimeDifferenceSeconds << " seconds" << "(" << runTimeDifferenceHours << "h " << runTimeDifferenceMinutes << "m)" << std::endl;
    std::cout << std::endl;
    std::cout << "********************************************" << std::endl;

    return 0;
}