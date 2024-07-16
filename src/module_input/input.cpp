#include "input.h"

INPUT::INPUT(){};
INPUT::~INPUT(){};

std::map<std::string, int> INPUT::configParInt;
std::map<std::string, float> INPUT::configParFloat;
std::map<std::string, bool> INPUT::configParBool;
std::map<std::string, std::string> INPUT::configParString;

/* stream all relevant input data */
void INPUT::getInputData(std::string path, UTILS ut, PARAMETER &param, WEATHER &weather, SOIL &soil, MANAGEMENT &manage)
{
    openAndReadConfigurationFile(path, ut, param);
    openAndReadSpeciesFile(path, ut, param);
    openAndReadWeatherFile(path, ut, param, weather);
    openAndReadSoilFile(path, ut, param, soil);
    openAndReadManagementFile(path, ut, param, manage);
}

/* open and read configuration file */
void INPUT::openAndReadConfigurationFile(std::string config, UTILS ut, PARAMETER &param)
{
    const char *filename = config.c_str();
    for (auto par : param.configParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
    {
        /* open file and search for name in all lines */
        searchParameterInInputFile(par, filename, ut);

        /* check if the parameter name was found at least once and read in each line */
        checkIfParameterExistsAndExtractValues(ut, par, lineValues, lineNumbers, lineTypeValues);

        /* get the corresponding datatype for the extracted parameter value */
        extractDataTypeForExtractedValue(ut, par);

        /* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
        convertAndCheckAndSetParameterValue(ut, par, parameterType, param);
    }

    /* transfer the mapped values of all parameter names to their variables in class PARAMETER */
    transferConfigParameterValueToModelParameter(param, ut);
}

/* open file and search for name (keyword) in all lines */
void INPUT::searchParameterInInputFile(std::string keyword, const char *filename, UTILS ut)
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
        ut.handleError("Cannot open the file" + s + ". There is no simuation possible. Please check if the file exists.");
    }
}

/* check if the parameter name (keyowrd) was found at least once */
void INPUT::checkIfParameterExistsAndExtractValues(UTILS ut, std::string keyword, std::vector<std::string> lineValues, std::vector<int> lineNumbers, std::vector<std::string> lineTypeValues)
{
    /* if parameter name (keyword) was not found in the input file */
    if (lineNumbers.size() == 0)
    {
        ut.handleError("The parameter " + keyword + " is missing in the input file. Please check the file!");
    }

    /* extract for each line with the found keyword only those that have the correct format */
    if (lineNumbers.size() >= 1)
    {
        extractLinesOfCorrectFormat(ut, keyword, lineValues);
    }
}

/* read in each line, split string and save correct formatted parameter values in variables */
void INPUT::extractLinesOfCorrectFormat(UTILS ut, std::string keyword, std::vector<std::string> lineValues)
{
    int index = 0;                          /* index used to track which of the found lines show the correct format of the found parameter (e.g. if found more than once) */
    std::vector<std::string> valueElements; /* first splitted line string by tabstopp */
    std::vector<std::string> cleanLine;     /* remove additional free spaces if they occur between the name and value of the parameter */

    keywordLineNumbers.clear(); // all lines where the parameter was found in the correct format
    keywordLineValues.clear();

    for (auto line : lineValues)
    {
        cleanLine.clear();
        ut.strings.clear();
        ut.splitString(line, '\t'); /* split string based on tabstop */
        valueElements.clear();
        for (auto it : ut.strings)
        {
            valueElements.push_back(it); /* save splitted string elements because ut.strings will be cleared at next step */
        }

        for (auto it : valueElements)
        {
            ut.strings.clear();
            ut.splitString(it, ' '); /* split string based on free space */
            for (auto word : ut.strings)
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
                ut.handleError("A value is missing for the parameter " + keyword + ". Please check the input file!");
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
                ut.handleError("The parameter " + keyword + " is mentioned several times. Please check the input file!");
            }
            else /* case: there is only one line in the input file where the parameter is mentioned and its the false format */
            {
                ut.handleError("The parameter " + keyword + " is either missing or in a wrong format. Please check the input file!");
            }
        }
        index++;
    }

    if (keywordLineNumbers.size() == 0) /* none of the identified lines shows a correct format of the parameter */
    {
        ut.handleError("The parameter " + keyword + " is either missing or in a wrong format. Please check the input file!");
    }
    else if (keywordLineNumbers.size() > 0) /* the parameter occurs at least once in the correct format */
    {

        if (keywordLineNumbers.size() > 1) /* the parameter occurs more than once in the correct format */
        {
            ut.handleError("The parameter " + keyword + " occurs more than once in the input file. Please check the input file!");
        }
    }
}

