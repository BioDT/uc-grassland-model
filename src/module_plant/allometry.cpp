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
double ALLOMETRY::laiFromBiomassAreaSla(double biomass, double area, double sla)
{
   return biomass * sla / area;
}

/**
 * @brief Calculates the ground area covered by the plant based on the plant width.
 *
 * @param width The diameter (or width) of the plant (in cm).
 * @return The calculated ground area (in square cm).
 */
double ALLOMETRY::areaFromWidth(double width)
{
   return PI / 4.0 * width * width;
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
double ALLOMETRY::heightFromBiomassWidthForm(double biomass, double width, double form)
{
   return biomass / areaFromWidth(width) / form;
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
double ALLOMETRY::heightFromWidthHwr(double width, double hwr)
{
   return width * hwr;
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
double ALLOMETRY::widthFromHeightHwr(double height, double hwr)
{
   return height / hwr;
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
double ALLOMETRY::heightFromBiomassHwrForm(double biomass, double hwr, double form)
{
   return pow(biomass * 4.0 / PI * (hwr * hwr) / form, 1.0 / 3.0);
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
double ALLOMETRY::widthFromBiomassHwrForm(double biomass, double hwr, double form)
{
   return pow(biomass * 4.0 / PI / hwr / form, 1.0 / 3.0);
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
double ALLOMETRY::biomassFromHeightWidthForm(double height, double width, double form)
{
   return areaFromWidth(width) * height * form;
}
