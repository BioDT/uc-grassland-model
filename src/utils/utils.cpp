#include "utils.h"

UTILS::UTILS() {};
UTILS::~UTILS() {};

/**
 * @brief Splits a string based on the given separator character and stores the resulting substrings in the strings vector.
 *
 * This function iterates through the input string `str`, splits it at each occurrence of the separator character,
 * and stores each substring in the `strings` vector. If no separator is found, the entire string is treated as a single substring.
 *
 * @param str The input string to be split.
 * @param separator The character used to split the string.
 */
void UTILS::splitString(std::string str, char separator)
{
   int startIndex = 0, endIndex = 0;
   for (int it = 0; it <= str.size(); it++)
   {
      if (str[it] == separator || it == str.size())
      {
         endIndex = it;
         std::string temp;
         temp.append(str, startIndex, endIndex - startIndex);
         strings.push_back(temp);
         startIndex = endIndex + 1;
      }
   }
}

/**
 * @brief Converts a string into a boolean value.
 *
 * This function converts the input string `str` into lowercase and checks whether it corresponds to
 * a boolean value. The function returns `true` if the string is "true" or "1", and `false` if the
 * string is "false" or "0". If the input string does not match any valid boolean representation,
 * an exception is thrown.
 *
 * @param str The input string to convert to a boolean.
 * @return true If the string represents a true value ("true" or "1").
 * @return false If the string represents a false value ("false" or "0").
 *
 * @throws std::invalid_argument If the string cannot be converted to a valid boolean.
 */
bool UTILS::stringToBool(const std::string &str)
{
   // Convert to lower case letters
   std::string lowercaseStr;
   for (char c : str)
   {
      lowercaseStr += std::tolower(c);
   }

   // Check if string is true or false
   if (lowercaseStr == "true" || lowercaseStr == "1")
   {
      return true;
   }
   else if (lowercaseStr == "false" || lowercaseStr == "0")
   {
      return false;
   }
   else
   {
      throw std::invalid_argument("Invalid boolean value: " + str);
   }
}

/**
 * @brief Handles runtime errors by throwing and catching a runtime_error.
 *
 * This function takes an error message as input, throws it as a `std::runtime_error`,
 * and immediately catches it. The error message is then printed to the standard error output (stderr).
 *
 * @param errorString The error message to be handled.
 */
