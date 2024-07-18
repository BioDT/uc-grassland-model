#pragma once
#include <iostream>
#include <direct.h>
#include <vector>
#include <string>

class UTILS
{
public:
   UTILS();
   ~UTILS();

   std::vector<std::string> strings;
   void splitString(std::string str, char separator);
   bool stringToBool(const std::string &str);
   void handleError(std::string errorString);
   std::string getFileEnding(std::string file);
   int calculateJulianDayFromDate(int day, int month, int year);
};
