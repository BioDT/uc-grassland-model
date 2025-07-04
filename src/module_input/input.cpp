#include "input.h"

INPUT::INPUT() {};
INPUT::~INPUT() {};

std::map<std::string, int> INPUT::configParInt;
std::map<std::string, float> INPUT::configParFloat;
std::map<std::string, bool> INPUT::configParBool;
std::map<std::string, std::string> INPUT::configParString;

/* stream all relevant input data */
void INPUT::getInputData(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather, SOIL &soil, MANAGEMENT &management)
{
   openAndReadConfigurationFile(path, utils, parameter);
   openAndReadPlantTraitsFile(path, utils, parameter);
   openAndReadWeatherFile(path, utils, parameter, weather);
   openAndReadSoilFile(path, utils, parameter, soil);
   openAndReadManagementFile(path, utils, parameter, management);
}

/* open and read configuration file */
void INPUT::openAndReadConfigurationFile(std::string config, UTILS utils, PARAMETER &parameter)
{
   const char *filename = config.c_str();
   for (auto par : parameter.configParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
   {
      /* open file and search for name in all lines */
      searchParameterInInputFile(par, filename, utils);

      /* check if the parameter name was found at least once and read in each line */
      checkIfParameterExistsAndExtractValues(utils, par, lineValues, lineNumbers, lineTypeValues);

      /* get the corresponding datatype for the extracted parameter value */
      extractDataTypeForExtractedValue(utils, par);

      /* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
      convertAndCheckAndSetParameterValue(utils, par, parameterType, parameter);
   }

   /* transfer the mapped values of all parameter names to their variables in class PARAMETER */
   transferConfigParameterValueToModelParameter(parameter, utils);
}

/* open file and search for name (keyword) in all lines */
void INPUT::searchParameterInInputFile(std::string keyword, const char *filename, UTILS utils)
{
   std::string line;   // current line text in parser
   int lineNumber = 0; // current line number in parser
   bool found = false; // was the keyword found in a streamed line?

   lineValues.clear();
   lineTypeValues.clear();
   lineNumbers.clear();

   /* detect all lines in which the keyword is found */
   std::ifstream file(filename);

   if (file.is_open())
   {
      found = false;
      while (std::getline(file, line))
      {
         if (found) /* if in the previous line the keyword was found, save the next line as datatype in lineTypeValues */
         {
            lineTypeValues.push_back(line);
            found = false;
         }

         lineNumber++;
         if (line.find(keyword) != std::string::npos) /* if the keyword is found in this line, save line number and line text in vectors, set found to true so that next line on datatype is also saved */
         {
            lineNumbers.push_back(lineNumber);
            lineValues.push_back(line);
            found = true;
         }
      }
      file.close();
   }
   else
   {
      std::string s = filename;
      utils.handleError("Cannot open the file" + s + ". There is no simuation possible. Please check if the file exists.");
   }
}

/* check if the parameter name (keyowrd) was found at least once */
void INPUT::checkIfParameterExistsAndExtractValues(UTILS utils, std::string keyword, std::vector<std::string> lineValues, std::vector<int> lineNumbers, std::vector<std::string> lineTypeValues)
{
   /* if parameter name (keyword) was not found in the input file */
   if (lineNumbers.size() == 0)
   {
      utils.handleError("The parameter " + keyword + " is missing in the input file. Please check the file!");
   }

   /* extract for each line with the found keyword only those that have the correct format */
   if (lineNumbers.size() >= 1)
   {
      extractLinesOfCorrectFormat(utils, keyword, lineValues);
   }
}

/* read in each line, split string and save correct formatted parameter values in variables */
void INPUT::extractLinesOfCorrectFormat(UTILS utils, std::string keyword, std::vector<std::string> lineValues)
{
   int index = 0;                          /* index used to track which of the found lines show the correct format of the found parameter (e.g. if found more than once) */
   std::vector<std::string> valueElements; /* first splitted line string by tabstopp */
   std::vector<std::string> cleanLine;     /* remove additional free spaces if they occur between the name and value of the parameter */

   keywordLineNumbers.clear(); // all lines where the parameter was found in the correct format
   keywordLineValues.clear();

   for (auto line : lineValues)
   {
      cleanLine.clear();
      utils.strings.clear();
      utils.splitString(line, '\t'); /* split string based on tabstop */
      valueElements.clear();
      for (auto it : utils.strings)
      {
         valueElements.push_back(it); /* save splitted string elements because ut.strings will be cleared at next step */
      }

      for (auto it : valueElements)
      {
         utils.strings.clear();
         utils.splitString(it, ' '); /* split string based on free space */
         for (auto word : utils.strings)
         {
            if (word != "")
            {
               cleanLine.push_back(word); /* now no more free spaces should be included */
            }
         }
      }

      if (cleanLine.at(0) == keyword) // the correct format: (cleaned) line should start with the parameter name (here, keyword) followed by its value
      {
         if (cleanLine.size() == 1)
         {
            utils.handleError("A value is missing for the parameter " + keyword + ". Please check the input file!");
         }
         else
         {
            keywordLineNumbers.push_back(index);
            for (int i = 1; i < cleanLine.size(); i++)
            {
               keywordLineValues.push_back(cleanLine.at(i));
            }
         }
      }
      else
      {
         if (lineValues.size() > 1) /* case: there are more than one line in the input file where the parameter is mentioned */
         {
            utils.handleWarning("The parameter " + keyword + " is mentioned several times. Please check the input file!");
         }
         else /* case: there is only one line in the input file where the parameter is mentioned and its the false format */
         {
            utils.handleError("The parameter " + keyword + " is either missing or in a wrong format. Please check the input file!");
         }
      }
      index++;
   }

   if (keywordLineNumbers.size() == 0) /* none of the identified lines shows a correct format of the parameter */
   {
      utils.handleError("The parameter " + keyword + " is either missing or in a wrong format. Please check the input file!");
   }
   else if (keywordLineNumbers.size() > 0) /* the parameter occurs at least once in the correct format */
   {

      if (keywordLineNumbers.size() > 1) /* the parameter occurs more than once in the correct format */
      {
         utils.handleError("The parameter " + keyword + " occurs more than once in the input file. Please check the input file!");
      }
   }
}

/* get the corresponding datatype for the extracted parameter value */
void INPUT::extractDataTypeForExtractedValue(UTILS utils, std::string keyword)
{
   if (keywordLineNumbers.size() > 0)
   {
      utils.strings.clear();
      utils.splitString(lineTypeValues.at(keywordLineNumbers.at(0)), ':'); // now strings should have 2 elements: "\datatype" type

      if (utils.strings.size() > 0)
      {
         if (utils.strings.at(0) != "\\datatype") // at least on index 2 of vector strings can be found
         {
            utils.handleError("The line following the parameter value for " + keyword + " does not include the datatype. Please check the input file!");
         }
         else
         {
            if (utils.strings.size() == 2)
            {
               parameterType = utils.strings.at(1);
            }
            else
            {
               utils.handleError("The datatype is missing for the parameter " + keyword + ". Please check the input file!");
            }
         }
      }
      else
      {
         utils.handleError("The datatype is missing for the parameter " + keyword + ". Please check the input file!");
      }
   }
}

/* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
void INPUT::convertAndCheckAndSetParameterValue(UTILS utils, std::string keyword, std::string parameterType, PARAMETER parameter)
{
   if (parameterType == "integer")
   {
      try
      {

         int value = utils.parseIntegerOrNaN(keywordLineValues.at(0));
         if (value < 0 && keyword != "randomNumberGeneratorSeed")
         {
            throw std::out_of_range("Value of parameter " + keyword + " is outside the valid range! Value is not allowed to be negative!");
         }
         else
         {
            configParInt[keyword] = value;
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParInt[keyword] = -1;
      }
   }
   else if (parameterType == "float")
   {
      try
      {
         float value = std::stof(keywordLineValues.at(0));
         if (value < 0)
         {
            throw std::out_of_range("Value of parameter " + keyword + " is outside the valid range! Value is not allowed to be negative!");
         }
         else
         {
            configParFloat[keyword] = value;
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParFloat[keyword] = -1;
      }
   }
   else if (parameterType == "date")
   {
      try
      {
         utils.strings.clear();
         utils.splitString(keywordLineValues.at(0), '-');
         int day = std::stoi(utils.strings.at(2));
         int month = std::stoi(utils.strings.at(1));
         int year = std::stoi(utils.strings.at(0));

         // calculate given day as count from first simulated day
         int dayCount = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);
         if (dayCount < 0)
         {
            throw std::out_of_range("Value of parameter " + keyword + " is outside the valid range! Please check the date!");
         }
         else
         {
            configParInt[keyword] = dayCount;
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParInt[keyword] = -1;
      }
   }
   else if (parameterType == "boolean")
   {
      try
      {
         bool value = utils.stringToBool(keywordLineValues.at(0));
         if (value != true && value != false)
         {
            throw std::out_of_range("Value of parameter " + keyword + " is outside the valid range!");
         }
         else
         {
            configParBool[keyword] = value;
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParBool[keyword] = false;
      }
   }
   else if (parameterType == "string")
   {
      try
      {
         if (keywordLineValues.at(0) == "")
         {
            if (keyword == "deimsID" || keyword == "outputWritingDatesFile")
            {
               throw std::out_of_range("Value of parameter " + keyword + " is not a string! If not in use or available, write at least NA in the parameter file.");
            }
            else
            {
               throw std::out_of_range("Value of parameter " + keyword + " is not a string! Please add an existing filename.");
            }
         }
         else if (keyword != "deimsID" && keyword != "outputWritingDatesFile" && keywordLineValues.at(0) == "NA")
         {
            throw std::out_of_range("Value of parameter " + keyword + " is an invalid string! Please add an existing filename.");
         }

         if (keyword != "deimsID" && keyword != "latitude" && keyword != "longitude")
         {
            if (!(keyword == "outputWritingDatesFile" && keywordLineValues.at(0) == "NaN"))
            {
               std::string fileEnding = "";
               fileEnding = utils.getFileEnding(keywordLineValues.at(0));
               if (fileEnding != "txt")
               {
                  configParString[keyword] = keywordLineValues.at(0) + ".txt";
               }
               else
               {
                  configParString[keyword] = keywordLineValues.at(0);
               }
            }
            else
            {
               configParString[keyword] = keywordLineValues.at(0);
            }
         }
         else
         {
            configParString[keyword] = keywordLineValues.at(0);
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParString[keyword] = "";
      }
      catch (const std::invalid_argument &e)
      {
         utils.handleError(e.what());
         configParString[keyword] = "";
      }
   }
   else if (parameterType == "integer-array")
   {
      int value;
      std::string array_pos;
      try
      {
         for (int i = 0; i < keywordLineValues.size(); i++)
         {
            array_pos = std::to_string(i);
            value = std::stoi(keywordLineValues.at(i));
            if (value < 0)
            {
               throw std::out_of_range("Value of parameter " + keyword + array_pos.c_str() + " is outside the valid range! Value is not allowed to be negative!");
            }
            else
            {
               configParInt[keyword + array_pos.c_str()] = value;
            }
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParInt[keyword + array_pos.c_str()] = -1;
      }
   }
   else if (parameterType == "float-array")
   {
      float value;
      std::string array_pos;
      try
      {
         for (int i = 0; i < keywordLineValues.size(); i++)
         {
            array_pos = std::to_string(i);
            value = std::stof(keywordLineValues.at(i));
            if (value < 0)
            {
               throw std::out_of_range("Value of parameter " + keyword + array_pos.c_str() + " is outside the valid range! Value is not allowed to be negative!");
            }
            else
            {
               configParFloat[keyword + array_pos.c_str()] = value;
            }
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParFloat[keyword + array_pos.c_str()] = -1;
      }
   }
   else if (parameterType == "boolean-array")
   {
      int value;
      std::string array_pos;
      try
      {
         for (int i = 0; i < keywordLineValues.size(); i++)
         {
            array_pos = std::to_string(i);
            value = std::stoi(keywordLineValues.at(i));
            if (value < 0)
            {
               throw std::out_of_range("Value of parameter " + keyword + array_pos.c_str() + " is outside the valid range! Value is not allowed to be negative!");
            }
            else
            {
               configParBool[keyword + array_pos.c_str()] = value;
            }
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParBool[keyword + array_pos.c_str()] = -1;
      }
   }
   else if (parameterType == "string-array")
   {
      std::string value;
      std::string array_pos;
      try
      {
         for (int i = 0; i < keywordLineValues.size(); i++)
         {
            array_pos = std::to_string(i);
            value = keywordLineValues.at(i);
            configParString[keyword + array_pos.c_str()] = value;
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParString[keyword + array_pos.c_str()] = "NA";
      }
   }
   else if (parameterType == "date-array")
   {
      std::string array_pos;
      try
      {
         for (int i = 0; i < keywordLineValues.size(); i++)
         {
            array_pos = std::to_string(i);

            utils.strings.clear();
            utils.splitString(keywordLineValues.at(i), '-');
            int day = std::stoi(utils.strings.at(2));
            int month = std::stoi(utils.strings.at(1));
            int year = std::stoi(utils.strings.at(0));

            // calculate given day as count from first simulated day
            int dayCount = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);
            if (dayCount < 0)
            {
               throw std::out_of_range("Value of parameter " + keyword + array_pos.c_str() + " is outside the valid range! Please check the date!");
            }
            else
            {
               configParInt[keyword + array_pos.c_str()] = dayCount;
            }
         }
      }
      catch (const std::out_of_range &e)
      {
         utils.handleError(e.what());
         configParInt[keyword + array_pos.c_str()] = -1;
      }
   }
   else
   {
      utils.handleError("No valid datatype for the parameter " + keyword + ". Please check the input file!");
   }
}

/* transfer the mapped values of all config parameter names to their variables in class PARAMETER */
void INPUT::transferConfigParameterValueToModelParameter(PARAMETER &parameter, UTILS utils)
{
   parameter.deimsID = configParString["deimsID"];
   parameter.latitude = configParString["latitude"];
   parameter.longitude = configParString["longitude"];
   parameter.weatherFile = configParString["weatherFile"];
   parameter.soilFile = configParString["soilFile"];
   parameter.managementFile = configParString["managementFile"];
   parameter.plantTraitsFile = configParString["plantTraitsFile"];
   parameter.outputWritingDatesFile = configParString["outputWritingDatesFile"];
   parameter.clippingHeightOfBiomassMeasurement = configParFloat["clippingHeightOfBiomassMeasurement"];
   parameter.randomNumberGeneratorSeed = configParInt["randomNumberGeneratorSeed"];
   parameter.firstYear = configParInt["firstYear"];
   parameter.lastYear = configParInt["lastYear"];
   // calculate reference julian days (1 Jan of param.firstYear and 31 Dec of param.lastYear)
   parameter.referenceJulianDayStart = utils.calculateJulianDayFromDate(1, 1, parameter.firstYear);
   parameter.referenceJulianDayEnd = utils.calculateJulianDayFromDate(31, 12, parameter.lastYear);
   parameter.simulationTimeInDays = parameter.referenceJulianDayEnd - parameter.referenceJulianDayStart + 1;
}

/* transfer the mapped values of all plant trait parameter names to their variables in class PARAMETER */
void INPUT::transferPlantTraitsParameterValueToModelParameter(PARAMETER &parameter)
{
   parameter.pftCount = configParInt["pftCount"];

   /* parameters independent of species or PFT */
   parameter.crowdingMortalityActivated = configParBool["crowdingMortalityActivated"];
   parameter.externalSeedInfluxActivated = configParBool["externalSeedInfluxActivated"];
   parameter.dayOfExternalSeedInfluxStart = configParInt["dayOfExternalSeedInfluxStart"];
   parameter.seedsFromMaturePlantsActivated = configParBool["seedsFromMaturePlantsActivated"];
   parameter.useStaticShootRootAllocationRates = configParBool["useStaticShootRootAllocationRates"];
   parameter.brownBiomassFractionFalling = configParFloat["brownBiomassFractionFalling"];
   parameter.plantResponseToTemperatureQ10Base = configParFloat["plantResponseToTemperatureQ10Base"];
   parameter.plantResponseToTemperatureQ10Reference = configParFloat["plantResponseToTemperatureQ10Reference"];
   parameter.plantCostRhizobiaSymbiosis = configParFloat["plantCostRhizobiaSymbiosis"];
   parameter.growthRespirationFraction = configParFloat["growthRespirationFraction"];
   parameter.maintenanceRespirationRate = configParFloat["maintenanceRespirationRate"];

   /* parameters dependent on species or PFT */
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      std::string array_pos = std::to_string(pft);
      parameter.maximumPlantHeight.push_back(configParInt["maximumPlantHeight" + array_pos]);
      parameter.plantHeightToWidthRatio.push_back(configParFloat["plantHeightToWidthRatio" + array_pos]);
      parameter.plantShootCorrectionFactor.push_back(configParFloat["plantShootCorrectionFactor" + array_pos]);
      parameter.plantShootRootRatio.push_back(configParFloat["plantShootRootRatio" + array_pos]);
      parameter.plantRootDepthParamIntercept.push_back(configParFloat["plantRootDepthParamIntercept" + array_pos]);
      parameter.plantRootDepthParamExponent.push_back(configParFloat["plantRootDepthParamExponent" + array_pos]);
      parameter.plantSpecificLeafArea.push_back(configParFloat["plantSpecificLeafArea" + array_pos]);
      parameter.plantShootOverlapFactors.push_back(configParFloat["plantShootOverlapFactors" + array_pos]);
      parameter.rootLifeSpan.push_back(configParInt["rootLifeSpan" + array_pos]);
      parameter.leafLifeSpan.push_back(configParInt["leafLifeSpan" + array_pos]);
      parameter.plantLifeSpan.push_back(configParString["plantLifeSpan" + array_pos]);
      parameter.plantMortalityProbability.push_back(configParFloat["plantMortalityProbability" + array_pos]);
      parameter.seedlingMortalityProbability.push_back(configParFloat["seedlingMortalityProbability" + array_pos]);
      parameter.seedGerminationTimes.push_back(configParInt["seedGerminationTimes" + array_pos]);
      parameter.seedGerminationRates.push_back(configParFloat["seedGerminationRates" + array_pos]);
      parameter.seedMasses.push_back(configParFloat["seedMasses" + array_pos]);
      parameter.maturityAges.push_back(configParFloat["maturityAges" + array_pos]);
      parameter.maturityHeights.push_back(configParFloat["maturityHeights" + array_pos]);
      parameter.externalSeedInfluxNumber.push_back(configParInt["externalSeedInfluxNumber" + array_pos]);
      parameter.maximumGrossLeafPhotosynthesisRate.push_back(configParFloat["maximumGrossLeafPhotosynthesisRate" + array_pos]);
      parameter.initialSlopeOfLightResponseCurve.push_back(configParFloat["initialSlopeOfLightResponseCurve" + array_pos]);
      parameter.lightExtinctionCoefficients.push_back(configParFloat["lightExtinctionCoefficients" + array_pos]);
      parameter.plantNppAllocationGrowth.push_back(configParFloat["plantNppAllocationGrowth" + array_pos]);
      parameter.plantNppAllocationExudation.push_back(configParFloat["plantNppAllocationExudation" + array_pos]);
      parameter.plantCNRatioGreenLeaves.push_back(configParFloat["plantCNRatioGreenLeaves" + array_pos]);
      parameter.plantCNRatioBrownLeaves.push_back(configParFloat["plantCNRatioBrownLeaves" + array_pos]);
      parameter.plantCNRatioRoots.push_back(configParFloat["plantCNRatioRoots" + array_pos]);
      parameter.plantCNRatioSeeds.push_back(configParFloat["plantCNRatioSeeds" + array_pos]);
      parameter.plantCNRatioExudates.push_back(configParFloat["plantCNRatioExudates" + array_pos]);
      parameter.nitrogenFixationAbility.push_back(configParBool["nitrogenFixationAbility" + array_pos]);
      parameter.plantWaterUseEfficiency.push_back(configParFloat["plantWaterUseEfficiency" + array_pos]);
      parameter.plantMinimalSoilWaterForGppReduction.push_back(configParFloat["plantMinimalSoilWaterForGppReduction" + array_pos]);
      parameter.plantMaximalSoilWaterForGppReduction.push_back(configParFloat["plantMaximalSoilWaterForGppReduction" + array_pos]);
   }
}

/* open and read plant traits parameter file */
void INPUT::openAndReadPlantTraitsFile(std::string path, UTILS utils, PARAMETER &parameter)
{
   char separator = '\\';
   utils.strings.clear();
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 3; it++)
   {
      plantTraitsDirectory = plantTraitsDirectory + utils.strings.at(it) + "\\";
   }
   plantTraitsDirectory = plantTraitsDirectory + "parameters\\" + parameter.plantTraitsFile;
   const char *filename = plantTraitsDirectory.c_str();

   for (auto par : parameter.plantTraitsParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
   {
      /* open file and search for name in all lines */
      searchParameterInInputFile(par, filename, utils);

      /* check if the parameter name was found at least once and read in each line */
      checkIfParameterExistsAndExtractValues(utils, par, lineValues, lineNumbers, lineTypeValues);

      /* get the corresponding datatype for the extracted parameter value */
      extractDataTypeForExtractedValue(utils, par);

      /* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
      convertAndCheckAndSetParameterValue(utils, par, parameterType, parameter);
   }

   /* transfer the mapped values of all parameter names to their variables in class PARAMETER */
   transferPlantTraitsParameterValueToModelParameter(parameter);
}

/* read-in weather variables from input file */
void INPUT::openAndReadWeatherFile(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather)
{
   char separator = '\\';
   utils.strings.clear();
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 3; it++)
   {
      weatherDirectory = weatherDirectory + utils.strings.at(it) + "\\";
   }
   std::string location = utils.strings.at(utils.strings.size() - 1);
   separator = '_';
   utils.strings.clear();
   utils.splitString(location, separator);
   location = utils.strings.at(0) + "_" + utils.strings.at(1);

   weatherDirectory = weatherDirectory + "scenarios\\" + location + "\\weather\\" + parameter.weatherFile;
   const char *filename = weatherDirectory.c_str();

   weather.weatherDates.clear();
   weather.precipitation.clear();
   weather.fullDayAirTemperature.clear();
   weather.dayTimeAirTemperature.clear();
   weather.photosyntheticPhotonFluxDensity.clear();
   weather.dayLength.clear();
   weather.potEvapoTranspiration.clear();

   std::string line;  // current line text in parser
   int m = 0;         // current line number in parser
   const char *value; // placeholder for extracted value from file

   weatherFileOpened = false;
   std::string startDate = std::to_string(parameter.firstYear) + "-01-01";
   std::string endDate = std::to_string(parameter.lastYear) + "-12-31";
   bool foundStartDate = false;
   bool foundEndDate = false;

   std::ifstream file(filename);
   if (file.is_open())
   {
      weatherFileOpened = true;
      while (std::getline(file, line))
      {
         m++;
         if (m > 1)
         { // skip header line
            char separator = '\t';
            utils.strings.clear();
            utils.splitString(line, separator);

            if (utils.strings.size() == 7)
            {
               weather.weatherDates.push_back(utils.strings.at(0));
               if (utils.strings.at(0) == startDate)
               {
                  foundStartDate = true;
               }

               if (foundStartDate && !foundEndDate)
               {
                  value = utils.strings.at(1).c_str();
                  weather.precipitation.push_back(atof(value));

                  value = utils.strings.at(2).c_str();
                  weather.fullDayAirTemperature.push_back(atof(value));

                  value = utils.strings.at(3).c_str();
                  weather.dayTimeAirTemperature.push_back(atof(value));

                  value = utils.strings.at(4).c_str();
                  weather.photosyntheticPhotonFluxDensity.push_back(atof(value));

                  value = utils.strings.at(5).c_str();
                  weather.dayLength.push_back(atof(value));

                  value = utils.strings.at(6).c_str();
                  weather.potEvapoTranspiration.push_back(atof(value));
               }

               if (utils.strings.at(0) == endDate)
               {
                  foundEndDate = true;
               }
            }
            else
            {
               utils.handleError("Values are missing in the weather input file in line " + std::to_string(m) + ". Please check the entry to be five values separated by tabulator.");
            }
         }
      }
      file.close();

      // check if first and last date (from parameters of configuration file) are included in the weather time series
      if (!foundStartDate || !foundEndDate)
      {
         utils.handleError("Error (weather input): the simulation period as specified in the configuration file is not included in the weather file.");
      }
   }
   else
   {
      utils.handleError("Error (weather input): The weather file cannot be opened. Please check the name in the configuration file.");
   }
}

/* read-in management information from input file */
void INPUT::openAndReadManagementFile(std::string path, UTILS utils, PARAMETER &parameter, MANAGEMENT &management)
{
   char separator = '\\';
   utils.strings.clear();
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 3; it++)
   {
      manageDirectory = manageDirectory + utils.strings.at(it) + "\\";
   }
   std::string location = utils.strings.at(utils.strings.size() - 1);
   separator = '_';
   utils.strings.clear();
   utils.splitString(location, separator);
   location = utils.strings.at(0) + "_" + utils.strings.at(1);

   manageDirectory = manageDirectory + "scenarios\\" + location + "\\management\\" + parameter.managementFile;
   const char *filename = manageDirectory.c_str();

   management.mowingDate.clear();
   management.mowingHeight.clear();

   management.fertilizationDate.clear();
   management.fertilizerAmount.clear();

   management.irrigationDate.clear();
   management.irrigationAmount.clear();

   management.sowingDate.clear();
   management.amountOfSownSeeds.clear(); // 2D vector
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      management.amountOfSownSeeds.push_back(std::vector<int>()); // add rows according to the number of pfts from configuration file
   }

   std::string line;                      // current line text in parser
   int m = 0;                             // current line number in parser
   std::string valueDate;                 // placeholder for extracted value from file (date of a specific managent action)
   double valueActionMowing = NAN;        // placeholder for extracted value from file (management action)
   double valueActionFertilization = NAN; // placeholder for extracted value from file (management action)
   double valueActionIrrigation = NAN;    // placeholder for extracted value from file (management action)
   double valueActionSowingActivated = 0; // placeholder for extracted value from file (management action)
   std::vector<double> valueActionSowing; // placeholder for extracted value from file (management action)
   std::string valueInformation;

   managementFileOpened = false;
   std::ifstream file(filename);
   if (file.is_open())
   {
      managementFileOpened = true;
      while (std::getline(file, line))
      {
         m++;
         if (m > 1)
         { // skip header line
            char separator = '\t';
            utils.strings.clear();
            utils.splitString(line, separator);

            if (utils.strings.size() == (4 + parameter.pftCount + 1))
            {

               valueDate = utils.strings.at(0);
               try
               {
                  valueActionMowing = utils.parseDoubleOrNaN(utils.strings.at(1).c_str());
               }
               catch (const std::invalid_argument &e)
               {
                  std::cerr << e.what() << std::endl;
               }
               try
               {
                  valueActionFertilization = utils.parseDoubleOrNaN(utils.strings.at(2).c_str());
               }
               catch (const std::invalid_argument &e)
               {
                  std::cerr << e.what() << std::endl;
               }
               try
               {
                  valueActionIrrigation = utils.parseDoubleOrNaN(utils.strings.at(3).c_str());
               }
               catch (const std::invalid_argument &e)
               {
                  std::cerr << e.what() << std::endl;
               }

               valueActionSowingActivated = 0;
               valueActionSowing.clear();
               for (int pft = 0; pft < parameter.pftCount; pft++)
               {
                  try
                  {
                     double sowPFT = utils.parseDoubleOrNaN(utils.strings.at(4 + pft).c_str());
                     if (!std::isnan(sowPFT)) /* if at least one PFT is sown, valueActionSowingActivated = 1*/
                     {
                        valueActionSowingActivated = 1;
                        valueActionSowing.push_back(atof(utils.strings.at(4 + pft).c_str()));
                     }
                     else /* the PFT is not sown at this day */
                     {
                        valueActionSowing.push_back(0);
                     }
                  }
                  catch (const std::invalid_argument &e)
                  {
                     std::cerr << e.what() << std::endl;
                  }
               }

               try
               {
                  valueInformation = utils.strings.at(4 + parameter.pftCount);
               }
               catch (const std::invalid_argument &e)
               {
                  std::cerr << e.what() << std::endl;
               }

               // mowing events
               if (!std::isnan(valueActionMowing))
               {
                  utils.strings.clear();
                  utils.splitString(valueDate, '-');
                  if (utils.strings.size() == 3)
                  {
                     int day = std::stoi(utils.strings.at(2).c_str());
                     int month = std::stoi(utils.strings.at(1).c_str());
                     int year = std::stoi(utils.strings.at(0).c_str());

                     int mowDay = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);

                     if (mowDay > 0 && mowDay < parameter.simulationTimeInDays)
                     {
                        management.mowingDate.push_back(mowDay);
                        management.mowingHeight.push_back(valueActionMowing);
                     }
                     else
                     {
                        utils.handleWarning("Mowing date " + valueDate + " is outside the simulation period and not used in this simulation.");
                     }
                  }
                  else
                  {
                     utils.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                  }
               }

               // fertilization events
               if (!std::isnan(valueActionFertilization))
               {
                  utils.strings.clear();
                  utils.splitString(valueDate, '-');
                  if (utils.strings.size() == 3)
                  {
                     int day = std::stoi(utils.strings.at(2).c_str());
                     int month = std::stoi(utils.strings.at(1).c_str());
                     int year = std::stoi(utils.strings.at(0).c_str());

                     int fertDay = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);
                     if (fertDay > 0 && fertDay < parameter.simulationTimeInDays)
                     {
                        management.fertilizationDate.push_back(fertDay);
                        management.fertilizerAmount.push_back(valueActionFertilization);
                     }
                     else
                     {
                        utils.handleWarning("Fertilization date " + valueDate + " is outside the simulation period and not used in this simulation.");
                     }
                  }
                  else
                  {
                     utils.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                  }
               }

               // irrigation events
               if (!std::isnan(valueActionIrrigation))
               {
                  utils.strings.clear();
                  utils.splitString(valueDate, '-');
                  if (utils.strings.size() == 3)
                  {
                     int day = std::stoi(utils.strings.at(2).c_str());
                     int month = std::stoi(utils.strings.at(1).c_str());
                     int year = std::stoi(utils.strings.at(0).c_str());

                     int irrigDay = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);
                     if (irrigDay > 0 && irrigDay < parameter.simulationTimeInDays)
                     {
                        management.irrigationDate.push_back(irrigDay);
                        management.irrigationAmount.push_back(valueActionIrrigation);
                     }
                     else
                     {
                        utils.handleWarning("Irrigation date " + valueDate + " is outside the simulation period and not used in this simulation.");
                     }
                  }
                  else
                  {
                     utils.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                  }
               }

               // seed sowing events
               if (valueActionSowingActivated > 0) // only if at least one PFT is sown (sum > 0)
               {
                  utils.strings.clear();
                  utils.splitString(valueDate, '-');
                  int day = std::stoi(utils.strings.at(2).c_str());
                  int month = std::stoi(utils.strings.at(1).c_str());
                  int year = std::stoi(utils.strings.at(0).c_str());

                  int sowDay = utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart);
                  if (sowDay > 0 && sowDay < parameter.simulationTimeInDays)
                  {
                     management.sowingDate.push_back(sowDay);
                     for (int pft = 0; pft < parameter.pftCount; pft++)
                     {
                        management.amountOfSownSeeds[pft].push_back((int)valueActionSowing[pft]);
                     }
                  }
                  else
                  {
                     utils.handleWarning("Sowing date " + valueDate + " is outside the simulation period and not used in this simulation.");
                  }
               }
            }
            else
            {
               utils.handleError("Values are missing in the management input file in line " + std::to_string(m) + ". Please check the entries.");
            }
         }
      }
      file.close();
   }
   else
   {
      utils.handleError("Error (management input): The management file cannot be opened. Please check the name in the configuration file.");
   }
}