/* get the corresponding datatype for the extracted parameter value */
void INPUT::extractDataTypeForExtractedValue(UTILS ut, std::string keyword)
{
    if (keywordLineNumbers.size() > 0)
    {
        ut.strings.clear();
        ut.splitString(lineTypeValues.at(keywordLineNumbers.at(0)), ':'); // now strings should have 2 elements: "\datatype" type

        if (ut.strings.size() > 0)
        {
            if (ut.strings.at(0) != "\\datatype") // at least on index 2 of vector strings can be found
            {
                ut.handleError("The line following the parameter value for " + keyword + " does not include the datatype. Please check the input file!");
            }
            else
            {
                if (ut.strings.size() == 2)
                {
                    parameterType = ut.strings.at(1);
                }
                else
                {
                    ut.handleError("The datatype is missing for the parameter " + keyword + ". Please check the input file!");
                }
            }
        }
        else
        {
            ut.handleError("The datatype is missing for the parameter " + keyword + ". Please check the input file!");
        }
    }
}

/* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
void INPUT::convertAndCheckAndSetParameterValue(UTILS ut, std::string keyword, std::string parameterType, PARAMETER param)
{
    if (parameterType == "integer")
    {
        try
        {
            int value = std::stoi(keywordLineValues.at(0));
            if (value < 0)
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
            ut.handleError(e.what());
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
            ut.handleError(e.what());
            configParFloat[keyword] = -1;
        }
    }
    else if (parameterType == "date")
    {
        try
        {
            ut.strings.clear();
            ut.splitString(keywordLineValues.at(0), '-');
            int day = std::stoi(ut.strings.at(2));
            int month = std::stoi(ut.strings.at(1));
            int year = std::stoi(ut.strings.at(0));
            // calculate julian day
            int julianDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;

            if (julianDay < 0)
            {
                throw std::out_of_range("Value of parameter " + keyword + " is outside the valid range! Please check the date!");
            }
            else
            {
                configParInt[keyword] = julianDay;
            }
        }
        catch (const std::out_of_range &e)
        {
            ut.handleError(e.what());
            configParInt[keyword] = -1;
        }
    }
    else if (parameterType == "boolean")
    {
        try
        {
            bool value = ut.stringToBool(keywordLineValues.at(0));
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
            ut.handleError(e.what());
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

            if (keyword != "deimsID")
            {
                std::string fileEnding = "";
                fileEnding = ut.getFileEnding(keywordLineValues.at(0));
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
        catch (const std::out_of_range &e)
        {
            ut.handleError(e.what());
            configParString[keyword] = "";
        }
        catch (const std::invalid_argument &e)
        {
            ut.handleError(e.what());
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
            ut.handleError(e.what());
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
            ut.handleError(e.what());
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
            ut.handleError(e.what());
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
            ut.handleError(e.what());
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

                ut.strings.clear();
                ut.splitString(keywordLineValues.at(i), '-');
                int day = std::stoi(ut.strings.at(2));
                int month = std::stoi(ut.strings.at(1));
                int year = std::stoi(ut.strings.at(0));
                // calculate julian day
                int julianDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;

                if (julianDay < 0)
                {
                    throw std::out_of_range("Value of parameter " + keyword + array_pos.c_str() + " is outside the valid range! Please check the date!");
                }
                else
                {
                    configParInt[keyword + array_pos.c_str()] = julianDay;
                }
            }
        }
        catch (const std::out_of_range &e)
        {
            ut.handleError(e.what());
            configParInt[keyword + array_pos.c_str()] = -1;
        }
    }
    else
    {
        ut.handleError("No valid datatype for the parameter " + keyword + ". Please check the input file!");
    }
}

/* transfer the mapped values of all config parameter names to their variables in class PARAMETER */
void INPUT::transferConfigParameterValueToModelParameter(PARAMETER &param, UTILS ut)
{
    param.deimsID = configParString["deimsID"];
    param.latitude = configParFloat["latitude"];
    param.longitude = configParFloat["longitude"];
    param.weatherFile = configParString["weatherFile"];
    param.soilFile = configParString["soilFile"];
    param.managementFile = configParString["managementFile"];
    param.speciesFile = configParString["speciesFile"];
    param.outputFile = configParBool["outputFile"];
    param.outputWritingDatesFile = configParString["outputWritingDatesFile"];
    param.clippingHeightForBiomassCalibration = configParFloat["clippingHeightForBiomassCalibration"];
    param.reproducibleResults = configParBool["reproducibleResults"];
    param.randomNumberGeneratorSeed = configParInt["randomNumberGeneratorSeed"];

    param.firstYear = configParInt["firstYear"];
    param.lastYear = configParInt["lastYear"];
    // calculate reference julian days (1 Jan of param.firstYear and 31 Dec of param.lastYear)
    param.referenceJulianDayStart = ut.calculateJulianDayFromDate(1, 1, param.firstYear);
    param.referenceJulianDayEnd = ut.calculateJulianDayFromDate(31, 12, param.lastYear);
}

