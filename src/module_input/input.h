/**
 * @file input.h
 * @brief Declares the INPUT class, which handles all file-based input parsing
 *        for the grassland model.
 *
 * INPUT reads six types of text files and populates the corresponding model
 * structs before the simulation starts:
 * - Configuration file (simulation settings, file names, output flags)
 * - Plant traits file (PFT-specific physiological and allometric parameters)
 * - Process setup file (sub-model activation flags)
 * - Weather file (daily meteorological time series)
 * - Soil file (texture and per-layer hydraulic properties)
 * - Management file (mowing, fertilisation, irrigation, and sowing schedules)
 *
 * All files use a tab-separated text format. Parameters in configuration and
 * trait files are identified by keyword and annotated with a `\datatype` line
 * that declares the expected C++ type.
 */
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

/**
 * @class INPUT
 * @brief Parses all model input files and transfers their contents to the
 *        corresponding model data structures.
 */
class INPUT
{
public:
    INPUT();
    ~INPUT();

    /** @brief Resolved path to the plant traits parameter file. */
    std::string plantTraitsDirectory;
    /** @brief Resolved path to the process setup parameter file. */
    std::string processSetupDirectory;
    /** @brief Resolved path to the weather input file. */
    std::string weatherDirectory;
    /** @brief Resolved path to the management input file. */
    std::string manageDirectory;
    /** @brief Resolved path to the soil input file. */
    std::string soilDirectory;

    /** @brief `true` after the plant traits file has been successfully opened. */
    bool plantTraitsFileOpened;
    /** @brief `true` after the weather file has been successfully opened. */
    bool weatherFileOpened;
    /** @brief `true` after the management file has been successfully opened. */
    bool managementFileOpened;
    /** @brief `true` after the soil file has been successfully opened. */
    bool soilFileOpened;
    /** @brief `true` after the process setup file has been successfully opened. */
    bool processSetupFileOpened;

    /** @brief Text of every line in which the current keyword was found. */
    std::vector<std::string> lineValues;
    /** @brief `\datatype` descriptor lines immediately following each keyword match. */
    std::vector<std::string> lineTypeValues;
    /** @brief Line numbers corresponding to entries in `lineValues`. */
    std::vector<int> lineNumbers;

    /**
     * @brief Indices (into `lineValues`) of lines whose first token matches the
     *        keyword, i.e. lines in the correct `keyword <value>` format.
     */
    std::vector<int> keywordLineNumbers;

    /**
     * @brief Value token(s) extracted from the correctly formatted keyword lines.
     *        For array parameters, one entry per element.
     */
    std::vector<std::string> keywordLineValues;
    /** @brief Raw string value of the most recently parsed parameter (unused externally). */
    std::string parameterValue;
    /** @brief Data-type string of the most recently parsed parameter (e.g. `"float"`). */
    std::string parameterType;

    /**
     * @brief Maps parsed integer parameter values to their keyword names.
     * Also used for `date` and `date-array` types (stored as day-count offsets).
     */
    static std::map<std::string, int> configParInt;
    /** @brief Maps parsed float parameter values to their keyword names. */
    static std::map<std::string, float> configParFloat;
    /** @brief Maps parsed boolean parameter values to their keyword names. */
    static std::map<std::string, bool> configParBool;
    /** @brief Maps parsed string parameter values to their keyword names. */
    static std::map<std::string, std::string> configParString;

    /**
     * @brief Reads and parses all input files required to run the simulation.
     */
    void getInputData(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather, SOIL &soil, MANAGEMENT &management);

    /**
     * @brief Opens and parses the main configuration file.
     */
    void openAndReadConfigurationFile(std::string config, UTILS utils, PARAMETER &parameter);

    /**
     * @brief Opens and parses the plant traits parameter file.
     */
    void openAndReadPlantTraitsFile(std::string config, UTILS utils, PARAMETER &parameter);

    /**
     * @brief Opens and parses the process setup parameter file, then validates
     *        the flags for mutual exclusivity.
     */
    void openAndReadProcessSetupFile(std::string path, UTILS utils, PARAMETER &parameter);

    /**
     * @brief Opens and parses the weather file for the simulation period.
     */
    void openAndReadWeatherFile(std::string path, UTILS utils, PARAMETER &parameter, WEATHER &weather);

    /**
     * @brief Opens and parses the soil file for texture and hydraulic properties.
     */
    void openAndReadSoilFile(std::string path, UTILS utils, PARAMETER &parameter, SOIL &soil);

    /**
     * @brief Opens and parses the management file for scheduled land-use events.
     */
    void openAndReadManagementFile(std::string path, UTILS utils, PARAMETER &parameter, MANAGEMENT &management);

    /**
     * @brief Searches a parameter file for all lines containing a keyword.
     */
    bool searchParameterInInputFile(std::string keyword, const char *filename, UTILS utils);

    /**
     * @brief Checks that a keyword was found at least once and extracts value lines.
     */
    void checkIfParameterExistsAndExtractValues(UTILS utils, std::string keyword, std::vector<std::string> lineValues, std::vector<int> lineNumbers, std::vector<std::string> lineTypeValues);

    /**
     * @brief Filters candidate lines to those in `keyword <value>` format and
     *        extracts the value token(s) into `keywordLineValues`.
     */
    void extractLinesOfCorrectFormat(UTILS utils, std::string keyword, std::vector<std::string> lineValues);

    /**
     * @brief Reads the `\datatype:<type>` descriptor from the line following
     *        the keyword match and stores it in `parameterType`.
     */
    void extractDataTypeForExtractedValue(UTILS utils, std::string keyword);

    /**
     * @brief Converts a raw value string to its declared type, range-checks it,
     *        and stores it in the appropriate static map.
     */
    void convertAndCheckAndSetParameterValue(UTILS utils, std::string keyword, std::string parameterType, PARAMETER parameter);

    /**
     * @brief Copies all configuration map values into the PARAMETER struct.
     */
    void transferConfigParameterValueToModelParameter(PARAMETER &parameter, UTILS utils);

    /**
     * @brief Copies all plant-trait map values into the PARAMETER struct.
     */
    void transferPlantTraitsParameterValueToModelParameter(PARAMETER &parameter);

    /**
     * @brief Copies all process-setup map values into the PARAMETER struct.
     */
    void transferProcessSetupParameterValueToModelParameter(PARAMETER &parameter);

    /**
     * @brief Validates that process-setup flags are not set in mutually exclusive
     *        combinations.
     */
    void checkIfProcessSetupParameterValuesAreConsistent(UTILS utils, PARAMETER parameter);
};
