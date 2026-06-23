#include "input.h"

INPUT::INPUT() {};
INPUT::~INPUT() {};

std::map<std::string, int> INPUT::configParInt;
std::map<std::string, float> INPUT::configParFloat;
std::map<std::string, bool> INPUT::configParBool;
std::map<std::string, std::string> INPUT::configParString;

/**
 * @brief Reads and parses all input data required to run the simulation.
 *
 * Opens and processes the following input files in order:
 * 1. Configuration file — general simulation settings.
 * 2. Plant traits file — species/PFT-specific physiological parameters.
 * 3. Weather file — daily meteorological time series.
 * 4. Soil file — texture and hydraulic properties per soil layer.
 * 5. Management file — mowing, fertilisation, irrigation, and sowing events.
 * 6. Process setup file — flags controlling which sub-models are active.
 *
 * @param path       Absolute path to the main configuration file; used to derive
 *                   the directory structure for all other input files.
 * @param utils      Utility object for string splitting and error handling.
 * @param parameter  Model parameter struct; filled with values read from all files.
 * @param weather    Weather struct; filled with the daily meteorological time series.
 * @param soil       Soil struct; filled with texture and per-layer hydraulic properties.
 * @param management Management struct; filled with all scheduled management events.
 */
void INPUT::getInputData(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather, SOIL &soil, MANAGEMENT &management)
{
    openAndReadConfigurationFile(path, utils, parameter);

    openAndReadPlantTraitsFile(path, utils, parameter);

    openAndReadWeatherFile(path, utils, parameter, weather);

    openAndReadSoilFile(path, utils, parameter, soil);

    openAndReadManagementFile(path, utils, parameter, management);

    openAndReadProcessSetupFile(path, utils, parameter);
}

/**
 * @brief Opens and parses the main configuration file, populating the PARAMETER struct.
 *
 * Iterates over all expected configuration parameter names (defined in PARAMETER),
 * searches for each in the file via searchParameterInInputFile(), validates its
 * presence and format, determines its data type, converts the raw string value to
 * the correct type, and finally transfers all mapped values to the PARAMETER struct.
 *
 * @param config    Absolute path to the configuration file.
 * @param utils     Utility object for string splitting and error handling.
 * @param parameter Model parameter struct; configuration values are written here.
 */
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

/**
 * @brief Searches a parameter file for all lines containing a given keyword.
 *
 * Opens the file line-by-line and records every line (and its number) that
 * contains `keyword`. The line immediately following each match is stored as
 * the candidate data-type descriptor. Windows-style carriage-return artifacts
 * are stripped before matching.
 *
 * Results are stored in the member variables `lineValues`, `lineNumbers`, and
 * `lineTypeValues` for use by subsequent parsing steps.
 *
 * @param keyword  The parameter name to search for.
 * @param filename Null-terminated path to the input file.
 * @param utils    Utility object for error handling.
 * @return `true` if the file was opened successfully, `false` otherwise
 *         (after calling `utils.handleError()`).
 */
bool INPUT::searchParameterInInputFile(std::string keyword, const char *filename, UTILS utils)
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
            if (!line.empty() && line.back() == '\r') /* if windows artifact, remove it*/
            {
                line.pop_back();
            }
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
        return true;
    }
    else
    {
        std::string s = filename;
        utils.handleError("Cannot open the file" + s + ". There is no simuation possible. Please check if the file exists.");
        return false;
    }
}

/**
 * @brief Verifies that a keyword was found in the input file and extracts its value lines.
 *
 * Raises an error if the keyword was not found at all. If it was found one or more
 * times, delegates to extractLinesOfCorrectFormat() to keep only lines that match
 * the expected `keyword <value>` tab-separated format.
 *
 * @param utils          Utility object for error handling.
 * @param keyword        The parameter name that was searched for.
 * @param lineValues     Text of all lines in which `keyword` appeared.
 * @param lineNumbers    Line numbers corresponding to `lineValues`.
 * @param lineTypeValues Lines immediately following each matched line
 *                       (expected to contain the `\datatype` descriptor).
 */
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

