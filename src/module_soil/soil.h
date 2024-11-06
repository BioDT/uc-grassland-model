#pragma once
#include "../module_init/constants.h"
#include "../module_parameter/parameter.h"
#include <vector>
#include <iostream>

class SOIL
{
public:
   SOIL();
   ~SOIL();

   /* soil input parameter */
   double siltContent;
   double sandContent;
   double clayContent;
   std::vector<double> permanentWiltingPoint;
   std::vector<double> fieldCapacity;
   std::vector<double> porosity;
   std::vector<double> saturatedHydraulicConductivity;

   double greenCarbonSurfaceLitter;
   double brownCarbonSurfaceLitter;
   double rootCarbonSoilLitter;
   double seedCarbonSoilLitter;

   double greenNitrogenSurfaceLitter;
   double brownNitrogenSurfaceLitter;
   double rootNitrogenSoilLitter;
   double seedNitrogenSoilLitter;

   void transferDyingPlantPartsToLitterPools(PARAMETER parameter, int number, double biomass, int typeOfMaterial, int pft);
};