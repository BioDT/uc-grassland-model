#include "output.h"

OUTPUT::OUTPUT(){};
OUTPUT::~OUTPUT(){};

/* Creates result folder, output file and its header */
void OUTPUT::prepareModelOutput(std::string path, UTILS ut, PARAMETER &param)
{
    /* create a folder where result files are written (if not already existing) */
    createOutputFolder(path, ut);
    createAndOpenOutputFiles(param, ut);
    writeHeaderInOutputFiles(param, ut);
    openAndReadOutputWritingDates(path, ut, param);
}

/* Creates the output file */
void OUTPUT::createAndOpenOutputFiles(PARAMETER param, UTILS ut)
{
    if (param.outputFile)
    {
        ut.strings.clear();
        ut.splitString(param.speciesFile, '/');
        std::string endingLocation = std::to_string(param.latitude) + "_" + std::to_string(param.longitude);
        std::string endingYears = "_" + std::to_string(param.firstYear) + "_" + std::to_string(param.lastYear) + "_";
        std::string endingParameter = ut.strings.at(1);
        std::string fileEnding = endingLocation + endingYears + endingParameter;
        std::string filename = outputDirectory + "output_" + fileEnding;
        outputFile.open(filename);
        if (!outputFile.is_open())
        {
            ut.handleError("Error writing to the output file.");
        }
    }
};

/* Writes the header in the output file */
void OUTPUT::writeHeaderInOutputFiles(PARAMETER param, UTILS ut)
{
    if (param.outputFile)
    {
        if (!outputFile.is_open())
        {
            ut.handleError("Error writing to the output file.");
        }
        else
        {
            outputFile << "Simulation results on grassland dynamics (at population and community level)." << std::endl;
            outputFile << "Time\tPFT\tFraction" << std::endl;
        }
    }
}

/* writes daily simulation resulst (state variables of the community) to the output file */
void OUTPUT::writeSimulationResultsToOutputFiles(PARAMETER param, UTILS ut, STATE state)
{
    if (param.outputFile)
    {
        if (outputFile.is_open())
        {
            for (auto day : outputWritingDates)
            {
                if (param.day == day)
                {
                    for (int pft = 0; pft < param.numberOfSpecies; pft++)
                    {
                        outputFile << param.day << "\t" << pft << "\t" << state.pftComposition[pft] << std::endl;
                    }
                }
            }
        }
        else
        {
            ut.handleError("The output file is not open for writing.");
        }
    }
}

/* Closes the output file */
void OUTPUT::closeOutputFiles(UTILS ut)
{
    if (outputFile.is_open())
    {
        outputFile.close();
    }
    else
    {
        ut.handleError("The output file is not open.");
    }
}

/* Creates a folder for the output file */
void OUTPUT::createOutputFolder(std::string path, UTILS ut)
{
    char separator = '\\';
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 1; it++)
    {
        outputDirectory = outputDirectory + ut.strings.at(it) + "\\";
    }
    outputDirectory = outputDirectory + "output\\";
    _mkdir(outputDirectory.c_str());
}

/* Read-in output writing dates from file */
void OUTPUT::openAndReadOutputWritingDates(std::string path, UTILS ut, PARAMETER &param)
{
    char separator = '\\';
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 1; it++)
    {
        fileDirectory = fileDirectory + ut.strings.at(it) + "\\";
    }
    fileDirectory = fileDirectory + param.outputWritingDatesFile;

    const char *filename = fileDirectory.c_str();
    std::ifstream file(filename);

    std::string line; // current line text in parser
    int m = 0;        // current line number in parser
    outputWritingDates.clear();

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            m++;
            if (m > 1)
            { // skip header line
                ut.strings.clear();
                ut.splitString(line, '-');
                int day = std::stoi(ut.strings.at(2).c_str());
                int month = std::stoi(ut.strings.at(1).c_str());
                int year = std::stoi(ut.strings.at(0).c_str());
                outputWritingDates.push_back(ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1);
            }
        }
        file.close();
    }
}

/* print important settings of the simulation to the console (stdout.txt) */
void OUTPUT::printSimulationSettingsToConsole(PARAMETER param)
{

    std::cout << "*******************************************" << std::endl;
    std::cout << "******* Grassland site simulation *********" << std::endl;
    std::cout << "*******************************************" << std::endl
              << std::endl;

    std::cout << "DEIMS.id = " << param.deimsID << std::endl;
    std::cout << "Latitude = " << param.latitude << std::endl;
    std::cout << "Longitude = " << param.longitude << std::endl;
    std::cout << "First year = " << param.firstYear << std::endl;
    std::cout << "Last year = " << param.lastYear << std::endl
              << std::endl;

    std::cout << "******* Input files *********" << std::endl
              << std::endl;

    std::cout << "Weather data: " << param.weatherFile << std::endl;
    std::cout << "Soil data: " << param.soilFile << std::endl;
    std::cout << "Management data: " << param.managementFile << std::endl;
    std::cout << "Plant species traits: " << param.speciesFile << std::endl
              << std::endl;

    std::cout << "******* Simulation output writing *********" << std::endl
              << std::endl;

    std::string yesNo = (param.reproducibleResults == true) ? "Yes" : "No";
    std::string seed = (param.reproducibleResults == true) ? (" (seed = " + std::to_string(param.randomNumberGeneratorSeed) + ")") : "";
    std::cout << "Reproducible simulation output? " << yesNo << seed << std::endl;

    yesNo = (param.outputFile == true) ? "Yes" : "No";
    std::string dates = (param.outputFile == true) ? (" (at dates provided in " + param.outputWritingDatesFile + ")") : "";
    std::cout << "Write output file? " << yesNo << dates << std::endl
              << std::endl;

    std::cout << std::endl
              << std::endl;
}