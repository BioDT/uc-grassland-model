/**
 * @file allometry.h
 * @brief Declares the ALLOMETRY class, which provides allometric conversion
 *        functions for plant geometry and biomass.
 *
 * All functions are pure calculations — no state is stored between calls.
 * Plants are modelled as upright cylinders whose base area equals
 * @f$\frac{\pi}{4} w^2@f$ (circle with diameter @f$w@f$). The
 * `shootCorrectionFactor` (g ODM cm⁻³) represents the effective biomass
 * density of that cylinder and links biomass to geometry.
 *
 * Consistent units throughout:
 * - Length / height / depth: **cm**
 * - Area: **cm²**
 * - Biomass: **g ODM**
 * - Specific leaf area: **cm² g⁻¹ ODM**
 * - Ratios: **dimensionless**
 */
#pragma once
#include "../module_init/constants.h"
#include "../utils/utils.h"
#include <cmath>

/**
 * @class ALLOMETRY
 * @brief Stateless helper providing allometric equations that relate plant
 *        geometry variables (height, width, covered area, rooting depth, LAI)
 *        to biomass quantities.
 *
 * The class is instantiated once and passed by value to any module that needs
 * geometric conversions. All methods either compute a single output from given
 * inputs or raise an error via `utils.handleError()` for invalid (zero/negative)
 * denominators.
 */
class ALLOMETRY
{
public:
    ALLOMETRY();
    ~ALLOMETRY();

    /**
     * @brief Calculates the Leaf Area Index from shoot biomass, covered area, and
     *        specific leaf area.
     */
    double laiFromShootBiomassAreaSla(UTILS utils, double shootBiomass, double area, double sla);

    /**
     * @brief Calculates covered ground area from canopy diameter.
     */
    double areaFromWidth(double width);

    /**
     * @brief Calculates plant height from shoot biomass, canopy width, and shoot
     *        correction factor.
     */
    double heightFromShootBiomassWidthShootCorrection(UTILS utils, double shootBiomass, double width, double shootCorrectionFactor);

    /**
     * @brief Calculates plant height from canopy width and the height-to-width ratio.
     */
    double heightFromWidthByRatio(double width, double heightWidthRatio);

    /**
     * @brief Calculates canopy width from plant height and the height-to-width ratio.
     */
    double widthFromHeightByRatio(UTILS utils, double height, double heightWidthRatio);

    /**
     * @brief Calculates plant height from shoot biomass, height-to-width ratio, and
     *        shoot correction factor (normal proportional growth).
     */
    double heightFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor);

    /**
     * @brief Calculates canopy width from shoot biomass, height-to-width ratio, and
     *        shoot correction factor (normal proportional growth).
     */
    double widthFromShootBiomassByRatioAndShootCorrection(UTILS utils, double shootBiomass, double heightWidthRatio, double shootCorrectionFactor);

    /**
     * @brief Calculates shoot biomass from plant height, canopy width, and shoot
     *        correction factor.
     */
    double shootBiomassFromHeightWidthShootCorrection(double height, double width, double shootCorrectionFactor);

    /**
     * @brief Calculates plant height from total plant biomass, allometric ratios, and
     *        shoot correction factor (used during initialisation).
     */
    double heightFromPlantBiomassShootCorrectionAndByRatios(UTILS utils, double plantBiomass, double heightWidthRatio, double shootCorrectionFactor, double shootRootRatio);

    /**
     * @brief Calculates root biomass from shoot biomass and the shoot-root ratio.
     */
    double rootBiomassFromShootBiomass(UTILS utils, double shootBiomass, double shootRootRatio);

    /**
     * @brief Calculates root-zone depth from root biomass using an allometric
     *        power-law equation.
     */
    double rootDepthFromRootBiomassParametersRatioAndShootCorrection(UTILS utils, double rootBiomass, double parameterIntercept, double parameterExponent, double shootRootRatio, double shootCorrectionFactor);

    /**
     * @brief Determines the number of soil layers reached by plant roots.
     */
    double calculateNumberOfRootingSoillayer(std::vector<double> soilLayerWidth, double plantRootingDepth);
};