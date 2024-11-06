#pragma once
#include <iostream>
#include <direct.h>
#include <vector>
#include <string>
#include <locale>
#include <sstream>
#include <limits>

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

   void splitString(std::string str, char separator);
   bool stringToBool(const std::string &str);
   void handleError(std::string errorString);
   void handleWarning(std::string warnString);
   std::string getFileEnding(std::string file);
   int calculateJulianDayFromDate(int day, int month, int year);
   int calculateDayCountFromDate(int day, int month, int year, int startDay);
   int calculateDateFromDayCount(UTILS utils, int dayCount, int startDay, std::string keywordForReturn);
   double parseDoubleOrNaN(const std::string &str);
   int parseIntegerOrNaN(const std::string &str);
};
