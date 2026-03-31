#pragma once
#include <vector>
#include <iostream>
#include "../utils/utils.h"
#include "../module_parameter/parameter.h"

/**
 * @brief Represents weather data for the simulation.
 *
 * The `WEATHER` class stores various weather-related input variables
 * that can be used in environmental modeling and simulations.
 * It includes vectors to hold weather dates, precipitation data,
 * air temperature values, and other relevant meteorological parameters.
 */
class WEATHER
{
public:
    WEATHER();
    ~WEATHER();

    std::vector<std::string> weatherDates;
    std::vector<double> precipitation;
    std::vector<double> fullDayAirTemperature;
    std::vector<double> dayTimeAirTemperature;
    std::vector<double> photosyntheticPhotonFluxDensity;
    std::vector<double> potEvapoTranspiration;
    std::vector<double> dayLength;

    double calculateAnnualPrecipitationOfSpecificYear(UTILS utils, PARAMETER parameter, int year);
    double calculateMeanAnnualPrecipitationFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB);
    double calculateAverageAirTemperatureOfSpecificYear(UTILS utils, PARAMETER parameter, int specificYear);
    double calculateAverageAirTemperatureFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB);
};