/**
 * @brief Filters candidate lines to those with the correct `keyword <value>` format
 *        and extracts the associated value token(s).
 *
 * For each candidate line, splits first by tab and then by spaces to produce a
 * clean token list. A line is considered correctly formatted if its first token
 * matches `keyword`. The extracted value token(s) are appended to
 * `keywordLineValues` and the corresponding index to `keywordLineNumbers`.
 *
 * Raises an error if:
 * - No correctly formatted occurrence of the keyword is found,
 * - A matched line contains no value token,
 * - The keyword occurs in a correct format more than once.
 *
 * @param utils      Utility object for string splitting, warning, and error handling.
 * @param keyword    The parameter name expected as the first token of a valid line.
 * @param lineValues Candidate lines returned by searchParameterInInputFile().
 */
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

/**
 * @brief Reads the data-type descriptor from the line following the keyword match.
 *
 * Expects a line of the form `\datatype:<type>` immediately after the parameter
 * value line. Splits on `:` and stores the type string in `parameterType`.
 * Raises an error if the descriptor line is absent, malformed, or the type token
 * is missing.
 *
 * @param utils   Utility object for string splitting and error handling.
 * @param keyword Parameter name; used only for informative error messages.
 */
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

/**
 * @brief Converts a raw string value to its declared data type and stores it
 *        in the appropriate static map.
 *
 * Supported types and their target maps:
 * - `integer`, `integer-array`  → `configParInt`
 * - `float`, `float-array`      → `configParFloat`
 * - `boolean`, `boolean-array`  → `configParBool`
 * - `string`, `string-array`    → `configParString`
 * - `date`, `date-array`        → `configParInt` (stored as day-count offset
 *                                 from `parameter.referenceJulianDayStart`)
 *
 * For scalar types, non-negative range checks are applied (with named exceptions
 * such as `randomNumberGeneratorSeed`, `h2H`, `h2L`). For string parameters,
 * a `.txt` extension is appended if missing (except for IDs and coordinate strings).
 * Errors are reported via `utils.handleError()` for out-of-range or missing values.
 *
 * @param utils         Utility object for parsing helpers and error handling.
 * @param keyword       Parameter name; used as the map key and in error messages.
 * @param parameterType Data-type string (e.g. `"float"`, `"integer-array"`).
 * @param parameter     Read-only; provides `referenceJulianDayStart` for date conversion.
 */
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
            if (value < 0 && keyword != "h2H" && keyword != "h2L")
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

            if (keyword != "deimsID" && keyword != "latitude" && keyword != "longitude" && keyword != "plantGppReductionBySoilWaterApproach")
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

/**
 * @brief Copies all parsed configuration values from the static maps into the
 *        PARAMETER struct.
 *
 * Transfers file names, output flags, simulation period (first/last year),
 * clipping height, and the RNG seed. Also computes the Julian-day reference
 * boundaries (`referenceJulianDayStart`, `referenceJulianDayEnd`) and the
 * total simulation length in days.
 *
 * @param parameter Model parameter struct; all configuration fields are written here.
 * @param utils     Utility object used for Julian-day calculations.
 */
