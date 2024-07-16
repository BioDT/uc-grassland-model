#include "allometry.h"

ALLOMETRY::ALLOMETRY(){};
ALLOMETRY::~ALLOMETRY(){};

/* leaf area index out of shoot biomass, ground area, specific leaf area (sla) */
double ALLOMETRY::laiFromBiomassAreaSla(double biomass, double area, double sla)
{
    return biomass * sla / area;
}

/* ground area out of plant width (i.e. area of circle from diameter) */
double ALLOMETRY::areaFromWidth(double width)
{
    return PI / 4.0 * width * width;
}

/* plant height out of shoot biomass, plant width, shoot form factor */
double ALLOMETRY::heightFromBiomassWidthForm(double biomass, double width, double form)
{
    return biomass / areaFromWidth(width) / form;
}

/* plant height out of plant width, height-width-ratio */
double ALLOMETRY::heightFromWidthHwr(double width, double hwr)
{
    return width * hwr;
}

/* plant width out of plant height, height-width-ratio */
double ALLOMETRY::widthFromHeightHwr(double height, double hwr)
{
    return height / hwr;
}

/* plant height out of shoot biomass, height-width-ratio, shoot form factor */
double ALLOMETRY::heightFromBiomassHwrForm(double biomass, double hwr, double form)
{
    return pow(biomass * 4.0 / PI * (hwr * hwr) / form, 1.0 / 3.0);
}

/* plant width out of shoot biomass, height-width-ratio, shoot form factor */
double ALLOMETRY::widthFromBiomassHwrForm(double biomass, double hwr, double form)
{
    return pow(biomass * 4.0 / PI / hwr / form, 1.0 / 3.0);
}

/* shoot biomass out of plant height, plant width, shoot form factor */
double ALLOMETRY::biomassFromHeightWidthForm(double height, double width, double form)
{
    return areaFromWidth(width) * height * form;
}
