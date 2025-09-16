#include "weather.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"

WEATHER::WEATHER() {};
WEATHER::~WEATHER() {};

double WEATHER::calculateAnnualPrecipitationOfSpecificYear(UTILS utils, PARAMETER parameter, int specificYear)
{
   double annualPrecipitation = 0.0;
   double numberOfDaysWithinYear = 0;

   for (int i = 0; i < weatherDates.size(); i++)
   {
      utils.strings.clear();
      utils.splitString(weatherDates.at(i), '-');
      if (utils.strings.size() == 3)
      {
         int year = std::stoi(utils.strings.at(0).c_str());
         if (year == specificYear)
         {
            annualPrecipitation += precipitation.at(i);
            numberOfDaysWithinYear += 1.0;
         }
      }
   }

   return (annualPrecipitation / numberOfDaysWithinYear);
}

double WEATHER::calculateMeanAnnualPrecipitationFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB)
{
   double meanAnnualPrecipitation = 0.0;
   double numberOfYears = yearB - yearA + 1.0;

   for (int i = yearA; i <= yearB; i++)
   {
      meanAnnualPrecipitation += calculateAnnualPrecipitationOfSpecificYear(utils, parameter, i);
   }

   return (meanAnnualPrecipitation / numberOfYears);
}

double WEATHER::calculateAverageAirTemperatureOfSpecificYear(UTILS utils, PARAMETER parameter, int specificYear)
{
   double averageTemperature = 0.0;
   double numberOfDaysWithinYear = 0;

   for (int i = 0; i < weatherDates.size(); i++)
   {
      utils.strings.clear();
      utils.splitString(weatherDates.at(i), '-');
      if (utils.strings.size() == 3)
      {
         int year = std::stoi(utils.strings.at(0).c_str());
         if (year == specificYear)
         {
            averageTemperature += fullDayAirTemperature.at(i);
            numberOfDaysWithinYear += 1.0;
         }
      }
   }

   return (averageTemperature / numberOfDaysWithinYear);
}

double WEATHER::calculateAverageAirTemperatureFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB)
{
   double averageAirTemperature = 0.0;
   double numberOfYears = yearB - yearA + 1.0;

   for (int i = yearA; i <= yearB; i++)
   {
      averageAirTemperature += calculateAverageAirTemperatureOfSpecificYear(utils, parameter, i);
   }

   return (averageAirTemperature / numberOfYears);
}