/* transfer the mapped values of all species parameter names to their variables in class PARAMETER */
void INPUT::transferSpeciesParameterValueToModelParameter(PARAMETER &param)
{
    param.numberOfSpecies = configParInt["numberOfSpecies"];

    /* parameter independent of species type */
    param.crowdingMortalityActivated = configParBool["crowdingMortalityActivated"];
    param.externalSeedInfluxActivated = configParBool["externalSeedInfluxActivated"];
    param.dayOfExternalSeedInfluxStart = configParInt["dayOfExternalSeedInfluxStart"];
    param.plantSeedProductionActivated = configParBool["plantSeedProductionActivated"];
    param.brownBiomassFractionFalling = configParFloat["brownBiomassFractionFalling"];
    param.plantResponseToTemperatureQ10Base = configParFloat["plantResponseToTemperatureQ10Base"];
    param.plantResponseToTemperatureQ10Reference = configParFloat["plantResponseToTemperatureQ10Reference"];
    param.plantCostRhizobiaSymbiosis = configParFloat["plantCostRhizobiaSymbiosis"];
    param.growthRespirationFraction = configParFloat["growthRespirationFraction"];
    param.maintenanceRespirationRate = configParFloat["maintenanceRespirationRate"];

    /* parameter dependent of species type */
    for (int pft = 0; pft < param.numberOfSpecies; pft++)
    {
        std::string array_pos = std::to_string(pft);
        param.maximumPlantHeight.push_back(configParInt["maximumPlantHeight" + array_pos]);
        param.plantHeightToWidthRatio.push_back(configParFloat["plantHeightToWidthRatio" + array_pos]);
        param.plantShootCorrectionFactor.push_back(configParFloat["plantShootCorrectionFactor" + array_pos]);
        param.plantShootRootRatio.push_back(configParFloat["plantShootRootRatio" + array_pos]);
        param.plantRootDepthParamIntercept.push_back(configParFloat["plantRootDepthParamIntercept" + array_pos]);
        param.plantRootDepthParamExponent.push_back(configParFloat["plantRootDepthParamExponent" + array_pos]);
        param.plantSpecificLeafArea.push_back(configParFloat["plantSpecificLeafArea" + array_pos]);
        param.plantShootOverlapFactors.push_back(configParFloat["plantShootOverlapFactors" + array_pos]);
        param.rootLifeSpan.push_back(configParInt["rootLifeSpan" + array_pos]);
        param.leafLifeSpan.push_back(configParInt["leafLifeSpan" + array_pos]);
        param.plantLifeSpan.push_back(configParString["plantLifeSpan" + array_pos]);
        param.plantMortalityRates.push_back(configParFloat["plantMortalityRates" + array_pos]);
        param.seedlingMortalityRates.push_back(configParFloat["seedlingMortalityRates" + array_pos]);
        param.seedGerminationTimes.push_back(configParInt["seedGerminationTimes" + array_pos]);
        param.seedGerminationRates.push_back(configParFloat["seedGerminationRates" + array_pos]);
        param.seedMasses.push_back(configParFloat["seedMasses" + array_pos]);
        param.maturityAges.push_back(configParFloat["maturityAges" + array_pos]);
        param.externalSeedInfluxNumber.push_back(configParInt["externalSeedInfluxNumber" + array_pos]);
        param.maximumGrossLeafPhotosynthesisRate.push_back(configParFloat["maximumGrossLeafPhotosynthesisRate" + array_pos]);
        param.initialSlopeOfLightResponseCurve.push_back(configParFloat["initialSlopeOfLightResponseCurve" + array_pos]);
        param.lightExtinctionCoefficients.push_back(configParFloat["lightExtinctionCoefficients" + array_pos]);
        param.plantNPPAllocationToPlantGrowth.push_back(configParFloat["plantNPPAllocationToPlantGrowth" + array_pos]);
        param.plantCNRatioGreen.push_back(configParFloat["plantCNRatioGreen" + array_pos]);
        param.plantCNRatioBrown.push_back(configParFloat["plantCNRatioBrown" + array_pos]);
        param.nitrogenFixationAbility.push_back(configParBool["nitrogenFixationAbility" + array_pos]);
        param.plantWaterUseEfficiency.push_back(configParFloat["plantWaterUseEfficiency" + array_pos]);
        param.plantMinimalSoilWaterForGPPReduction.push_back(configParFloat["plantMinimalSoilWaterForGPPReduction" + array_pos]);
        param.plantMaximalSoilWaterForGPPReduction.push_back(configParFloat["plantMaximalSoilWaterForGPPReduction" + array_pos]);
    }
}

