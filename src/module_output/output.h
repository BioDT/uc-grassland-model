#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../utils/utils.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <direct.h>

class OUTPUT
{
public:
    OUTPUT();
    ~OUTPUT();

    std::string outputDirectory;
    std::ofstream outputFile;
    std::string fileDirectory; // directory of outputWritingDates file

    std::vector<int> outputWritingDates;

    void prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter);
    void createOutputFolder(std::string path, UTILS utils);
    void openAndReadOutputWritingDates(std::string path, UTILS utils, PARAMETER &parameter);
    void printSimulationSettingsToConsole(PARAMETER parameter);

    void createAndOpenOutputFiles(PARAMETER parameter, UTILS utils);
    void writeHeaderInOutputFiles(PARAMETER parameter, UTILS utils);
    void writeSimulationResultsToOutputFiles(PARAMETER parameter, UTILS utils, COMMUNITY community);
    void closeOutputFiles(UTILS utils);
};