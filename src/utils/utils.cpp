#include "utils.h"

UTILS::UTILS() {};
UTILS::~UTILS() {};

/* Split a string str based on the given separator and save substrings in vector strings */
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

/* Convert a string into a boolean variable */
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

/* Handle runtime error */
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

/* Handle warnings & information written to stdcerr */
void UTILS::handleWarning(std::string warnString)
{
   std::cerr << warnString << std::endl;
}

/* Get the file-ending after dot */
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

/* Calculate the julian day of a given date */
int UTILS::calculateJulianDayFromDate(int day, int month, int year)
{
   int a = (14 - month) / 12;
   int y = year + 4800 - a;
   int m = month + 12 * a - 3;

   int julianDay = day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
   return julianDay;
}

/* Calculate the day (int number) of a given date, counted starting from 1 for a reference day (given as julian day) */
int UTILS::calculateDayCountFromDate(int day, int month, int year, int startDay)
{
   int dayCount = calculateJulianDayFromDate(day, month, year) - startDay + 1;
   return dayCount;
}

/* check if a parameter value is a double number or NaN */
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

/* check if a parameter value is an integer number or NaN */
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