/* open and read species parameter file */
void INPUT::openAndReadSpeciesFile(std::string path, UTILS ut, PARAMETER &param)
{
    char separator = '\\';
    ut.strings.clear();
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 3; it++)
    {
        speciesDirectory = speciesDirectory + ut.strings.at(it) + "\\";
    }
    speciesDirectory = speciesDirectory + "parameters\\" + param.speciesFile;
    const char *filename = speciesDirectory.c_str();

    for (auto par : param.speciesParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
    {
        /* open file and search for name in all lines */
        searchParameterInInputFile(par, filename, ut);

        /* check if the parameter name was found at least once and read in each line */
        checkIfParameterExistsAndExtractValues(ut, par, lineValues, lineNumbers, lineTypeValues);

        /* get the corresponding datatype for the extracted parameter value */
        extractDataTypeForExtractedValue(ut, par);

        /* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
        convertAndCheckAndSetParameterValue(ut, par, parameterType, param);
    }

    /* transfer the mapped values of all parameter names to their variables in class PARAMETER */
    transferSpeciesParameterValueToModelParameter(param);
}

/* read-in weather variables from input file */
void INPUT::openAndReadWeatherFile(std::string path, UTILS ut, PARAMETER &param, WEATHER &weather)
{
    char separator = '\\';
    ut.strings.clear();
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 3; it++)
    {
        weatherDirectory = weatherDirectory + ut.strings.at(it) + "\\";
    }
    weatherDirectory = weatherDirectory + "scenarios\\weather\\" + param.weatherFile;
    const char *filename = weatherDirectory.c_str();

    // @Thomas: From template (eLTER data call)
    // Date	Precipitation[mmd-1]	Temperature[degC]	PPFD[mmolm-2s-1]	PET[mmd-1]
    weather.weatherDates.clear();
    weather.precipitation.clear();
    weather.airTemperature.clear();
    weather.photosyntheticPhotonFluxDensity.clear();
    weather.potEvapoTranspiration.clear();
    weather.dayLength.clear();

    std::string line;  // current line text in parser
    int m = 0;         // current line number in parser
    const char *value; // placeholder for extracted value from file

    std::ifstream file(filename);
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            m++;
            if (m > 1)
            { // skip header line
                char separator = '\t';
                ut.strings.clear();
                ut.splitString(line, separator);

                if (ut.strings.size() == 5)
                {
                    weather.weatherDates.push_back(ut.strings.at(0));

                    value = ut.strings.at(1).c_str();
                    weather.precipitation.push_back(atof(value));

                    value = ut.strings.at(2).c_str();
                    weather.airTemperature.push_back(atof(value));

                    value = ut.strings.at(3).c_str();
                    weather.photosyntheticPhotonFluxDensity.push_back(atof(value));

                    value = ut.strings.at(4).c_str();
                    weather.potEvapoTranspiration.push_back(atof(value));

                    // TODO: add calculation of day length
                    weather.dayLength.push_back(weather.calculateAstronomicDayLength());

                    // TODO: add calculation of PET if not given as value in the weather file
                    if (weather.potEvapoTranspiration.at(m - 2) == -9999)
                    {
                        weather.dayLength.push_back(weather.calculatePotentialEvapoTranspiration());
                    }
                }
                else
                {
                    ut.handleError("Values are missing in the weather input file in line " + std::to_string(m) + ". Please check the entry to be five values separated by tabulator.");
                }
            }
        }
        file.close();

        // check first and last date if it matches the parameters of the configuration file
        std::string startDate = std::to_string(param.firstYear) + "-01-01";
        std::string endDate = std::to_string(param.lastYear) + "-12-31";

        if (weather.weatherDates.at(0) != startDate || weather.weatherDates.at(weather.weatherDates.size() - 1) != endDate)
        {
            ut.handleError("Error (weather input): the first and last dates in the weather file do not match the simulation period as specified in the configuration file.");
        }

        // check if there are no missing days inbetween
        if ((m - 1) < param.numberOfDaysToSimulate)
        {
            ut.handleError("Error (weather input): there are not enough data given in the weather file for the simulation period as specified in the configuration file.");
        }
    }
}

