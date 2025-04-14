#include "allometry.h"

ALLOMETRY::ALLOMETRY() {};
ALLOMETRY::~ALLOMETRY() {};

/**
 * @brief Calculates the Leaf Area Index (LAI) from shoot biomass, ground area, and specific leaf area (SLA).
 *
 * @param biomass The total shoot biomass (in g).
 * @param area The ground area that is covered by the plant (in square cm).
 * @param sla The specific leaf area (in square cm per g).
 * @return The calculated Leaf Area Index (LAI).
 */
double ALLOMETRY::laiFromShootBiomassAreaSla(UTILS utils, double shootBiomass, double area, double sla)
{
   if (area <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (coveredArea) in function 'laiFromShootBiomassAreaSla'.");
   }
   return (shootBiomass * sla / area);
}

/**
 * @brief Calculates the ground area covered by the plant based on the plant width.
 *
 * @param width The diameter (or width) of the plant (in cm).
 * @return The calculated ground area (in square cm).
 */
double ALLOMETRY::areaFromWidth(double width)
{
   return ((PI / 4.0) * width * width);
}

/**
 * @brief Calculates the plant height based on shoot biomass, plant width, and shoot form factor.
 *
 * This function computes the height of a plant using the shoot biomass,
 * the width of the plant, and a form factor that accounts for biomass fraction in the cylindric shoot.
 *
 * @param biomass The shoot biomass of the plant (in g).
 * @param width The diameter (or width) of the plant (in cm).
 * @param form The shoot form factor, which represents how much biomass is contained in the cylindric shape of the plant (g per cubic cm).
 * @return The calculated height of the plant (in cm).
 */
double ALLOMETRY::heightFromShootBiomassWidthShootCorrection(UTILS utils, double shootBiomass, double width, double shootCorrectionFactor)
{
   if (width <= 0.0 || shootCorrectionFactor <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (width or shootCorrectionFactor) in function 'heightFromShootBiomassWidthCorrectionFactor'.");
   }
   return ((shootBiomass / areaFromWidth(width)) / shootCorrectionFactor);
}

/**
 * @brief Calculates the plant height based on plant width and height-width ratio.
 *
 * This function computes the height of a plant using its width and a
 * height-width ratio.
 *
 * @param width The width of the plant (in cm).
 * @param hwr The height-width ratio of the plant (in cm per cm).
 * @return The calculated height of the plant (in cm).
 */
double ALLOMETRY::heightFromWidthByRatio(double width, double heightWidthRatio)
{
   return (width * heightWidthRatio);
}

/**
 * @brief Calculates the plant width based on plant height and height-width ratio.
 *
 * This function computes the width of a plant using its height and a
 * height-width ratio.
 *
 * @param height The height of the plant (in cm).
 * @param hwr The height-width ratio of the plant (in cm per cm).
 * @return The calculated width of the plant (in cm).
 */
double ALLOMETRY::widthFromHeightByRatio(UTILS utils, double height, double heightWidthRatio)
{
   if (heightWidthRatio <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (heightWidthRatio) in function 'widthFromHeightByRatio'.");
   }
   return (height / heightWidthRatio);
}

/**
 * @brief Calculates the plant height based on shoot biomass, height-width ratio, and shoot form factor.
 *
 * This function computes the height of a plant using its shoot biomass,
 * the height-width ratio, and the shoot form factor.
 *
 * @param biomass The shoot biomass of the plant (in g).
 * @param hwr The height-width ratio of the plant (in cm per cm).
 * @param form The shoot form factor of the plant (in g per cubic cm).
 * @return The calculated height of the plant (in cm).
 */
double ALLOMETRY::heightFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor)
{
   if (shootCorrectionFactor <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (shootCorrectionFactor) in function 'heightFromShootBiomassByRatioAndShootCorrection'.");
   }
   return std::pow(shootBiomass * (4.0 / PI) * (heightWidthRatio * heightWidthRatio) / shootCorrectionFactor, 1.0 / 3.0);
}

/**
 * @brief Calculates the plant width based on shoot biomass, height-width ratio, and shoot form factor.
 *
 * This function computes the width of a plant using its shoot biomass,
 * the height-width ratio, and the shoot form factor.
 *
 * @param biomass The shoot biomass of the plant (in g).
 * @param hwr The height-width ratio of the plant (in cm per cm).
 * @param form The shoot form factor of the plant (in g per cubic cm).
 * @return The calculated width of the plant (in g).
 */
double ALLOMETRY::widthFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor)
{
   if (heightWidthRatio <= 0.0 || shootCorrectionFactor <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (heightWidthRatio or shootCorrectionFactor) in function 'widthFromShootBiomassByRatioAndShootCorrection'.");
   }

   double calcPart1 = ((shootBiomass * (4.0 / PI)) / heightWidthRatio);
   double calcPart2 = calcPart1 / shootCorrectionFactor;
   double calcPart3 = std::pow(calcPart2, 1.0 / 3.0);
   return (calcPart3);
}

/**
 * @brief Calculates the shoot biomass based on plant height, plant width, and shoot form factor.
 *
 * This function computes the shoot biomass of a plant using its height,
 * width, and shoot form factor.
 *
 * @param height The height of the plant (in cm).
 * @param width The width of the plant (in cm).
 * @param form The shoot form factor of the plant (in g per cubic cm).
 * @return The calculated shoot biomass of the plant (in g).
 */
double ALLOMETRY::shootBiomassFromHeightWidthShootCorrection(double height, double width, double shootCorrectionFactor)
{
   return areaFromWidth(width) * height * shootCorrectionFactor;
}

double ALLOMETRY::heightFromPlantBiomassShootCorrectionAndByRatios(UTILS utils, double plantBiomass, double heightWidthRatio, double shootCorrectionFactor, double shootRootRatio)
{

   if (shootRootRatio <= 0.0 || shootCorrectionFactor <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (heightWidthRatio or shootCorrectionFactor) in function 'heightFromPlantBiomassShootCorrectionAndByRatios'.");
   }
   double calcPart1 = (4.0 / PI) * plantBiomass * std::pow(heightWidthRatio, 2.0) * (1.0 / shootCorrectionFactor) * (1.0 / (1.0 + (1.0 / shootRootRatio)));
   double calcPart2 = pow(calcPart1, 1.0 / 3.0);
   return (calcPart2);
}

double ALLOMETRY::rootBiomassFromShootBiomass(UTILS utils, double shootBiomass, double shootRootRatio)
{
   if (shootRootRatio <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (shootRootRatio) in function 'rootBiomassFromShootBiomass'.");
   }
   return (shootBiomass / shootRootRatio);
}

double ALLOMETRY::rootDepthFromRootBiomassParametersRatioAndShootCorrection(UTILS utils, double rootBiomass, double parameterIntercept, double parameterExponent, double shootRootRatio, double shootCorrectionFactor)
{
   if (shootCorrectionFactor <= 0.0)
   {
      utils.handleError("Error (allometry): division by zero (shootCorrectionFactor) in function 'rootDepthFromRootBiomassParametersRatioAndShootCorrection'.");
   }
   double calcPart1 = std::pow((shootRootRatio / shootCorrectionFactor), parameterExponent);
   double calcPart2 = std::pow(rootBiomass, parameterExponent);
   return (parameterIntercept * calcPart1 * calcPart2);
}
