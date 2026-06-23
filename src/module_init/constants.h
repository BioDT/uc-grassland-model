/**
 * @file constants.h
 * @brief Global physical, chemical, and numerical constants used throughout the model.
 *
 * All constants are defined as file-scope `const` values and are included
 * wherever needed via this header. Units are noted inline for each entry.
 */
#pragma once

/** @brief General numerical tolerance used for logical comparisons (dimensionless). */
const double TOLERANCE = 0.01;

/** @brief Mathematical constant π (dimensionless). */
const double PI = 3.14159265358979323846;

/**
 * @brief Carbon fraction of organic dry matter (g C g⁻¹ ODM). Used to convert biomass expressed as organic dry matter (ODM) to carbon mass.
 */
const double CARBON_CONTENT_ODM = 0.44;

/**
 * @brief Mass of organic dry matter contained in 1 g CO₂ (g ODM g⁻¹ CO₂). Used to convert photosynthetic CO₂ uptake to ODM production.
 */
const double CO2_CONVERSION_TO_ODM = 0.63;

/**
 * @brief Molar mass conversion factor from µmol CO₂ to g CO₂ (g µmol⁻¹). Equals the molar mass of CO₂ (44 g mol⁻¹) expressed in g per µmol.
 */
const double MOLAR_MASS_OF_CO2 = 44E-6;

/**
 * @brief Fraction of incident radiation transmitted through the canopy base (dimensionless).
 * Used in the Beer-Lambert light extinction calculation as the minimum
 * light level transmitted even at high LAI.
 */
const double LIGHT_TRANSMISSION_COEFFICIENT = 0.1;

/**
 * @brief Width of a single vertical canopy height layer (cm).
 * The canopy is discretised into horizontal layers of this thickness
 * for community-shading photosynthesis calculations.
 */
const int HEIGHT_LAYER_WIDTH = 1;

/**
 * @brief Maximum number of canopy height layers (dimensionless).
 * Defines the upper boundary of the height-layer array.
 * With HEIGHT_LAYER_WIDTH = 1 cm, this corresponds to a maximum
 * vegetation height of 300 cm (3 m).
 */
const int MAXIMUM_HEIGHT_LAYER = 300;

/**
 * @brief Number of seconds in one day (s d⁻¹).
 * Used when converting per-second rates to daily totals.
 */
const int DAY_IN_SECONDS = 86400;

/**
 * @brief Horizontal area of the simulation plot (cm²).
 * All per-plant fluxes are scaled to this area when computing
 * community-level quantities. Equals 1 m².
 */
const double SIMULATION_AREA = 10000;

/**
 * @brief Strict numerical tolerance for floating-point near-zero comparisons (dimensionless).
 * Used where a tighter bound than TOLERANCE is required, e.g. to guard
 * against division-by-zero in geometric or light-extinction calculations.
 */
const double NUMERIC_TOLERANCE = 1e-12;