/* Reads-in soil parameters from input file */
void INPUT::openAndReadSoilFile(std::string path, UTILS utils, PARAMETER &parameter, SOIL &soil)
{
   char separator = '\\';
   utils.strings.clear();
   utils.splitString(path, separator);
   for (int it = 0; it < utils.strings.size() - 3; it++)
   {
      soilDirectory = soilDirectory + utils.strings.at(it) + "\\";
   }
   std::string location = utils.strings.at(utils.strings.size() - 1);
   separator = '_';
   utils.strings.clear();
   utils.splitString(location, separator);
   location = utils.strings.at(0) + "_" + utils.strings.at(1);

   soilDirectory = soilDirectory + "scenarios\\" + location + "\\soil\\" + parameter.soilFile;
   const char *filename = soilDirectory.c_str();

   soil.siltContent = -1;
   soil.sandContent = -1;
   soil.clayContent = -1;

   soil.permanentWiltingPoint.clear();
   soil.fieldCapacity.clear();
   soil.porosity.clear();
   soil.saturatedHydraulicConductivity.clear();

   std::string line;  // current line text in parser
   int m = 0;         // current line number in parser
   const char *value; // placeholder for extracted value from file

   std::ifstream file(filename);
   if (file.is_open())
   {
      while (std::getline(file, line))
      {
         m++;
         if (m == 2)
         { // skip header line
            char separator = '\t';
            utils.strings.clear();
            utils.splitString(line, separator);

            if (utils.strings.size() == 3)
            {
               value = utils.strings.at(0).c_str();
               soil.siltContent = atof(value);

               value = utils.strings.at(1).c_str();
               soil.clayContent = atof(value);

               value = utils.strings.at(2).c_str();
               soil.sandContent = atof(value);
            }
            else
            {
               utils.handleError("Values are missing in the soil input file in line " + std::to_string(m) + ". Please check the entries to be exactly three values separated by tabulator.");
            }
         }

         if (m >= 5)
         { // skip header lines above
            char separator = '\t';
            utils.strings.clear();
            utils.splitString(line, separator);

            if (utils.strings.size() == 5)
            {
               // skip layer number in column 1
               value = utils.strings.at(1).c_str();
               soil.fieldCapacity.push_back(atof(value));

               value = utils.strings.at(2).c_str();
               soil.permanentWiltingPoint.push_back(atof(value));

               value = utils.strings.at(3).c_str();
               soil.porosity.push_back(atof(value));

               value = utils.strings.at(4).c_str();
               soil.saturatedHydraulicConductivity.push_back(atof(value));

               if (soil.fieldCapacity.at(soil.fieldCapacity.size() - 1) < 0 || soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) < 0 ||
                   soil.porosity.at(soil.porosity.size() - 1) < 0 || soil.saturatedHydraulicConductivity.at(soil.saturatedHydraulicConductivity.size() - 1) < 0)
               {
                  utils.handleError("Error (soil input): pwp, fc, porosity or saturated hydraulic conductivity are out of range. Please check the soil file.");
               }

               if (soil.fieldCapacity.at(soil.fieldCapacity.size() - 1) < soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) ||
                   soil.porosity.at(soil.porosity.size() - 1) < soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) ||
                   soil.porosity.at(soil.porosity.size() - 1) < soil.fieldCapacity.at(soil.fieldCapacity.size() - 1))
               {
                  utils.handleError("Error (soil input): pwp, fc and porosity are not in reasonable order of values (pwp < fc < porosity). Please check the soil file.");
               }
            }
            else
            {
               utils.handleError("Values are missing in the soil input file in line " + std::to_string(m) + ". Please check the entries to be exactly five values separated by tabulator.");
            }
         }
      }
      file.close();

      double contentSum = soil.sandContent + soil.siltContent + soil.clayContent;
      if ((contentSum < (1.0 - tolerance)) || (contentSum > (1.0 + tolerance)))
      {
         utils.handleError("Error (soil input): sand, silt and clay content do not sum up to one. Please check the soil file.");
      }

      if (soil.sandContent < 0 || soil.siltContent < 0 || soil.clayContent < 0 || soil.sandContent > 1 || soil.siltContent > 1 || soil.clayContent > 1)
      {
         utils.handleError("Error (soil input): sand, silt or clay content are out of range. Please check the soil file.");
      }

      int soilLayers = 20;
      if (soil.permanentWiltingPoint.size() != soilLayers || soil.fieldCapacity.size() != soilLayers || soil.porosity.size() != soilLayers || soil.saturatedHydraulicConductivity.size() != soilLayers)
      {
         utils.handleError("Error (soil input): there are not enough or too many values for 20 soil layers. Please check the soil file.");
      }
   }
   else
   {
      utils.handleError("Error (soil input): The soil file cannot be opened. Please check the name in the configuration file.");
   }
}
