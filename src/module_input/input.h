#pragma once
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_soil/soil.h"
#include "../module_management/management.h"
#include "../utils/utils.h"
#include "../module_init/constants.h"
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include <limits>
#include <filesystem>

class INPUT
{
public:
   INPUT();
   ~INPUT();

   std::string plantTraitsDirectory;
   std::string weatherDirectory;
   std::string manageDirectory;
   std::string soilDirectory;

   bool plantTraitsFileOpened;
   bool weatherFileOpened;
   bool managementFileOpened;
   bool soilFileOpened;

   /* vectors to store information of parsed lines for parameters */
   std::vector<std::string> lineValues;        // stored parameter text lines
   std::vector<std::string> lineTypeValues;    // stored datatype lines of parameters
   std::vector<int> lineNumbers;               // stored line numbers of parameters
   std::vector<int> keywordLineNumbers;        // vector which stores only those lines that start with the correct format i.e. parameterName (here keyword)
   std::vector<std::string> keywordLineValues; // vector which stores only values of those lines that start with the correct format i.e. parameterName (here keyword)
   std::string parameterValue;
   std::string parameterType;

   /* auxillary structure to map parsed parameter to its name */
   static std::map<std::string, int> configParInt;
   static std::map<std::string, float> configParFloat;
   static std::map<std::string, bool> configParBool;
   static std::map<std::string, std::string> configParString;

   /* functions of the INPUT class */
   void getInputData(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather, SOIL &soil, MANAGEMENT &management);
   void openAndReadConfigurationFile(std::string config, UTILS utils, PARAMETER &parameter);
   void openAndReadPlantTraitsFile(std::string config, UTILS utils, PARAMETER &parameter);
   void openAndReadWeatherFile(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather);
   void openAndReadSoilFile(std::string path, UTILS utils, PARAMETER &parameter, SOIL &soil);
   void openAndReadManagementFile(std::string path, UTILS utils, PARAMETER &parameter, MANAGEMENT &management);

   void searchParameterInInputFile(std::string keyword, const char *filename, UTILS utils);
   void checkIfParameterExistsAndExtractValues(UTILS utils, std::string keyword, std::vector<std::string> lineValues, std::vector<int> lineNumbers, std::vector<std::string> lineTypeValues);
   void extractLinesOfCorrectFormat(UTILS utils, std::string keyword, std::vector<std::string> lineValues);
   void extractDataTypeForExtractedValue(UTILS utils, std::string keyword);
   void convertAndCheckAndSetParameterValue(UTILS utils, std::string keyword, std::string parameterType, PARAMETER parameter);
   void transferConfigParameterValueToModelParameter(PARAMETER &parameter, UTILS utils);
   void transferPlantTraitsParameterValueToModelParameter(PARAMETER &parameter);
};
