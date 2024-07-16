#include <iostream>
#include <direct.h>
#include <locale>
#include "utils.h"

UTILS::UTILS(){};
UTILS::~UTILS(){};

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