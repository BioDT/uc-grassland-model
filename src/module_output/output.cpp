#include "output.h"

OUTPUT::OUTPUT() {};
OUTPUT::~OUTPUT() {};

/**
 * @brief Creates the result folder, output file, and its header.
 *
 * This method prepares the model output by performing the following steps:
 * - Creates a folder where the result files will be written if it does not already exist.
 * - Creates and opens the output files for writing.
 * - Writes the header information in the output files.
 * - Opens and reads the output writing dates from a specified file.
 *
 * @param path The base path for the output files. This is the directory where
 *             the result folder will be created and where output files will be saved.
 * @param utils Utility functions for file handling and directory management.
 * @param parameter Reference to the PARAMETER object containing simulation settings
 *                  and configurations required for output file creation and writing.
 */
void OUTPUT::prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter)
{
   createOutputFolder(path, utils);
   createAndOpenOutputFiles(parameter, utils);
   writeHeaderInOutputFiles(utils);
   openAndReadOutputWritingDates(path, utils, parameter);
}

/**
 * @brief Creates and opens the output file for writing simulation results.
 *
 * This method constructs a filename based on parameters such as latitude,
 * longitude, simulation years, and a random number generator seed. It uses
 * these parameters to generate a unique filename for the output file, then
 * attempts to open this file for writing. If the file cannot be opened,
 * an error is logged and handled appropriately.
 *
 * The generated filename format is as follows:
 *
 * - Base output directory: `outputDirectory`
 * - Ending location: derived from latitude and longitude.
 * - Ending years: formatted as `__firstYear-01-01_lastYear-12-31`.
 * - Random seed: formatted as `__randomNumberGeneratorSeed`.
 * - Parameter ending: derived from the `plantTraitsFile`.
 *
 * @param parameter Reference to the PARAMETER object that contains simulation
 *                  configurations, including geographical coordinates and
 *                  simulation years.
 * @param utils Utility functions for string manipulation and error handling.
 *
 * @throws std::ios_base::failure If the output file cannot be opened.
 */
void OUTPUT::createAndOpenOutputFiles(PARAMETER parameter, UTILS utils)
{
   utils.strings.clear();
   utils.splitString(parameter.plantTraitsFile, '/');
   std::string endingLocation = "lat" + parameter.latitude + "_lon" + parameter.longitude;
   std::string endingYears = "__" + std::to_string(parameter.firstYear) + "-01-01_" + std::to_string(parameter.lastYear) + "-12-31";
   std::string runNumber = (parameter.randomNumberGeneratorSeed < 10) ? ("00" + std::to_string(parameter.randomNumberGeneratorSeed)) : ((parameter.randomNumberGeneratorSeed < 100) ? ("0" + std::to_string(parameter.randomNumberGeneratorSeed)) : (std::to_string(parameter.randomNumberGeneratorSeed)));
   std::string endingRandomSeed = "__run" + runNumber;
   std::string plantTraitsFile = utils.strings.at(1);
   utils.strings.clear();
   utils.splitString(plantTraitsFile, '_');
   std::string endingParameter = utils.strings.at(utils.strings.size() - 2) + "_" + utils.strings.at(utils.strings.size() - 1);

   std::string filenameCommunity = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputCommunity__" + endingParameter;
   std::string filenamePFTPopulation = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputPFT__" + endingParameter;
   std::string filenamePlant = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputPlant__" + endingParameter;

   outputCommunity.open(filenameCommunity);
   if (!outputCommunity.is_open())
   {
      utils.handleError("Error writing to the community output file.");
   }

   outputPFTPopulation.open(filenamePFTPopulation);
   if (!outputPFTPopulation.is_open())
   {
      utils.handleError("Error writing to the PFT population output file.");
   }

   outputPlant.open(filenamePlant);
   if (!outputPlant.is_open())
   {
      utils.handleError("Error writing to the plant / cohorte output file.");
   }
};

/**
 * @brief Writes the header to the output file.
 *
 * This method writes the header line to the output file, which includes
 * column titles for the data that will be recorded during the simulation.
 * The header includes the following columns:
 *
 * - Date
 * - PFT (Plant Functional Type)
 * - Fraction
 * - Number of Plants
 *
 * The method checks if the output file is open before attempting to write.
 * If the file is not open, an error is logged and handled appropriately.
 *
 * @param utils Utility functions for error handling and other utilities.
 *
 * @throws std::ios_base::failure If the output file is not open.
 */
