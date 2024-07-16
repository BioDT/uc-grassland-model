#pragma once
#include "../module_parameter/parameter.h"
#include "../module_step/state.h"
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

    void prepareModelOutput(std::string path, UTILS ut, PARAMETER &param);
    void createOutputFolder(std::string path, UTILS ut);
    void openAndReadOutputWritingDates(std::string path, UTILS ut, PARAMETER &param);
    void printSimulationSettingsToConsole(PARAMETER param);

    void createAndOpenOutputFiles(PARAMETER param, UTILS ut);
    void writeHeaderInOutputFiles(PARAMETER param, UTILS ut);
    void writeSimulationResultsToOutputFiles(PARAMETER param, UTILS ut, STATE state);
    void closeOutputFiles(UTILS ut);
};