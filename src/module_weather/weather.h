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

    /**
     * @brief Dates of the weather data entries, stored as strings in "YYYY-MM-DD" format.
     */
    std::vector<std::string> weatherDates;

    /**
     * @brief Daily precipitation values corresponding to `weatherDates` (mm).
     */
    std::vector<double> precipitation;

    /**
     * @brief Full-day air temperature values corresponding to `weatherDates` (°C).
     */
    std::vector<double> fullDayAirTemperature;

    /**
     * @brief Daytime air temperature values corresponding to `weatherDates` (°C).
     */
    std::vector<double> dayTimeAirTemperature;

    /**
     * @brief Photosynthetic photon flux density values corresponding to `weatherDates` (µmol m⁻² s⁻¹).
     */
    std::vector<double> photosyntheticPhotonFluxDensity;

    /**
     * @brief Potential evapotranspiration values corresponding to `weatherDates` (mm).
     */
    std::vector<double> potEvapoTranspiration;

    /**
     * @brief Day length values corresponding to `weatherDates` (hours).
     */
    std::vector<double> dayLength;

    // =========================================================================
    // Methods
    // =========================================================================

    /**
     * @brief Computes the total annual precipitation for a specific calendar year.
     */
    double calculateAnnualPrecipitationOfSpecificYear(UTILS utils, PARAMETER parameter, int year);

    /**
     * @brief Computes the mean annual precipitation over a range of calendar years.
     */
    double calculateMeanAnnualPrecipitationFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB);

    /**
     * @brief Computes the mean full-day air temperature for a specific calendar year.
     */
    double calculateAverageAirTemperatureOfSpecificYear(UTILS utils, PARAMETER parameter, int specificYear);

    /**
     * @brief Computes the mean annual air temperature over a range of calendar years.
     */
    double calculateAverageAirTemperatureFromYearAToYearB(UTILS utils, PARAMETER parameter, int yearA, int yearB);
};