void OUTPUT::writeHeaderInOutputFiles(UTILS utils)
{

   if (!outputCommunity.is_open())
   {
      utils.handleError("Error writing to the community output file.");
   }
   else
   {
      outputCommunity << "Date\tDayCount\tNumberPlants\tLeafAreaIndex";
      outputCommunity << std::endl;
   }

   if (!outputPFTPopulation.is_open())
   {
      utils.handleError("Error writing to the PFT population output file.");
   }
   else
   {
      outputPFTPopulation << "Date\tDayCount\tPFT\tFraction\tNumberPlants\t";
      outputPFTPopulation << "CoveredArea\tShootBiomass\tGreenShootBiomass\tBrownShootBiomass\t";
      outputPFTPopulation << "ClippedShootBiomass\tRootBiomass\tRecruitmentBiomass\tExudationBiomass\t";
      outputPFTPopulation << "GPP\tNPP\tRespiration";
      outputPFTPopulation << std::endl;
   }

   if (!outputPlant.is_open())
   {
      utils.handleError("Error writing to the plant / cohorte output file.");
   }
   else
   {
      outputPlant << "Date\tDayCount\tPFT\tAge\tNumberPlants\tHeight\tWidth\tLAI\t";
      outputPlant << "CoveredArea\tRootDepth\tNumberSoilLayers\t";
      outputPlant << "ShootBiomass\tGreenShootBiomass\tBrownShootBiomass\t";
      outputPlant << "ClippedShootBiomass\tRootBiomass\tRecruitmentBiomass\tExudationBiomass\t";
      outputPlant << "GPP\tNPP\tRespiration\t";
      outputPlant << "Radiation\tShadingIndicator\tLimitingFactorWater\tLimitingFactorNitrogen\t";
      outputPlant << "AllocationShoot\tAllocationRoot\tAllocationRecruitment\tAllocationExudation";
      outputPlant << std::endl;
   }
}

/**
 * @brief Writes daily simulation results to the output file.
 *
 * This method writes the daily simulation results, which include state
 * variables of the community, to the output file. The results are stored
 * in a temporary buffer and are written to the file if the file is open.
 * After writing, the buffer is cleared to prepare for the next set of results.
 *
 * @param utils Utility functions for error handling and other utilities.
 *
 * @throws std::ios_base::failure If the output file is not open.
 */
void OUTPUT::writeSimulationResultsToOutputFiles(UTILS utils)
{
   if (outputCommunity.is_open())
   {
      outputCommunity << bufferCommunity.str();
      bufferCommunity.str("");
      bufferCommunity.clear();
   }
   else
   {
      utils.handleError("The output file of community variables is not open for writing.");
   }

   if (outputPFTPopulation.is_open())
   {
      outputPFTPopulation << bufferPFTPopulation.str();
      bufferPFTPopulation.str("");
      bufferPFTPopulation.clear();
   }
   else
   {
      utils.handleError("The output file of PFT population variables is not open for writing.");
   }

   if (outputPlant.is_open())
   {
      outputPlant << bufferPlant.str();
      bufferPlant.str("");
      bufferPlant.clear();
   }
   else
   {
      utils.handleError("The output file of single plant variables / cohorts is not open for writing.");
   }
}

/**
 * @brief Closes the output file.
 *
 * This method closes the output file if it is currently open. If the
 * output file is not open, it triggers an error handling function to
 * notify the user of the issue.
 *
 * @param utils Utility functions for error handling and other utilities.
 *
 * @throws std::ios_base::failure If the output file is not open when
 *                                  attempting to close it.
 */
void OUTPUT::closeOutputFiles(UTILS utils)
{
   if (outputCommunity.is_open())
   {
      outputCommunity.close();
   }
   else
   {
      utils.handleError("The output file of community variables is not open.");
   }

   if (outputPFTPopulation.is_open())
   {
      outputPFTPopulation.close();
   }
   else
   {
      utils.handleError("The output file of PFT population variables is not open.");
   }

   if (outputPlant.is_open())
   {
      outputPlant.close();
   }
   else
   {
      utils.handleError("The output file of single plant variables / cohorts is not open.");
   }
}

/**
 * @brief Creates a folder for the output files.
 *
 * This method constructs an output directory path from the provided path
 * and creates a new directory named "output" within that path. The
 * method uses a utility function to split the input path into components
 * based on the directory separator. If the directory already exists,
 * the method does nothing.
 *
 * @param path The base path where the output folder will be created.
 * @param utils Utility functions for string manipulation and other tasks.
 *
 * @note This method uses the _mkdir function to create the directory.
 *       Ensure that the application has permission to create directories
 *       in the specified location.
 */
void OUTPUT::createOutputFolder(std::string path, UTILS utils)
{
   char separator = '\\';
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 1; it++)
   {
      outputDirectory = outputDirectory + utils.strings.at(it) + "\\";
   }
   outputDirectory = outputDirectory + "output\\";
   _mkdir(outputDirectory.c_str());
}