void UTILS::handleError(std::string errorString)
{
   try
   {
      throw std::runtime_error(errorString);
   }
   catch (const std::runtime_error &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

/**
 * @brief Handles warnings by printing the warning message to the standard error output (stderr).
 *
 * This function takes a warning message as input and writes it to the standard error stream (stderr).
 * It is used for logging warnings and informational messages.
 *
 * @param warnString The warning message to be displayed.
 */
void UTILS::handleWarning(std::string warnString)
{
   std::cerr << warnString << std::endl;
}

/**
 * @brief Extracts the file extension from a given file name.
 *
 * This function splits the input string `file` at the dot ('.') to extract the file extension.
 * If the string contains no dot, an error is handled via the `handleError` function.
 * Any spaces after the file extension are removed.
 *
 * @param file The file name from which to extract the extension.
 * @return A string representing the file extension, or an empty string if no valid extension is found.
 *
 * @throws std::runtime_error If the input string does not contain a dot.
 */
std::string UTILS::getFileEnding(std::string file)
{
   std::string fileEnding = "";
   strings.clear();
   splitString(file, '.'); // split string based on dot
   if (strings.size() > 1)
   {
      fileEnding = strings.at(strings.size() - 1);
      strings.clear();
      splitString(fileEnding, ' '); // remove any spaces after file-ending
      if (strings.size() > 0)
      {
         fileEnding = strings.at(0);
      }
   }
   else
   {
      handleError("String does not contain a dot to be splitted by.");
   }

   return (fileEnding);
}

/**
 * @brief Calculates the Julian day number for a given date.
 *
 * This function computes the Julian day number corresponding to a specific day, month, and year.
 * The Julian day is the continuous count of days since the beginning of the Julian Period
 * (January 1, 4713 BC in the Julian calendar).
 *
 * @param day The day of the month (1-31).
 * @param month The month of the year (1-12).
 * @param year The year (e.g., 2024).
 * @return The Julian day number corresponding to the given date.
 */
int UTILS::calculateJulianDayFromDate(int day, int month, int year)
{
   int a = (14 - month) / 12;  // Adjustment needed? (1 for January/February, 0 otherwise).
   int y = year + 4800 - a;    // Year adjusted for the Julian calendar.
   int m = month + 12 * a - 3; // Month adjusted for the Julian calendar (March = 0, April = 1, ..., February = 11).

   int julianDay = day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
   return julianDay;
}

/**
 * @brief Calculates the number of days from a given reference day to a specific date.
 *
 * This function computes the number of days between a reference date, represented by the Julian day `startDay`,
 * and a specific day, month, and year. The count starts from 1 for the reference day.
 *
 * @param day The day of the target date (1-31).
 * @param month The month of the target date (1-12).
 * @param year The year of the target date (e.g., 2024).
 * @param startDay The Julian day number of the reference date, from which the counting starts.
 * @return The number of days from the reference date to the given date, starting with 1 for the reference date.
 */
int UTILS::calculateDayCountFromDate(int day, int month, int year, int startDay)
{
   int dayCount = calculateJulianDayFromDate(day, month, year) - startDay + 1;
   return dayCount;
}

/**
 * @brief Calculates a specific date component (day, month, or year) based on a day count from a start date.
 *
 * This function calculates the date (day, month, or year) by adding a given day count to a start date.
 * Depending on the `keywordForReturn` parameter, the function returns the day, month, or year of the calculated date.
 *
 * @param utils A UTILS object used for error handling methods.
 * @param dayCount The number of days to add to the start date.
 * @param startDay The Julian day number of the start date.
 * @param keywordForReturn A keyword ("day", "month", or "year") to specify which part of the date should be returned.
 *                         It accepts any case variation (e.g., "Day", "MONTH").
 * @return int Returns the calculated day, month, or year depending on `keywordForReturn`.
 * @throws Outputs an error message if `keywordForReturn` is not "day", "month", or "year" (in any capitalization).
 *
 * @note The function accounts for the Gregorian calendar and includes leap year adjustments.
 */
int UTILS::calculateDateFromDayCount(UTILS utils, int dayCount, int startDay, std::string keywordForReturn)
{
   int a = dayCount + startDay - 1 + 32044;
   int b = (4 * a + 3) / 146097;
   int c = a - (146097 * b) / 4;
   int d = (4 * c + 3) / 1461;
   int e = c - (1461 * d) / 4;
   int m = (5 * e + 2) / 153;

   int day = e - (153 * m + 2) / 5 + 1;
   int month = m + 3 - 12 * (m / 10);
   int year = 100 * b + d - 4800 + (m / 10);

   if (keywordForReturn == "day" || keywordForReturn == "Day" || keywordForReturn == "DAY")
   {
      return day;
   }
   else if (keywordForReturn == "month" || keywordForReturn == "Month" || keywordForReturn == "MONTH")
   {
      return month;
   }
   else if (keywordForReturn == "year" || keywordForReturn == "Year" || keywordForReturn == "YEAR")
   {
      return year;
   }
   else
   {
      utils.handleError("The keyword for the return value" + keywordForReturn + "does not match any of the options day, month or year.");
   }
}

/**
 * @brief Checks if a given string represents a double value or NaN.
 *
 * This function attempts to convert the input string `str` into a double number.
 * If the conversion is successful, the double value is returned. If the string represents "NaN"
 * (in any capitalization), the function returns a `NaN` value. If the string is neither a valid
 * number nor "NaN", an exception is thrown.
 *
 * @param str The string to be parsed.
 * @return The parsed double value, or NaN if the string represents "NaN".
 *
 * @throws std::invalid_argument If the input string is neither a valid number nor "NaN".
 */
double UTILS::parseDoubleOrNaN(const std::string &str)
{
   std::stringstream ss(str);
   double value;

   // convert the string into a number
   if (ss >> value)
   {
      return value;
   }
   else
   {
      // check if the string is "NaN"
      if (str == "NaN" || str == "nan" || str == "NAN")
      {
         return std::numeric_limits<double>::quiet_NaN();
      }
      else
      {
         // string is no number and not NaN
         throw std::invalid_argument("Invalid input: " + str);
      }
   }
}

/**
 * @brief Checks if a given string represents an integer value or NaN.
 *
 * This function attempts to convert the input string `str` into an integer.
 * If the conversion is successful, the integer value is returned. If the string represents "NaN"
 * (in any capitalization), the function returns `std::numeric_limits<int>::min()` as a NaN equivalent for integers.
 * If the string is neither a valid integer nor "NaN", an exception is thrown.
 *
 * @param str The string to be parsed.
 * @return The parsed integer value, or `std::numeric_limits<int>::min()` if the string represents "NaN".
 *
 * @throws std::invalid_argument If the input string is neither a valid integer nor "NaN".
 */
int UTILS::parseIntegerOrNaN(const std::string &str)
{
   std::stringstream ss(str);
   int value;

   // convert the string into a number
   if (ss >> value)
   {
      return value;
   }
   else
   {
      // check if the string is "NaN"
      if (str == "NaN" || str == "nan" || str == "NAN")
      {
         return std::numeric_limits<int>::min();
      }
      else
      {
         // string is no number and not NaN
         throw std::invalid_argument("Invalid input: " + str);
      }
   }
}