/* read-in management information from input file */
void INPUT::openAndReadManagementFile(std::string path, UTILS ut, PARAMETER &param, MANAGEMENT &manage)
{
    char separator = '\\';
    ut.strings.clear();
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 3; it++)
    {
        manageDirectory = manageDirectory + ut.strings.at(it) + "\\";
    }
    manageDirectory = manageDirectory + "scenarios\\management\\" + param.managementFile;
    const char *filename = manageDirectory.c_str();

    manage.mowingDate.clear();
    manage.mowingHeight.clear();

    manage.fertilizationDate.clear();
    manage.fertilizerAmount.clear();

    manage.irrigationDate.clear();
    manage.irrigationAmount.clear();

    manage.sowingDate.clear();
    manage.amountOfSownSeeds.clear(); // 2D vector
    for (int pft = 0; pft < param.numberOfSpecies; pft++)
    {
        manage.amountOfSownSeeds.push_back(std::vector<int>()); // add rows according to the number of pfts from configuration file
    }

    std::string line;                      // current line text in parser
    int m = 0;                             // current line number in parser
    std::string valueDate;                 // placeholder for extracted value from file (date of a specific managent action)
    double valueActionMowing;              // placeholder for extracted value from file (management action)
    double valueActionFertilization;       // placeholder for extracted value from file (management action)
    double valueActionIrrigation;          // placeholder for extracted value from file (management action)
    double valueActionSowingActivated;     // placeholder for extracted value from file (management action)
    std::vector<double> valueActionSowing; // placeholder for extracted value from file (management action)

    std::ifstream file(filename);
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            m++;
            if (m > 1)
            { // skip header line
                char separator = '\t';
                ut.strings.clear();
                ut.splitString(line, separator);

                if (ut.strings.size() == (4 + param.numberOfSpecies))
                {

                    valueDate = ut.strings.at(0);
                    valueActionMowing = atof(ut.strings.at(1).c_str());
                    valueActionFertilization = atof(ut.strings.at(2).c_str());
                    valueActionIrrigation = atof(ut.strings.at(3).c_str());
                    valueActionSowingActivated = 0;
                    valueActionSowing.clear();
                    for (int pft = 0; pft < param.numberOfSpecies; pft++)
                    {
                        valueActionSowingActivated += atof(ut.strings.at(4 + pft).c_str());
                        valueActionSowing.push_back(atof(ut.strings.at(4 + pft).c_str()));
                    }

                    // mowing events
                    if (valueActionMowing > 0)
                    {
                        ut.strings.clear();
                        ut.splitString(valueDate, '-');
                        if (ut.strings.size() == 3)
                        {
                            int day = std::stoi(ut.strings.at(2).c_str());
                            int month = std::stoi(ut.strings.at(1).c_str());
                            int year = std::stoi(ut.strings.at(0).c_str());

                            int mowDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;
                            if (mowDay > 0 && mowDay < (param.referenceJulianDayEnd - param.referenceJulianDayStart + 1))
                            {
                                manage.mowingDate.push_back(mowDay);
                                manage.mowingHeight.push_back(valueActionMowing);
                            }
                            else
                            {
                                ut.handleError("Mowing date is not valid within the simulation period.");
                            }
                        }
                        else
                        {
                            ut.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                        }
                    }

                    // fertilization events
                    if (valueActionFertilization > 0)
                    {
                        ut.strings.clear();
                        ut.splitString(valueDate, '-');
                        if (ut.strings.size() == 3)
                        {
                            int day = std::stoi(ut.strings.at(2).c_str());
                            int month = std::stoi(ut.strings.at(1).c_str());
                            int year = std::stoi(ut.strings.at(0).c_str());

                            int fertDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;
                            if (fertDay > 0 && fertDay < (param.referenceJulianDayEnd - param.referenceJulianDayStart + 1))
                            {
                                manage.fertilizationDate.push_back(fertDay);
                                manage.fertilizerAmount.push_back(valueActionFertilization);
                            }
                            else
                            {
                                ut.handleError("Fertilization date is not valid within the simulation period.");
                            }
                        }
                        else
                        {
                            ut.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                        }
                    }

                    // irrigation events
                    if (valueActionIrrigation > 0)
                    {
                        ut.strings.clear();
                        ut.splitString(valueDate, '-');
                        if (ut.strings.size() == 3)
                        {
                            int day = std::stoi(ut.strings.at(2).c_str());
                            int month = std::stoi(ut.strings.at(1).c_str());
                            int year = std::stoi(ut.strings.at(0).c_str());

                            int irrigDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;
                            if (irrigDay > 0 && irrigDay < (param.referenceJulianDayEnd - param.referenceJulianDayStart + 1))
                            {
                                manage.irrigationDate.push_back(irrigDay);
                                manage.irrigationAmount.push_back(valueActionIrrigation);
                            }
                            else
                            {
                                ut.handleError("Irrigation date is not valid within the simulation period.");
                            }
                        }
                        else
                        {
                            ut.handleError("Error (management input): the date seems not to have a correct format. Please check the file.");
                        }
                    }

                    // seed sowing events
                    if (valueActionSowingActivated > 0) // only if at least one PFT is sown (sum > 0)
                    {
                        ut.strings.clear();
                        ut.splitString(valueDate, '-');
                        int day = std::stoi(ut.strings.at(2).c_str());
                        int month = std::stoi(ut.strings.at(1).c_str());
                        int year = std::stoi(ut.strings.at(0).c_str());

                        int sowDay = ut.calculateJulianDayFromDate(day, month, year) - param.referenceJulianDayStart + 1;
                        if (sowDay > 0 && sowDay < (param.referenceJulianDayEnd - param.referenceJulianDayStart + 1))
                        {
                            manage.sowingDate.push_back(sowDay);
                            for (int pft = 0; pft < param.numberOfSpecies; pft++)
                            {
                                manage.amountOfSownSeeds[pft].push_back((int)valueActionSowing[pft]);
                            }
                        }
                        else
                        {
                            ut.handleError("Sowing date is not valid within the simulation period.");
                        }
                    }
                }
                else
                {
                    ut.handleError("Values are missing in the management input file in line " + std::to_string(m) + ". Please check the entries.");
                }
            }
        }
        file.close();
    }
}