/**
 * @brief Reads output writing dates from a file.
 *
 * This method constructs the full path to the output writing dates file
 * using the specified path and the filename from the parameters. It opens
 * the file and reads the dates from it, storing them in the outputWritingDates
 * vector. The first line of the file is considered a header and is skipped.
 * If the file cannot be opened, a warning is issued and daily resolution
 * will be used for writing simulation results.
 *
 * @param path The base path where the output writing dates file is located.
 * @param utils Utility functions for string manipulation and calculations.
 * @param parameter Reference to the PARAMETER object containing configuration details.
 *
 * @note The output writing dates file must be formatted correctly, with dates
 *       specified in the format "YYYY-MM-DD". If the file is empty or cannot
 *       be opened, daily simulation results will be written by default.
 */
void OUTPUT::openAndReadOutputWritingDates(std::string path, UTILS utils, PARAMETER &parameter)
{
   char separator = '\\';
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 1; it++)
   {
      fileDirectory = fileDirectory + utils.strings.at(it) + "\\";
   }
   fileDirectory = fileDirectory + parameter.outputWritingDatesFile;

   const char *filename = fileDirectory.c_str();
   std::ifstream file(filename);

   std::string line; // current line text in parser
   int m = 0;        // current line number in parser
   outputWritingDates.clear();

   outputWritingDatesFileOpened = false;
   if (file.is_open())
   {
      outputWritingDatesFileOpened = true;
      while (std::getline(file, line))
      {
         m++;
         if (m > 1)
         { // skip header line
            utils.strings.clear();
            utils.splitString(line, '-');
            int day = std::stoi(utils.strings.at(2).c_str());
            int month = std::stoi(utils.strings.at(1).c_str());
            int year = std::stoi(utils.strings.at(0).c_str());
            outputWritingDates.push_back(utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart));
         }
      }
      file.close();
   }
   else
   {
      utils.handleWarning("No OutputWritingDates file has been opened. Simulation results will then be written at a daily resolution.");
   }
}

/**
 * @brief Prints important settings of the simulation to the console (stdout.txt).
 *
 * This method outputs key configuration parameters of the simulation, including
 * site identification, geographical information, the first and last year of the
 * simulation, and the names of the input files. It also reports whether the input
 * files were successfully opened. Additionally, it details how and when the simulation
 * output will be written, including the random number generator seed.
 *
 * @param parameter Reference to the PARAMETER object containing simulation settings.
 * @param input Reference to the INPUT object containing status information about input files.
 *
 * @note If any input files fail to open, a corresponding message will be printed to
 *       the console. The method also indicates whether output writing dates are set to
 *       daily or at specified dates from the output writing dates file.
 */
void OUTPUT::printSimulationSettingsToConsole(PARAMETER parameter, INPUT input)
{

   std::cout << "*******************************************" << std::endl;
   std::cout << "******* Grassland site simulation *********" << std::endl;
   std::cout << "*******************************************" << std::endl
             << std::endl;

   std::cout << "DEIMS.id = " << parameter.deimsID << std::endl;
   std::cout << "Latitude = " << parameter.latitude << std::endl;
   std::cout << "Longitude = " << parameter.longitude << std::endl;
   std::cout << "First year = " << parameter.firstYear << std::endl;
   std::cout << "Last year = " << parameter.lastYear << std::endl
             << std::endl;

   std::cout << "******* Input files *********" << std::endl
             << std::endl;

   std::cout << "Weather data: " << parameter.weatherFile << std::endl;
   if (!input.weatherFileOpened)
   {
      std::cout << "File failed to be opened!" << std::endl
                << std::endl;
   }
   std::cout << "Soil data: " << parameter.soilFile << std::endl;
   if (!input.soilFileOpened)
   {
      std::cout << "File failed to be opened!" << std::endl
                << std::endl;
   }
   std::cout << "Management data: " << parameter.managementFile << std::endl;
   if (!input.managementFileOpened)
   {
      std::cout << "File failed to be opened!" << std::endl
                << std::endl;
   }
   std::cout << "Plant traits: " << parameter.plantTraitsFile << std::endl
             << std::endl;
   if (!input.plantTraitsFileOpened)
   {
      std::cout << "File failed to be opened!" << std::endl
                << std::endl;
   }

   std::cout << "******* Simulation output writing *********" << std::endl
             << std::endl;

   std::cout << "Seed of random number generator to reproduce simulation output: " << std::to_string(parameter.randomNumberGeneratorSeed) << std::endl
             << std::endl;

   std::string dates = "daily";
   if (outputWritingDatesFileOpened == true)
   {
      dates = "at dates provided in " + parameter.outputWritingDatesFile;
   }
   std::cout << "Simulation output is written: " << dates << std::endl;
   if (!outputWritingDatesFileOpened)
   {
      if (parameter.outputWritingDatesFile != "NaN" && parameter.outputWritingDatesFile != "")
      {
         std::cout << "File on OutputWritingDates failed to be opened!" << std::endl
                   << std::endl;
      }
      else
      {
         std::cout << "File on OutputWritingDates was not included as parameter!" << std::endl
                   << std::endl;
      }
   }

   std::cout << std::endl;
}
