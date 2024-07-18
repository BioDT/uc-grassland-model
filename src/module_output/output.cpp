#include "output.h"

OUTPUT::OUTPUT(){};
OUTPUT::~OUTPUT(){};

/* Creates result folder, output file and its header */
void OUTPUT::prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter)
{
   /* create a folder where result files are written (if not already existing) */
   createOutputFolder(path, utils);
   createAndOpenOutputFiles(parameter, utils);
   writeHeaderInOutputFiles(parameter, utils);
   openAndReadOutputWritingDates(path, utils, parameter);
}

/* Creates the output file */
void OUTPUT::createAndOpenOutputFiles(PARAMETER parameter, UTILS utils)
{
   if (parameter.outputFile)
   {
      utils.strings.clear();
      utils.splitString(parameter.plantTraitsFile, '/');
      std::string endingLocation = std::to_string(parameter.latitude) + "_" + std::to_string(parameter.longitude);
      std::string endingYears = "_" + std::to_string(parameter.firstYear) + "_" + std::to_string(parameter.lastYear) + "_";
      std::string endingParameter = utils.strings.at(1);
      std::string fileEnding = endingLocation + endingYears + endingParameter;
      std::string filename = outputDirectory + "output_" + fileEnding;
      outputFile.open(filename);
      if (!outputFile.is_open())
      {
         utils.handleError("Error writing to the output file.");
      }
   }
};

/* Writes the header in the output file */
void OUTPUT::writeHeaderInOutputFiles(PARAMETER parameter, UTILS utils)
{
   if (parameter.outputFile)
   {
      if (!outputFile.is_open())
      {
         utils.handleError("Error writing to the output file.");
      }
      else
      {
         outputFile << "Simulation results on grassland dynamics (at population and community level)." << std::endl;
         outputFile << "Time\tPFT\tFraction" << std::endl;
      }
   }
}

/* writes daily simulation resulst (state variables of the community) to the output file */
void OUTPUT::writeSimulationResultsToOutputFiles(PARAMETER parameter, UTILS utils, COMMUNITY community)
{
   if (parameter.outputFile)
   {
      if (outputFile.is_open())
      {
         for (auto day : outputWritingDates)
         {
            if (parameter.day == day)
            {
               for (int pft = 0; pft < parameter.pftCount; pft++)
               {
                  outputFile << parameter.day << "\t" << pft << "\t" << community.pftComposition[pft] << std::endl;
               }
            }
         }
      }
      else
      {
         utils.handleError("The output file is not open for writing.");
      }
   }
}

/* Closes the output file */
void OUTPUT::closeOutputFiles(UTILS utils)
{
   if (outputFile.is_open())
   {
      outputFile.close();
   }
   else
   {
      utils.handleError("The output file is not open.");
   }
}

/* Creates a folder for the output file */
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

/* Read-in output writing dates from file */
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

   if (file.is_open())
   {
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
}

/* print important settings of the simulation to the console (stdout.txt) */
void OUTPUT::printSimulationSettingsToConsole(PARAMETER parameter)
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
   std::cout << "Soil data: " << parameter.soilFile << std::endl;
   std::cout << "Management data: " << parameter.managementFile << std::endl;
   std::cout << "Plant traits: " << parameter.plantTraitsFile << std::endl
             << std::endl;

   std::cout << "******* Simulation output writing *********" << std::endl
             << std::endl;

   std::cout << "Seed of random number generator to reproduce simulation output: " << std::to_string(parameter.randomNumberGeneratorSeed) << std::endl;

   std::string yesNo = (parameter.outputFile == true) ? "Yes" : "No";
   std::string dates = (parameter.outputFile == true) ? (" (at dates provided in " + parameter.outputWritingDatesFile + ")") : "";
   std::cout << "Write output file? " << yesNo << dates << std::endl
             << std::endl;

   std::cout << std::endl
             << std::endl;
}