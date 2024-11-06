#pragma once
#include "../module_init/constants.h"
#include <math.h>

/**
 * @brief Represents allometric relationships for plants.
 *
 * The `ALLOMETRY` class provides methods to calculate various plant metrics
 * based on allometric equations. These methods allow for conversions between
 * biomass, height, width, and other related geometric plant state variables.
 */
class ALLOMETRY
{
public:
   ALLOMETRY();
   ~ALLOMETRY();

   double laiFromBiomassAreaSla(double biomass, double area, double sla);
   double areaFromWidth(double width);
   double heightFromBiomassWidthForm(double biomass, double width, double form);
   double heightFromWidthHwr(double width, double hwr);
   double widthFromHeightHwr(double height, double hwr);
   double heightFromBiomassHwrForm(double biomass, double hwr, double form);
   double widthFromBiomassHwrForm(double biomass, double hwr, double form);
   double biomassFromHeightWidthForm(double height, double width, double form);
};