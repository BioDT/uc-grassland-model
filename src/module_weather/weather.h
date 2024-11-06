#pragma once
#include <vector>
#include <iostream>

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
    * @brief A vector storing the dates of the weather data.
    *
    * Each entry corresponds to a specific day for which weather data is recorded.
    */
   std::vector<std::string> weatherDates;

   /**
    * @brief A vector storing precipitation values for each corresponding date.
    *
    * The values represent the amount of precipitation measured (in mm / day).
    */
   std::vector<double> precipitation;

   /**
    * @brief A vector storing the full-day average air temperature.
    *
    * The values represent the average air temperature for the full day (average of 24 hours, in °C).
    */
   std::vector<double> fullDayAirTemperature;

   /**
    * @brief A vector storing the daytime air temperature.
    *
    * The values represent the average air temperature during daylight hours (from astronomic sunrise to sunset, in °C).
    */
   std::vector<double> dayTimeAirTemperature;

   /**
    * @brief A vector storing photosynthetic photon flux density (PPFD) values.
    *
    * The values represent the light intensity available for photosynthesis (in µmol(photons)/m²/s).
    */
   std::vector<double> photosyntheticPhotonFluxDensity;

   /**
    * @brief A vector storing potential evapotranspiration values.
    *
    * The values represent the potential evapotranspiration (mm / day).
    */
   std::vector<double> potEvapoTranspiration;

   /**
    * @brief A vector storing the length of the day.
    *
    * The values represent the astronomically duration of daylight (from sunrise to sunset, in hours / day).
    */
   std::vector<double> dayLength;
};