/* Reads-in soil parameters from input file */
void INPUT::openAndReadSoilFile(std::string path, UTILS ut, PARAMETER &param, SOIL &soil)
{
    char separator = '\\';
    ut.strings.clear();
    ut.splitString(path, separator);
    for (int it = 0; it < ut.strings.size() - 3; it++)
    {
        soilDirectory = soilDirectory + ut.strings.at(it) + "\\";
    }
    soilDirectory = soilDirectory + "scenarios\\soil\\" + param.soilFile;
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
                ut.strings.clear();
                ut.splitString(line, separator);

                if (ut.strings.size() == 3)
                {
                    value = ut.strings.at(0).c_str();
                    soil.siltContent = atof(value);

                    value = ut.strings.at(1).c_str();
                    soil.clayContent = atof(value);

                    value = ut.strings.at(2).c_str();
                    soil.sandContent = atof(value);
                }
                else
                {
                    ut.handleError("Values are missing in the soil input file in line " + std::to_string(m) + ". Please check the entries to be exactly three values separated by tabulator.");
                }
            }

            if (m >= 5)
            { // skip header lines above
                char separator = '\t';
                ut.strings.clear();
                ut.splitString(line, separator);

                if (ut.strings.size() == 5)
                {
                    // skip layer number in column 1
                    value = ut.strings.at(1).c_str();
                    soil.fieldCapacity.push_back(atof(value));

                    value = ut.strings.at(2).c_str();
                    soil.permanentWiltingPoint.push_back(atof(value));

                    value = ut.strings.at(3).c_str();
                    soil.porosity.push_back(atof(value));

                    value = ut.strings.at(4).c_str();
                    soil.saturatedHydraulicConductivity.push_back(atof(value));

                    if (soil.fieldCapacity.at(soil.fieldCapacity.size() - 1) < 0 || soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) < 0 ||
                        soil.porosity.at(soil.porosity.size() - 1) < 0 || soil.saturatedHydraulicConductivity.at(soil.saturatedHydraulicConductivity.size() - 1) < 0)
                    {
                        ut.handleError("Error (soil input): pwp, fc, porosity or saturated hydraulic conductivity are out of range. Please check the soil file.");
                    }

                    if (soil.fieldCapacity.at(soil.fieldCapacity.size() - 1) < soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) ||
                        soil.porosity.at(soil.porosity.size() - 1) < soil.permanentWiltingPoint.at(soil.permanentWiltingPoint.size() - 1) ||
                        soil.porosity.at(soil.porosity.size() - 1) < soil.fieldCapacity.at(soil.fieldCapacity.size() - 1))
                    {
                        ut.handleError("Error (soil input): pwp, fc and porosity are not in reasonable order of values (pwp < fc < porosity). Please check the soil file.");
                    }

                    // TODO: if values are missing, pedotransfer functions could be directly included here (declared and defined in module_soil) and written to the soil file. including note in stderr.txt
                }
                else
                {
                    ut.handleError("Values are missing in the soil input file in line " + std::to_string(m) + ". Please check the entries to be exactly five values separated by tabulator.");
                }
            }
        }
        file.close();

        double contentSum = soil.sandContent + soil.siltContent + soil.clayContent;
        if ((contentSum < 1.0) || (contentSum > 1.0))
        {
            ut.handleError("Error (soil input): sand, silt and clay content do not sum up to one. Please check the soil file.");
        }

        if (soil.sandContent < 0 || soil.siltContent < 0 || soil.clayContent < 0 || soil.sandContent > 1 || soil.siltContent > 1 || soil.clayContent > 1)
        {
            ut.handleError("Error (soil input): sand, silt or clay content are out of range. Please check the soil file.");
        }

        int soilLayers = 20;
        if (soil.permanentWiltingPoint.size() != soilLayers || soil.fieldCapacity.size() != soilLayers || soil.porosity.size() != soilLayers || soil.saturatedHydraulicConductivity.size() != soilLayers)
        {
            ut.handleError("Error (soil input): there are not enough or too many values for 20 soil layers. Please check the soil file.");
        }

        // TODO (when soil dynamics added): check if added for groundwater storage layer???
        // par.nitrogenSoilPerLayer.push_back(atof(linevalue) * 1E-06);
    }
}