void INPUT::transferConfigParameterValueToModelParameter(PARAMETER &parameter, UTILS utils)
{
    parameter.deimsID = configParString["deimsID"];
    parameter.latitude = configParString["latitude"];
    parameter.longitude = configParString["longitude"];
    parameter.weatherFile = configParString["weatherFile"];
    parameter.soilFile = configParString["soilFile"];
    parameter.managementFile = configParString["managementFile"];
    parameter.plantTraitsFile = configParString["plantTraitsFile"];
    parameter.processSetupFile = configParString["processSetupFile"];
    parameter.soilParametersFile = configParString["soilParametersFile"];

    parameter.communityOutputFile = configParBool["communityOutputFile"];
    parameter.pftOutputFile = configParBool["pftOutputFile"];
    parameter.plantCohortOutputFile = configParBool["plantCohortOutputFile"];
    parameter.soilCarbonOutputFile = configParBool["soilCarbonOutputFile"];
    parameter.soilNitrogenOutputFile = configParBool["soilNitrogenOutputFile"];
    parameter.soilWaterOutputFile = configParBool["soilWaterOutputFile"];
    parameter.soilResourcesPerSoilLayerOutputFile = configParBool["soilResourcesPerSoilLayerOutputFile"];
    parameter.soilFluxesDetailsOutputFile = configParBool["soilFluxesDetailsOutputFile"];

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

/**
 * @brief Copies all parsed plant-trait values from the static maps into the
 *        PARAMETER struct.
 *
 * Transfers PFT count, global flags (crowding mortality, seed influx, allocation
 * mode, etc.), community-level physiological constants, and all per-PFT vectors
 * (allometric parameters, photosynthesis parameters, allocation fractions, C:N
 * ratios, water use, etc.).
 *
 * @param parameter Model parameter struct; all plant-trait fields are written here.
 */
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
    parameter.rhizobiaExchangeRateCToN = configParFloat["rhizobiaExchangeRateCToN"];
    parameter.growthRespirationFraction = configParFloat["growthRespirationFraction"];
    parameter.maintenanceRespirationRate = configParFloat["maintenanceRespirationRate"];
    parameter.communityShadingInGppCalculation = configParBool["communityShadingInGppCalculation"];
    parameter.plantGppReductionBySoilWaterApproach = configParString["plantGppReductionBySoilWaterApproach"];
    parameter.tresholdCohortDeathDeterministic = configParFloat["tresholdCohortDeathDeterministic"];

    /* parameter relevant for coupling */
    parameter.h2L = configParFloat["h2L"];
    parameter.h2H = configParFloat["h2H"];
    parameter.crowdingCalculationFromPlantTopLayer = configParInt["crowdingCalculationFromPlantTopLayer"];
    parameter.minLayerReductionFactorFromAverage = configParInt["minLayerReductionFactorFromAverage"];
    parameter.disableRunoff = configParBool["disableRunoff"];

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
        parameter.symbioticNitrogenFixationFraction.push_back(configParFloat["symbioticNitrogenFixationFraction" + array_pos]);
        parameter.plantWaterUseEfficiency.push_back(configParFloat["plantWaterUseEfficiency" + array_pos]);
        parameter.lowerSoilWaterFractionForPlantGppReduction.push_back(configParFloat["lowerSoilWaterFractionForPlantGppReduction" + array_pos]);
        parameter.lowerSoilWaterContentForPlantGppReduction.push_back(configParFloat["lowerSoilWaterContentForPlantGppReduction" + array_pos]);
        parameter.upperSoilWaterContentForPlantGppReduction.push_back(configParFloat["upperSoilWaterContentForPlantGppReduction" + array_pos]);
    }
}

/**
 * @brief Opens and parses the plant-traits parameter file, populating
 *        PFT-specific fields in the PARAMETER struct.
 *
 * Derives the file path from `path` (up three directory levels, then into
 * `parameters/`). Iterates over all expected plant-trait parameter names,
 * searches for each, validates format, extracts the data type, converts the
 * value, and finally calls transferPlantTraitsParameterValueToModelParameter().
 *
 * @param path      Path to the main configuration file; used to derive the
 *                  `parameters/` directory location.
 * @param utils     Utility object for path splitting and error handling.
 * @param parameter Model parameter struct; plant-trait fields are written here.
 */
