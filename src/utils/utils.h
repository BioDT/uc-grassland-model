#pragma once
#include <iostream>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h> // For mkdir on Unix-like systems
#endif
#include <vector>
#include <string>
#include <locale>
#include <sstream>
#include <limits>
#include "../module_init/constants.h"

/**
 * @class UTILS
 * @brief A utility class providing various helper functions for string manipulation, date calculations, and error handling.
 *
 * This class contains functions for splitting strings, converting strings to boolean, handling errors and warnings,
 * parsing numbers from strings, and calculating dates. It is designed to assist in handling common utility operations
 * required in the program.
 */
class UTILS
{
public:
    UTILS();
    ~UTILS();

    /**
     * @brief Vector to store strings resulting from splitting a string.
     */
    std::vector<std::string> strings;

    /**
     * @brief Splits a string `str` into substrings based on a specified `separator` character and stores the results in the `strings` vector.
     */
    void splitString(std::string str, char separator);

    /**
     * @brief Converts a string to a boolean value.
     */
    bool stringToBool(const std::string &str);

    /**
     * @brief Checks if a given value is negative and handles it as an error if so.
     */
    void checkForNegativeValue(double valueToCheck, const std::string context);

    /**
     * @brief Handles an error by displaying an error message and terminating the program.
     */
    void handleError(std::string errorString);

    /**
     * @brief Handles a warning by displaying a warning message.
     */
    void handleWarning(std::string warnString);

    /**
     * @brief Retrieves the file ending (extension) from a given file name.
     */
    std::string getFileEnding(std::string file);

    /**
     * @brief Calculates the Julian day from a given date.
     */
    int calculateJulianDayFromDate(int day, int month, int year);

    /**
     * @brief Calculates the day count from a given date and start day.
     */
    int calculateDayCountFromDate(int day, int month, int year, int startDay);

    /**
     * @brief Calculates the date from a given day count and start day.
     */
    int calculateDateFromDayCount(UTILS utils, int dayCount, int startDay, std::string keywordForReturn);

    /**
     * @brief Parses a string into a double value or returns NaN if the string represents "NaN".
     */
    double parseDoubleOrNaN(const std::string &str);

    /**
     * @brief Parses a string into an integer value or returns a NaN equivalent if the string represents "NaN".
     */
    int parseIntegerOrNaN(const std::string &str);

    /**
     * @brief Gets the path separator character based on the operating system (e.g., '/' for Unix-like systems and '\\' for Windows).
     */
    inline char getPathSeparator()
    {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }
};
