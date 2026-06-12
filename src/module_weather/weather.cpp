#include "weather.h"
#include "../module_parameter/parameter.h"
#include "../utils/utils.h"

WEATHER::WEATHER() {};
WEATHER::~WEATHER() {};

/**
 * @brief Computes the total annual precipitation for a specific calendar year.
 *
 * Iterates over `weatherDates`, parses the year from each `"YYYY-MM-DD"` entry,
 * and sums the corresponding `precipitation` values for days that fall within
 * `specificYear`. The result is converted from mm to cm before returning.
 *
 * @param utils       Utility object used for string splitting.
 * @param parameter   Reserved (not used inside this function; kept for interface
 *                    consistency with other weather helper functions).
 * @param specificYear Calendar year to aggregate (e.g. 2020).
 * @return Annual precipitation for `specificYear` (cm).
 */
double WEATHER::calculateAnnualPrecipitationOfSpecificYear(UTILS utils, PARAMETER parameter, int specificYear)
{
    double annualPrecipitation = 0.0;

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
            }
        }
    }

    return (annualPrecipitation / 10.0); // convert from mm to cm
}

/**
 * @brief Computes the mean annual precipitation over a range of calendar years.
 *
 * Calls `calculateAnnualPrecipitationOfSpecificYear()` for each year from
 * `yearA` to `yearB` (inclusive) and returns the arithmetic mean.
 *
 * @param utils    Utility object passed through to the per-year helper.
 * @param parameter Reserved; passed through to the per-year helper.
 * @param yearA    First calendar year of the averaging period (inclusive).
 * @param yearB    Last calendar year of the averaging period (inclusive).
 * @return Mean annual precipitation over [yearA, yearB] (cm yr⁻¹).
 */
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

/**
 * @brief Computes the mean full-day air temperature for a specific calendar year.
 *
 * Iterates over `weatherDates`, parses the year from each `"YYYY-MM-DD"` entry,
 * and accumulates `fullDayAirTemperature` values for days within `specificYear`.
 * Returns the arithmetic mean over all matching days.
 *
 * @param utils       Utility object used for string splitting.
 * @param parameter   Reserved; kept for interface consistency.
 * @param specificYear Calendar year to aggregate.
 * @return Mean full-day air temperature for `specificYear` (°C).
 */
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

/**
 * @brief Computes the mean annual air temperature over a range of calendar years.
 *
 * Calls `calculateAverageAirTemperatureOfSpecificYear()` for each year from
 * `yearA` to `yearB` (inclusive) and returns the arithmetic mean of those
 * annual means.
 *
 * @param utils     Utility object passed through to the per-year helper.
 * @param parameter Reserved; passed through to the per-year helper.
 * @param yearA     First calendar year of the averaging period (inclusive).
 * @param yearB     Last calendar year of the averaging period (inclusive).
 * @return Mean annual air temperature over [yearA, yearB] (°C yr⁻¹).
 */
double WEATHER::calculateAverageAirTemperatureFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB)
{
    double averageAirTemperature = 0.0;
    double numberOfYears = yearB - yearA + 1.0;

    for (int i = yearA; i <= yearB; i++)
    {
        double averageTemperature = calculateAverageAirTemperatureOfSpecificYear(utils, parameter, i);
        averageAirTemperature += averageTemperature;
    }

    return (averageAirTemperature / numberOfYears);
}