void INPUT::openAndReadPlantTraitsFile(std::string path, UTILS utils, PARAMETER &parameter)
{
    char pathSeparator = utils.getPathSeparator();
    utils.strings.clear();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 3; it++)
    {
        plantTraitsDirectory = plantTraitsDirectory + utils.strings.at(it) + pathSeparator;
    }
    plantTraitsDirectory = plantTraitsDirectory + "parameters" + pathSeparator + parameter.plantTraitsFile;
    const char *filename = plantTraitsDirectory.c_str();

    for (auto par : parameter.plantTraitsParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
    {
        /* open file and search for name in all lines */
        plantTraitsFileOpened = false;
        plantTraitsFileOpened = searchParameterInInputFile(par, filename, utils);

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

/**
 * @brief Opens and parses the process-setup parameter file, then validates
 *        the resulting configuration for consistency.
 *
 * Derives the file path analogously to openAndReadPlantTraitsFile(). After
 * transferring values via transferProcessSetupParameterValueToModelParameter(),
 * calls checkIfProcessSetupParameterValuesAreConsistent() to detect mutually
 * exclusive module flag combinations.
 *
 * @param path      Path to the main configuration file; used to derive the
 *                  `parameters/` directory location.
 * @param utils     Utility object for path splitting and error handling.
 * @param parameter Model parameter struct; process-setup flags are written here.
 */
void INPUT::openAndReadProcessSetupFile(std::string path, UTILS utils, PARAMETER &parameter)
{
    char pathSeparator = utils.getPathSeparator();
    utils.strings.clear();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 3; it++)
    {
        processSetupDirectory = processSetupDirectory + utils.strings.at(it) + pathSeparator;
    }
    processSetupDirectory = processSetupDirectory + "parameters" + pathSeparator + parameter.processSetupFile;
    const char *filename = processSetupDirectory.c_str();

    for (auto par : parameter.processSetupParameterNames) /* parameterNames are listed in the class definition of PARAMETER (parameter.h)*/
    {
        /* open file and search for name in all lines */
        processSetupFileOpened = false;
        processSetupFileOpened = searchParameterInInputFile(par, filename, utils);

        /* check if the parameter name was found at least once and read in each line */
        checkIfParameterExistsAndExtractValues(utils, par, lineValues, lineNumbers, lineTypeValues);

        /* get the corresponding datatype for the extracted parameter value */
        extractDataTypeForExtractedValue(utils, par);

        /* convert the extracted value to its datatype, check for inconsistencies and map the value to the parameter name */
        convertAndCheckAndSetParameterValue(utils, par, parameterType, parameter);
    }

    /* transfer the mapped values of all parameter names to their variables in class PARAMETER */
    transferProcessSetupParameterValueToModelParameter(parameter);

    /* check for consistency of parameter settings */
    checkIfProcessSetupParameterValuesAreConsistent(utils, parameter);
}

/**
 * @brief Validates that process-setup flags are not set in mutually exclusive
 *        combinations.
 *
 * Checks for incompatible pairs such as:
 * - Both internal and external (BODIUM) soil modules active simultaneously.
 * - Both self-coupling get and set interfaces active simultaneously.
 * - Internal soil module combined with a coupling-interface flag.
 * - External BODIUM module combined with a coupling-interface flag.
 *
 * Calls `utils.handleError()` for each detected inconsistency.
 *
 * @param utils     Utility object for error reporting.
 * @param parameter Read-only; provides the boolean module activation flags.
 */
void INPUT::checkIfProcessSetupParameterValuesAreConsistent(UTILS utils, PARAMETER parameter)
{

    if (parameter.useInternalSoilModule && parameter.useExternalSoilModule_BODIUM)
    {
        utils.handleError("Inconsistent parameter settings: Both internal and external soil module are activated. Please check the process setup parameter file!");
    }

    if (parameter.useExternalSoilModule_selfCoupled_getVariables && parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        utils.handleError("Inconsistent parameter settings: Both getting and setting variables from/to the coupling interface are activated. Please check the process setup parameter file!");
    }

    if (parameter.useInternalSoilModule && parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        utils.handleError("Inconsistent parameter settings: Internal soil module and getting variables from the coupling interface are both activated. Please check the process setup parameter file!");
    }

    if (parameter.useInternalSoilModule && parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        utils.handleError("Inconsistent parameter settings: Internal soil module and setting variables to the coupling interface are both activated. Please check the process setup parameter file!");
    }

    if (parameter.useExternalSoilModule_BODIUM && parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        utils.handleError("Inconsistent parameter settings: External soil module BODIUM and getting variables from the coupling interface are both activated. Please check the process setup parameter file!");
    }

    if (parameter.useExternalSoilModule_BODIUM && parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        utils.handleError("Inconsistent parameter settings: External soil module BODIUM and setting variables to the coupling interface are both activated. Please check the process setup parameter file!");
    }
}

/**
 * @brief Copies all parsed process-setup values from the static maps into the
 *        PARAMETER struct.
 *
 * Transfers soil-module activation flags (internal, external BODIUM,
 * self-coupled get/set) and the stochastic simulation flag.
 *
 * @param parameter Model parameter struct; process-setup fields are written here.
 */
void INPUT::transferProcessSetupParameterValueToModelParameter(PARAMETER &parameter)
{
    parameter.useInternalSoilModule = configParBool["useInternalSoilModule"];
    parameter.useExternalSoilModule_BODIUM = configParBool["useExternalSoilModule_BODIUM"];
    parameter.useExternalSoilModule_selfCoupled_getVariables = configParBool["useExternalSoilModule_selfCoupled_getVariables"];
    parameter.useInternalSoilModule_selfCoupled_setVariables = configParBool["useInternalSoilModule_selfCoupled_setVariables"];

    parameter.stochasticSimulation = configParBool["stochasticSimulation"];
}

/**
 * @brief Opens and parses the weather file, populating the WEATHER struct with
 *        the daily meteorological time series for the simulation period.
 *
 * Derives the file path from `path` by navigating to
 * `scenarios/<location>/weather/`. Reads tab-separated rows (one per day)
 * with seven columns: date, precipitation, full-day temperature, daytime
 * temperature, PPFD, day length, and potential evapotranspiration. Only rows
 * within the simulation period (`firstYear`–`lastYear`) are stored.
 *
 * Raises an error if the file cannot be opened, if any row has fewer than seven
 * columns, or if the required start or end date is not present in the file.
 *
 * @param path      Path to the main configuration file; used to derive the
 *                  `scenarios/<location>/weather/` directory.
 * @param utils     Utility object for string splitting and error handling.
 * @param parameter Read-only; provides `firstYear`, `lastYear`, and file name.
 * @param weather   Weather struct; all daily time-series vectors are filled here.
 */
void INPUT::openAndReadWeatherFile(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather)
{
    char pathSeparator = utils.getPathSeparator();
    utils.strings.clear();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 3; it++)
    {
        weatherDirectory = weatherDirectory + utils.strings.at(it) + pathSeparator;
    }
    std::string location = utils.strings.at(utils.strings.size() - 1);
    char filenameSeparator = '_';
    utils.strings.clear();
    utils.splitString(location, filenameSeparator);
    location = utils.strings.at(0) + "_" + utils.strings.at(1);

    weatherDirectory = weatherDirectory + "scenarios" + pathSeparator + location + pathSeparator + "weather" + pathSeparator + parameter.weatherFile;
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
            if (!line.empty() && line.back() == '\r') /* if windows artifact, remove it*/
            {
                line.pop_back();
            }
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

/**
 * @brief Opens and parses the management file, populating the MANAGEMENT struct
 *        with all scheduled land-management events.
 *
 * Derives the file path from `path` by navigating to
 * `scenarios/<location>/management/`. Each data row contains a date followed
 * by columns for mowing height, fertiliser amount, irrigation amount, per-PFT
 * sowing amounts, and an information string. NaN values indicate that a
 * management action does not occur on that day.
 *
 * Events falling outside the simulation period are skipped with a warning.
 * Raises an error if the file cannot be opened, if any row has an unexpected
 * number of columns, or if a date string cannot be parsed.
 *
 * @param path       Path to the main configuration file; used to derive the
 *                   `scenarios/<location>/management/` directory.
 * @param utils      Utility object for string splitting and error handling.
 * @param parameter  Read-only; provides `pftCount`, simulation period bounds,
 *                   and `referenceJulianDayStart`.
 * @param management Management struct; mowing, fertilisation, irrigation, and
 *                   sowing event vectors are filled here.
 */
void INPUT::openAndReadManagementFile(std::string path, UTILS utils, PARAMETER &parameter, MANAGEMENT &management)
{
    char pathSeparator = utils.getPathSeparator();
    utils.strings.clear();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 3; it++)
    {
        manageDirectory = manageDirectory + utils.strings.at(it) + pathSeparator;
    }
    std::string location = utils.strings.at(utils.strings.size() - 1);
    char filenameSeparator = '_';
    utils.strings.clear();
    utils.splitString(location, filenameSeparator);
    location = utils.strings.at(0) + "_" + utils.strings.at(1);

    manageDirectory = manageDirectory + "scenarios" + pathSeparator + location + pathSeparator + "management" + pathSeparator + parameter.managementFile;
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
            if (!line.empty() && line.back() == '\r') /* if windows artifact, remove it*/
            {
                line.pop_back();
            }
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

/**
 * @brief Opens and parses the soil parameter file, populating texture and
 *        per-layer hydraulic properties in the SOIL and PARAMETER structs.
 *
 * Derives the file path from `path` by navigating to
 * `scenarios/<location>/soil/`. The file format is:
 * - Row 2: silt, clay, and sand content fractions (must sum to 1).
 * - Rows 5+: per-layer data with columns: layer index, layer width (cm),
 *   field capacity, permanent wilting point, porosity, and saturated
 *   hydraulic conductivity.
 *
 * Validates that texture fractions sum to 1 within TOLERANCE, that all
 * values are non-negative and in range, that pwp < fc < porosity holds for
 * every layer, and that the number of data rows matches the declared layer
 * count. Raises an error for any violation or if the file cannot be opened.
 *
 * @param path      Path to the main configuration file; used to derive the
 *                  `scenarios/<location>/soil/` directory.
 * @param utils     Utility object for string splitting and error handling.
 * @param parameter Model parameter struct; `numberOfSoilLayers`, `soilLayerWidth`,
 *                  and `soilDepth` are written here.
 * @param soil      Soil struct; texture fractions and per-layer hydraulic
 *                  property vectors are filled here.
 */
void INPUT::openAndReadSoilFile(std::string path, UTILS utils, PARAMETER &parameter, SOIL &soil)
{
    char pathSeparator = utils.getPathSeparator();
    utils.strings.clear();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 3; it++)
    {
        soilDirectory = soilDirectory + utils.strings.at(it) + pathSeparator;
    }
    std::string location = utils.strings.at(utils.strings.size() - 1);
    char filnameSeparator = '_';
    utils.strings.clear();
    utils.splitString(location, filnameSeparator);
    location = utils.strings.at(0) + "_" + utils.strings.at(1);

    soilDirectory = soilDirectory + "scenarios" + pathSeparator + location + pathSeparator + "soil" + pathSeparator + parameter.soilFile;
    const char *filename = soilDirectory.c_str();

    soilFileOpened = false;

    soil.siltContent = -1;
    soil.sandContent = -1;
    soil.clayContent = -1;

    parameter.soilDepth = 0;
    parameter.numberOfSoilLayers = 0;
    parameter.soilLayerWidth.clear();

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
        soilFileOpened = true;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r') /* if windows artifact, remove it*/
            {
                line.pop_back();
            }
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

                if (utils.strings.size() == 6)
                {
                    // skip layer number in column 1 (index 0)
                    parameter.numberOfSoilLayers += 1;

                    value = utils.strings.at(1).c_str();
                    parameter.soilLayerWidth.push_back(atof(value));
                    parameter.soilDepth += atof(value);

                    value = utils.strings.at(2).c_str();
                    soil.fieldCapacity.push_back(atof(value));

                    value = utils.strings.at(3).c_str();
                    soil.permanentWiltingPoint.push_back(atof(value));

                    value = utils.strings.at(4).c_str();
                    soil.porosity.push_back(atof(value));

                    value = utils.strings.at(5).c_str();
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
        if ((contentSum < (1.0 - TOLERANCE)) || (contentSum > (1.0 + TOLERANCE)))
        {
            utils.handleError("Error (soil input): sand, silt and clay content do not sum up to one. Please check the soil file.");
        }

        if (soil.sandContent < 0 || soil.siltContent < 0 || soil.clayContent < 0 || soil.sandContent > 1 || soil.siltContent > 1 || soil.clayContent > 1)
        {
            utils.handleError("Error (soil input): sand, silt or clay content are out of range. Please check the soil file.");
        }

        if (soil.permanentWiltingPoint.size() != parameter.numberOfSoilLayers || soil.fieldCapacity.size() != parameter.numberOfSoilLayers || soil.porosity.size() != parameter.numberOfSoilLayers || soil.saturatedHydraulicConductivity.size() != parameter.numberOfSoilLayers)
        {
            utils.handleError("Error (soil input): there are not enough or too many values for " + std::to_string(parameter.numberOfSoilLayers) + " soil layers. Please check the soil file.");
        }
    }
    else
    {
        utils.handleError("Error (soil input): The soil file cannot be opened. Please check the name in the configuration file.");
    }
}
