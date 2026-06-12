#include "allometry.h"

ALLOMETRY::ALLOMETRY() {};
ALLOMETRY::~ALLOMETRY() {};

/**
 * @brief Calculates the Leaf Area Index (LAI) from shoot biomass, covered area,
 *        and specific leaf area (SLA).
 *
 * @f[ \text{LAI} = \frac{B_{\text{shoot}} \cdot \text{SLA}}{A} @f]
 *
 * @param utils        Utility object; raises an error if `area` ≤ 0.
 * @param shootBiomass Shoot biomass of the plant (g ODM).
 * @param area         Ground area covered by the plant (cm²); must be > 0.
 * @param sla          Specific leaf area (cm² g⁻¹ ODM).
 * @return Leaf Area Index (m² leaf m⁻² ground, dimensionless when area and
 *         biomass use consistent units).
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
 * @brief Calculates the ground area covered by a plant from its canopy diameter.
 *
 * Models the plant canopy as a circle:
 * @f[ A = \frac{\pi}{4} \cdot w^2 @f]
 *
 * @param width Canopy diameter (width) of the plant (cm).
 * @return Covered ground area (cm²).
 */
double ALLOMETRY::areaFromWidth(double width)
{
    return ((PI / 4.0) * width * width);
}

/**
 * @brief Calculates plant height from shoot biomass, canopy width, and shoot
 *        correction factor.
 *
 * Assumes the shoot volume approximates a cylinder of base area
 * @f$A = \frac{\pi}{4} w^2@f$ and height @f$h@f$, with biomass density
 * scaled by `shootCorrectionFactor`:
 * @f[ h = \frac{B_{\text{shoot}} / A}{f_{\text{shoot}}} @f]
 *
 * Used when the plant width is known but needs its height recalculated
 * after biomass changes (e.g. following mowing).
 *
 * @param utils               Utility object; raises an error if `width` or
 *                            `shootCorrectionFactor` ≤ 0.
 * @param shootBiomass        Shoot biomass (g ODM).
 * @param width               Canopy width / diameter (cm); must be > 0.
 * @param shootCorrectionFactor Biomass density of the cylindrical shoot volume
 *                            (g ODM cm⁻³); must be > 0.
 * @return Plant height (cm).
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
 * @brief Calculates plant height from canopy width and the height-to-width ratio.
 *
 * @f[ h = w \cdot r_{hw} @f]
 *
 * @param width           Canopy width / diameter (cm).
 * @param heightWidthRatio Height-to-width allometric ratio (cm cm⁻¹, dimensionless).
 * @return Plant height (cm).
 */
double ALLOMETRY::heightFromWidthByRatio(double width, double heightWidthRatio)
{
    return (width * heightWidthRatio);
}

/**
 * @brief Calculates canopy width from plant height and the height-to-width ratio.
 *
 * @f[ w = \frac{h}{r_{hw}} @f]
 *
 * @param utils           Utility object; raises an error if `heightWidthRatio` ≤ 0.
 * @param height          Plant height (cm).
 * @param heightWidthRatio Height-to-width allometric ratio (cm cm⁻¹); must be > 0.
 * @return Canopy width / diameter (cm).
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
 * @brief Calculates plant height from shoot biomass, height-to-width ratio, and
 *        shoot correction factor.
 *
 * Derived by combining the cylindrical shoot-volume model with the allometric
 * height-width constraint:
 * @f[
 *   h = \left( B_{\text{shoot}} \cdot \frac{4}{\pi}
 *       \cdot \frac{r_{hw}^2}{f_{\text{shoot}}} \right)^{1/3}
 * @f]
 *
 * Used for normal proportional growth where both height and width increase
 * simultaneously.
 *
 * @param utils               Utility object; raises an error if
 *                            `shootCorrectionFactor` ≤ 0.
 * @param shootBiomass        Shoot biomass (g ODM).
 * @param heightWidthRatio    Height-to-width allometric ratio (cm cm⁻¹).
 * @param shootCorrectionFactor Biomass density of the cylindrical shoot volume
 *                            (g ODM cm⁻³); must be > 0.
 * @return Plant height (cm).
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
 * @brief Calculates canopy width from shoot biomass, height-to-width ratio, and
 *        shoot correction factor.
 *
 * Inverse of `heightFromShootBiomassByRatioAndShootCorrection()` with respect to
 * width:
 * @f[
 *   w = \left( \frac{B_{\text{shoot}} \cdot 4/\pi}
 *              {r_{hw} \cdot f_{\text{shoot}}} \right)^{1/3}
 * @f]
 *
 * @param utils               Utility object; raises an error if `heightWidthRatio`
 *                            or `shootCorrectionFactor` ≤ 0.
 * @param shootBiomass        Shoot biomass (g ODM).
 * @param heightWidthRatio    Height-to-width allometric ratio (cm cm⁻¹); must be > 0.
 * @param shootCorrectionFactor Biomass density of the cylindrical shoot volume
 *                            (g ODM cm⁻³); must be > 0.
 * @return Canopy width / diameter (cm).
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
 * @brief Calculates shoot biomass from plant height, canopy width, and shoot
 *        correction factor.
 *
 * Inverse of `heightFromShootBiomassWidthShootCorrection()`:
 * @f[ B_{\text{shoot}} = A(w) \cdot h \cdot f_{\text{shoot}} @f]
 * where @f$A(w) = \frac{\pi}{4} w^2@f$.
 *
 * @param height              Plant height (cm).
 * @param width               Canopy width / diameter (cm).
 * @param shootCorrectionFactor Biomass density of the cylindrical shoot volume
 *                            (g ODM cm⁻³).
 * @return Shoot biomass (g ODM).
 */
