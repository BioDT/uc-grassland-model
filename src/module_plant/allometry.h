#pragma once
#include "../module_init/constants.h"
#include "../utils/utils.h"
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

   double laiFromShootBiomassAreaSla(UTILS utils, double shootBiomass, double area, double sla);
   double areaFromWidth(double width);
   double heightFromShootBiomassWidthShootCorrection(UTILS utils, double shootBiomass, double width, double shootCorrectionFactor);
   double heightFromWidthByRatio(double width, double heightWidthRatio);
   double widthFromHeightByRatio(UTILS utils, double height, double heightWidthRatio);
   double heightFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor);
   double widthFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor);
   double shootBiomassFromHeightWidthShootCorrection(double height, double width, double shootCorrectionFactor);
   double heightFromPlantBiomassShootCorrectionAndByRatios(UTILS utils, double plantBiomass, double heightWidthRatio, double shootCorrectionFactor, double shootRootRatio);
   double rootBiomassFromShootBiomass(UTILS utils, double shootBiomass, double shootRootRatio);
   double rootDepthFromRootBiomassParametersRatioAndShootCorrection(UTILS utils, double rootBiomass, double parameterIntercept, double parameterExponent, double shootRootRatio, double shootCorrectionFactor);
};