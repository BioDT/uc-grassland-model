#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_input/input.h"
#include "../module_soil/soil.h"
#include "../module_management/management.h"
#include "../module_recruitment/recruitment.h"
#include "../utils/utils.h"
#include <iostream>
#include <vector>
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/**
 * @brief Handles output operations for the simulation.
 *
 * The `OUTPUT` class manages the generation and storage of simulation output
 * data, including creating output files, writing simulation results, and
 * managing output directories.
 */
class OUTPUT
{
public:
    OUTPUT();
    ~OUTPUT();

    std::string outputDirectory;       /// Directory where output files will be stored.
    std::string fileDirectory;         /// Directory of the outputWritingDates file.
    std::ofstream outputCommunity;     /// Output file stream for writing data.
    std::ofstream outputPFTPopulation; /// Output file stream for writing data.
    std::ofstream outputPlant;         /// Output file stream for writing data.
    std::ofstream outputSoilCarbon;    /// Output file stream for writing data.
    std::ofstream outputSoilNitrogen;  /// Output file stream for writing data.
    std::ofstream outputSoilWater;     /// Output file stream for writing data.

    std::stringstream bufferCommunity;     /// Temporary storage buffer for output data at community level.
    std::stringstream bufferPFTPopulation; /// Temporary storage buffer for output data at PFT level.
    std::stringstream bufferPlant;         /// Temporary storage buffer for output data at cohort level.
    std::stringstream bufferSoilCarbon;    /// Temporary storage buffer for output data of soil carbon dynamics.
    std::stringstream bufferSoilNitrogen;  /// Temporary storage buffer for output data of soil nitrogen dynamics.
    std::stringstream bufferSoilWater;     /// Temporary storage buffer for output data of soil water dynamics.

    std::vector<int> outputWritingDates; /// Dates for writing output data.
    bool outputWritingDatesFileOpened;   /// Flag indicating if the output writing dates file is opened.

    void updateVegetationStateVariablesForOutput(PARAMETER parameter, COMMUNITY &community, SOIL soil, MANAGEMENT management, RECRUITMENT recruitment);
    void prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter);
    void createOutputFolder(std::string path, UTILS utils);
    void openAndReadOutputWritingDates(std::string path, UTILS utils, PARAMETER &parameter);
    void printSimulationSettingsToConsole(PARAMETER parameter, INPUT input);

    void createAndOpenOutputFiles(UTILS utils, PARAMETER parameter);
    void writeHeaderInOutputFiles(UTILS utils, PARAMETER parameter);
    void writeSimulationResultsToOutputFiles(UTILS utils, PARAMETER parameter);
    void closeOutputFiles(UTILS utils, PARAMETER parameter);
};