double ALLOMETRY::shootBiomassFromHeightWidthShootCorrection(double height, double width, double shootCorrectionFactor)
{
    return areaFromWidth(width) * height * shootCorrectionFactor;
}

/**
 * @brief Calculates plant height from total plant biomass, allometric ratios, and
 *        shoot correction factor.
 *
 * Accounts for the shoot-root split when only total biomass is known:
 * @f[
 *   h = \left( \frac{4}{\pi} \cdot B_{\text{plant}} \cdot r_{hw}^2
 *       \cdot \frac{1}{f_{\text{shoot}}}
 *       \cdot \frac{1}{1 + 1/r_{\text{sr}}} \right)^{1/3}
 * @f]
 * where @f$r_{\text{sr}}@f$ is the shoot-root ratio.
 *
 * Used during plant initialisation when individual biomass pools are not yet
 * available.
 *
 * @param utils               Utility object; raises an error if `shootRootRatio`
 *                            or `shootCorrectionFactor` ≤ 0.
 * @param plantBiomass        Total plant biomass (shoot + root, g ODM).
 * @param heightWidthRatio    Height-to-width allometric ratio (cm cm⁻¹).
 * @param shootCorrectionFactor Biomass density of the cylindrical shoot volume
 *                            (g ODM cm⁻³); must be > 0.
 * @param shootRootRatio      Target shoot-to-root biomass ratio (g g⁻¹); must be > 0.
 * @return Plant height (cm).
 */
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

/**
 * @brief Calculates root biomass from shoot biomass and the shoot-root ratio.
 *
 * @f[ B_{\text{root}} = \frac{B_{\text{shoot}}}{r_{\text{sr}}} @f]
 *
 * @param utils          Utility object; raises an error if `shootRootRatio` ≤ 0.
 * @param shootBiomass   Shoot biomass (g ODM).
 * @param shootRootRatio Target shoot-to-root biomass ratio (g g⁻¹); must be > 0.
 * @return Root biomass (g ODM).
 */
double ALLOMETRY::rootBiomassFromShootBiomass(UTILS utils, double shootBiomass, double shootRootRatio)
{
    if (shootRootRatio <= 0.0)
    {
        utils.handleError("Error (allometry): division by zero (shootRootRatio) in function 'rootBiomassFromShootBiomass'.");
    }
    return (shootBiomass / shootRootRatio);
}

/**
 * @brief Calculates root-zone depth from root biomass using an allometric
 *        power-law equation.
 *
 * The relationship is:
 * @f[
 *   d_{\text{root}} = a \cdot \left(\frac{r_{\text{sr}}}{f_{\text{shoot}}}
 *   \right)^{b} \cdot B_{\text{root}}^{b}
 * @f]
 * where @f$a@f$ = `parameterIntercept`, @f$b@f$ = `parameterExponent`,
 * @f$r_{\text{sr}}@f$ = shoot-root ratio, and @f$f_{\text{shoot}}@f$ =
 * shoot correction factor.
 *
 * @param utils               Utility object; raises an error if
 *                            `shootCorrectionFactor` ≤ 0.
 * @param rootBiomass         Root biomass (g ODM).
 * @param parameterIntercept  Intercept of the root-depth allometric equation (cm).
 * @param parameterExponent   Exponent of the root-depth allometric equation
 *                            (dimensionless).
 * @param shootRootRatio      Shoot-to-root biomass ratio (g g⁻¹).
 * @param shootCorrectionFactor Biomass density factor (g ODM cm⁻³); must be > 0.
 * @return Root-zone depth (cm).
 */
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

/**
 * @brief Determines the number of soil layers reached by plant roots.
 *
 * Iterates through soil layers from the surface downward, accumulating depth
 * until the cumulative depth equals or exceeds `plantRootingDepth`. Returns
 * at least 1 (the topmost layer is always included even for shallow-rooted
 * plants).
 *
 * @param soilLayerWidth    Vector of individual soil layer widths (cm), ordered
 *                          from surface to deepest layer.
 * @param plantRootingDepth Root-zone depth of the plant (cm).
 * @return Number of soil layers reached by the roots (integer, ≥ 1).
 */
double ALLOMETRY::calculateNumberOfRootingSoillayer(std::vector<double> soilLayerWidth, double plantRootingDepth)
{
    double cumulativeSoilDepth = 0.0;
    int numberOfRootingSoilLayers = 1;

    for (int soilLayer = 0; soilLayer < soilLayerWidth.size(); soilLayer++)
    {
        cumulativeSoilDepth += soilLayerWidth.at(soilLayer);
        if (plantRootingDepth > cumulativeSoilDepth)
        {
            numberOfRootingSoilLayers++;
        }
    }

    return (numberOfRootingSoilLayers);
}
