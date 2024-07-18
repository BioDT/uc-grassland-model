#pragma once
#include <vector>
#include <iostream>

class WEATHER
{
public:
   WEATHER();
   ~WEATHER();

   // weather input variables
   std::vector<std::string> weatherDates;
   std::vector<double> precipitation;
   std::vector<double> airTemperature;
   std::vector<double> photosyntheticPhotonFluxDensity;
   std::vector<double> potEvapoTranspiration;
   std::vector<double> dayLength;

   int calculateAstronomicDayLength(void);
   double calculatePotentialEvapoTranspiration(void);
};