#include "soil.h"

SOIL::SOIL() {};
SOIL::~SOIL() {};

void SOIL::transferDyingPlantPartsToLitterPools(PARAMETER parameter, int number, double biomass, int typeOfMaterial, int pft)
{
   if (typeOfMaterial == 0)
   {
      greenCarbonSurfaceLitter += (number * (biomass * carbonContentOdm));
      greenNitrogenSurfaceLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioGreenLeaves[pft]));
   }
   else if (typeOfMaterial == 1)
   {
      brownCarbonSurfaceLitter += (number * (biomass * carbonContentOdm));
      brownNitrogenSurfaceLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioBrownLeaves[pft]));
   }
   else if (typeOfMaterial == 2)
   {
      rootCarbonSoilLitter += (number * (biomass * carbonContentOdm));
      rootNitrogenSoilLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioRoots[pft]));
   }
   else if (typeOfMaterial == 3)
   {
      seedCarbonSoilLitter += (number * (biomass * carbonContentOdm));
      seedNitrogenSoilLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioSeeds[pft]));
   }
}