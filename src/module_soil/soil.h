/**
 * @file soil.h
 * @brief Declares the SOIL class, which stores all soil state variables and
 *        implements the soil water, carbon, and nitrogen dynamics.
 *
 * The class covers four functional areas:
 * 1. **Soil texture and hydraulic input parameters** — read from the soil file
 *    by INPUT and never changed during the simulation.
 * 2. **Coupling interfaces** — vectors and scalars used to exchange soil state
 *    and fluxes with external models (BODIUM) or a second GRASSMIND instance
 *    in self-coupling mode.
 * 3. **State variables and fluxes** — water content, snow, C/N pool contents,
 *    inter-pool fluxes, and decomposition tracking variables.  All flux
 *    variables are reset each time step by
 *    INIT::resetSoilResourceProcessAndFluxVariables().
 * 4. **Methods** — soil water dynamics (snow, interception, percolation,
 *    evaporation, plant uptake) and C/N decomposition (litter splitting,
 *    Century-based pool model, mineralisation, volatilisation).
 *
 * @cite Carbon and nitrogen dynamics based on the CENTURY 4.0 model.
 * @cite Soil water evaporation based on the BOWET model.
 * @cite Canopy interception and plant water limitation based on the FORMIND forest model.
 */
#pragma once
#include "../module_init/constants.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"
#include <vector>
#include <iostream>

/**
 * @class SOIL
 * @brief Stores all soil state and provides soil water and C/N dynamics.
 *
 * Units used throughout:
 * - Water content / fluxes: **mm** (equivalent to kg m⁻²)
 * - Carbon pool contents: **g C m⁻²**
 * - Nitrogen pool contents / fluxes: **g N m⁻²** (or g N m⁻² d⁻¹ for daily fluxes)
 * - Soil hydraulic properties: consistent with input files (fraction of
 *   saturation or mm per layer, see soil input file format)
 */
class SOIL
{
public:
    SOIL();
    ~SOIL();

    // =========================================================================
    // Soil texture and hydraulic input parameters
    // (read from the soil file; constant during the simulation)
    // =========================================================================

    /** @brief Silt mass fraction of the bulk soil (dimensionless, 0–1). */
    double siltContent;

    /** @brief Sand mass fraction of the bulk soil (dimensionless, 0–1). */
    double sandContent;

    /** @brief Clay mass fraction of the bulk soil (dimensionless, 0–1). */
    double clayContent;

    /**
     * @brief Permanent wilting point per soil layer (mm or volumetric fraction,
     *        consistent with the soil input file).
     */
    std::vector<double> permanentWiltingPoint;

    /**
     * @brief Field capacity per soil layer (same units as `permanentWiltingPoint`).
     */
    std::vector<double> fieldCapacity;

    /**
     * @brief Total porosity per soil layer (same units as `permanentWiltingPoint`).
     */
    std::vector<double> porosity;

    /**
     * @brief Saturated hydraulic conductivity per soil layer (mm d⁻¹).
     */
    std::vector<double> saturatedHydraulicConductivity;

    // =========================================================================
    // Coupling interface — self-coupling and BODIUM external module
    // =========================================================================

    /**
     * @brief Per-layer permanent wilting point exposed to the self-coupling
     *        interface (read by the get-mode instance).
     */
    std::vector<double> couplingInterface_permanentWiltingPointPerSoilLayer;

    /**
     * @brief Per-layer field capacity exposed to the self-coupling interface.
     */
    std::vector<double> couplingInterface_fieldCapacityPerSoilLayer;

    /**
     * @brief Per-layer porosity exposed to the self-coupling interface.
     */
    std::vector<double> couplingInterface_porosityPerSoilLayer;

    /**
     * @brief Per-layer soil water content exposed to the self-coupling interface.
     */
    std::vector<double> couplingInterface_waterContentPerSoilLayer;

    /**
     * @brief Per-layer water-uptake reduction factors provided by the coupling
     *        interface for PWP-based limitation.
     */
    std::vector<double> couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer;

    /**
     * @brief Per-layer mineral nitrogen content exposed to the self-coupling
     *        interface (read by the get-mode instance).
     */
    std::vector<double> couplingInterface_nitrogenContentPerSoilLayer;

    /**
     * @brief Per-layer soil water potential provided by BODIUM (Pa).
     *        Replaces the self-coupling PWP/FC/porosity/water-content interfaces.
     */
    std::vector<double> couplingInterface_soilWaterPotentialPerSoilLayer;

    /**
     * @brief Per-layer pore-water NH₄ concentration provided by BODIUM
     *        (kg N m⁻³ or equivalent). Replaces `couplingInterface_nitrogenContentPerSoilLayer`.
     */
    std::vector<double> couplingInterface_soilWaterNh4Concentration;

    /**
     * @brief Per-layer pore-water NO₃ concentration provided by BODIUM
     *        (same units as `couplingInterface_soilWaterNh4Concentration`).
     */
    std::vector<double> couplingInterface_soilWaterNo3Concentration;

    /**
     * @brief Per-layer plant NH₄ uptake exported to BODIUM (kg N m⁻² d⁻¹).
     */
    std::vector<double> couplingInterface_plantNh4UptakePerSoilLayer;

    /**
     * @brief Per-layer plant NO₃ uptake exported to BODIUM (kg N m⁻² d⁻¹).
     */
    std::vector<double> couplingInterface_plantNo3UptakePerSoilLayer;

    /**
     * @brief Per-layer plant water uptake (transpiration) exported to BODIUM (mm d⁻¹).
     */
    std::vector<double> couplingInterface_plantSoilWaterUptakePerLayer;

    // =========================================================================
    // Management inputs (set by MANAGEMENT each time step)
    // =========================================================================

    /**
     * @brief Mineral nitrogen added to the topsoil layer by fertilisation today
     *        (g N m⁻² d⁻¹). Reset to 0 each time step.
     */
    double addedMineralNitrogenToSoilByFertilization;

    /**
     * @brief Water added to the topsoil layer by irrigation today (mm d⁻¹).
     *        Reset to 0 each time step.
     */
    double addedWaterToSoilByIrrigation;

    // =========================================================================
    // Snow and water state variables / fluxes
    // =========================================================================

    /** @brief Solid (frozen) snow water equivalent on the soil surface (mm). */
    double solidSnowContent;

    /** @brief Liquid water retained within the snowpack (mm). */
    double liquidSnowContent;

    /**
     * @brief Water content per soil layer plus one groundwater storage layer (mm).
     */
    std::vector<double> waterContent_soilWaterPoolPerSoilLayer;

    /**
     * @brief Downward water flux leaving the bottom of each soil layer (mm d⁻¹).
     */
    std::vector<double> soilWaterFluxDownwardsOutOfSoilLayer;

    /** @brief Total evaporation (snow sublimation + bare-soil evaporation) today (mm d⁻¹). */
    double evaporation;

    /** @brief Surface run-off today (mm d⁻¹). */
    double surfaceRunOff;

    /** @brief Subsurface / groundwater run-off today (mm d⁻¹). */
    double soilRunOff;

    /** @brief Canopy interception today (mm d⁻¹). */
    double interception;

    /**
     * @brief Net surface water input after interception and snow processes,
     *        exported to BODIUM (mm d⁻¹).
     */
    double couplingInterface_soilWaterSurfaceInput;

    /**
     * @brief Remaining PET after subtracting interception and snow sublimation,
     *        exported to BODIUM (mm d⁻¹).
     */
    double couplingInterface_potentialEvapotranspirationReducedByInterceptionSublimation;

    // =========================================================================
    // Carbon pool contents (g C m⁻²)
    // Input litter pools — filled by transferDyingPlantPartsToLitterPools()
    // =========================================================================

    /** @brief Carbon in the surface green (fresh) litter input pool (g C m⁻²). */
    double carbonContent_surfaceGreenLitterPool;

    /** @brief Carbon in the surface brown (senescent) litter input pool (g C m⁻²). */
    double carbonContent_surfaceBrownLitterPool;

    /** @brief Carbon in the soil root litter input pool (g C m⁻²). */
    double carbonContent_soilRootLitterPool;

    /** @brief Carbon in the soil seed litter input pool (g C m⁻²). */
    double carbonContent_soilSeedLitterPool;

    /** @brief Carbon in the soil metabolic litter pool (g C m⁻²). */
    double carbonContent_soilMetabolicLitterPool;

    /** @brief Carbon in the surface metabolic litter pool (g C m⁻²). */
    double carbonContent_surfaceMetabolicLitterPool;

    /** @brief Carbon in the soil structural litter pool (g C m⁻²). */
    double carbonContent_soilStructuralLitterPool;

    /** @brief Carbon in the surface structural litter pool (g C m⁻²). */
    double carbonContent_surfaceStructuralLitterPool;

    /** @brief Carbon in the passive (recalcitrant humus) soil pool (g C m⁻²). */
    double carbonContent_soilPassivePool;

    /** @brief Carbon in the slow (stable humus) soil pool (g C m⁻²). */
    double carbonContent_soilSlowPool;

    /** @brief Carbon in the active (labile) soil pool (g C m⁻²). */
    double carbonContent_soilActivePool;

    /** @brief Carbon in the soil microbial biomass pool (g C m⁻²). */
    double carbonContent_soilMicrobesPool;

    // =========================================================================
    // Nitrogen pool contents (g N m⁻²)
    // =========================================================================

    /** @brief Nitrogen in the surface green litter input pool (g N m⁻²). */
    double nitrogenContent_surfaceGreenLitterPool;

    /** @brief Nitrogen in the surface brown litter input pool (g N m⁻²). */
    double nitrogenContent_surfaceBrownLitterPool;

    /** @brief Nitrogen in the soil root litter input pool (g N m⁻²). */
    double nitrogenContent_soilRootLitterPool;

    /** @brief Nitrogen in the soil seed litter input pool (g N m⁻²). */
    double nitrogenContent_soilSeedLitterPool;

    /** @brief Nitrogen in the soil metabolic litter pool (g N m⁻²). */
    double nitrogenContent_soilMetabolicLitterPool;

    /** @brief Nitrogen in the surface metabolic litter pool (g N m⁻²). */
    double nitrogenContent_surfaceMetabolicLitterPool;

    /** @brief Nitrogen in the soil structural litter pool (g N m⁻²). */
    double nitrogenContent_soilStructuralLitterPool;

    /** @brief Nitrogen in the surface structural litter pool (g N m⁻²). */
    double nitrogenContent_surfaceStructuralLitterPool;

    /** @brief Nitrogen in the passive soil pool (g N m⁻²). */
    double nitrogenContent_soilPassivePool;

    /** @brief Nitrogen in the slow soil pool (g N m⁻²). */
    double nitrogenContent_soilSlowPool;

    /** @brief Nitrogen in the active soil pool (g N m⁻²). */
    double nitrogenContent_soilActivePool;

    /** @brief Nitrogen in the soil microbial biomass pool (g N m⁻²). */
    double nitrogenContent_soilMicrobesPool;

    /**
     * @brief Mineral (plant-available) nitrogen per soil layer (g N m⁻²).
     */
    std::vector<double> nitrogenContent_soilMineralPoolPerSoilLayer;

    // =========================================================================
    // Coupling interface — litter and exudate fluxes to BODIUM
    // =========================================================================

    /**
     * @brief Daily surface litter carbon flux exported to BODIUM
     *        (sum of shoot + seed litter, g C m⁻² d⁻¹).
     */
    double couplingInterface_surfaceLitterFluxCarbon;

    /**
     * @brief Daily surface litter nitrogen flux exported to BODIUM
     *        (sum of shoot + seed litter, g N m⁻² d⁻¹).
     */
    double couplingInterface_surfaceLitterFluxNitrogen;

    /**
     * @brief Per-layer daily root litter carbon flux exported to BODIUM
     *        (g C m⁻² d⁻¹). Sized to `parameter.numberOfSoilLayers`.
     */
    std::vector<double> couplingInterface_rootLitterFluxCarbon;

    /**
     * @brief Per-layer daily root litter nitrogen flux exported to BODIUM
     *        (g N m⁻² d⁻¹). Sized to `parameter.numberOfSoilLayers`.
     */
    std::vector<double> couplingInterface_rootLitterFluxNitrogen;

    /**
     * @brief Per-layer daily root exudate carbon flux exported to BODIUM
     *        (g C m⁻² d⁻¹). Sized to `parameter.numberOfSoilLayers`.
     */
    std::vector<double> couplingInterface_rootExudatesCarbon;

    /**
     * @brief Per-layer daily root exudate nitrogen flux exported to BODIUM
     *        (g N m⁻² d⁻¹). Sized to `parameter.numberOfSoilLayers`.
     */
    std::vector<double> couplingInterface_rootExudatesNitrogen;

    // =========================================================================
    // Decisive C/N ratios for decomposition
    // (computed once at init; updated by calculateDecisiveCarbonNitrogenRatiosForDecomposition)
    // =========================================================================

    /**
     * @brief Decisive C/N ratio for the transfer from soil microbial biomass to
     *        slow pool (dimensionless). Controls mineralisation vs. immobilisation.
     */
    double decisiveCNRatio_soilMicrobesPool_soilSlowPool;

    /** @brief Decisive C/N ratio for passive pool → active pool transfer. */
    double decisiveCNRatio_soilPassivePool_soilActivePool;

    /** @brief Decisive C/N ratio for slow pool → active pool transfer. */
    double decisiveCNRatio_soilSlowPool_soilActivePool;

    /** @brief Decisive C/N ratio for slow pool → passive pool transfer. */
    double decisiveCNRatio_soilSlowPool_soilPassivePool;

    /** @brief Decisive C/N ratio for active pool → slow pool transfer. */
    double decisiveCNRatio_soilActivePool_soilSlowPool;

    /** @brief Decisive C/N ratio for active pool → passive pool transfer. */
    double decisiveCNRatio_soilActivePool_soilPassivePool;

    /** @brief Decisive C/N ratio for soil metabolic litter → active pool transfer. */
    double decisiveCNRatio_soilMetabolicLitterPool_soilActivePool;

    /** @brief Decisive C/N ratio for surface metabolic litter → microbes pool transfer. */
    double decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool;

    /**
     * @brief Decisive C/N ratio for surface structural litter → microbes pool.
     *        Computed once at initialisation.
     */
    double decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool;

    /**
     * @brief Decisive C/N ratio for surface structural litter → slow pool.
     *        Computed once at initialisation.
     */
    double decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool;

    /**
     * @brief Decisive C/N ratio for soil structural litter → active pool.
     *        Computed once at initialisation.
     */
    double decisiveCNRatio_soilStructuralLitterPool_soilActivePool;

    /**
     * @brief Decisive C/N ratio for soil structural litter → slow pool.
     *        Computed once at initialisation.
     */
    double decisiveCNRatio_soilStructuralLitterPool_soilSlowPool;

    // =========================================================================
    // Lignin contents (g lignin m⁻²)
    // =========================================================================

    /** @brief Lignin content of the surface structural litter pool (g m⁻²). */
    double ligninContent_surfaceStructuralLitterPool;

    /** @brief Lignin content of the soil structural litter pool (g m⁻²). */
    double ligninContent_soilStructuralLitterPool;

    // =========================================================================
    // Carbon inter-pool fluxes (g C m⁻² d⁻¹) — reset each time step
    // =========================================================================

    /** @brief C flux from surface structural litter to slow pool (g C m⁻² d⁻¹). */
    double carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool;

    /** @brief C flux from surface structural litter to microbes pool (g C m⁻² d⁻¹). */
    double carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;

    /** @brief C flux from surface metabolic litter to microbes pool (g C m⁻² d⁻¹). */
    double carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;

    /** @brief C flux from soil structural litter to slow pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilStructuralLitterPool_to_soilSlowPool;

    /** @brief C flux from soil structural litter to active pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilStructuralLitterPool_to_soilActivePool;

    /** @brief C flux from soil metabolic litter to active pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilMetabolicLitterPool_to_soilActivePool;

    /**
     * @brief Combined C flux from active pool to both passive and slow pools
     *        (g C m⁻² d⁻¹). Used as aggregate for output.
     */
    double carbonFlux_soilActivePool_to_soilPassiveAndSlowPool;

    /**
     * @brief Combined C flux from slow pool to both passive and active pools
     *        (g C m⁻² d⁻¹). Used as aggregate for output.
     */
    double carbonFlux_soilSlowPool_to_soilPassiveAndActivePool;

    /** @brief C flux from microbes pool to slow pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilMicrobesPool_to_soilSlowPool;

    /** @brief C flux from slow pool to passive pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilSlowPool_to_soilPassivePool;

    /** @brief C flux from slow pool to active pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilSlowPool_to_soilActivePool;

    /** @brief C flux from active pool to passive pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilActivePool_to_soilPassivePool;

    /** @brief C flux from active pool to slow pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilActivePool_to_soilSlowPool;

    /** @brief C flux from passive pool to active pool (g C m⁻² d⁻¹). */
    double carbonFlux_soilPassivePool_to_soilActivePool;

    // =========================================================================
    // Nitrogen inter-pool fluxes (g N m⁻² d⁻¹) — reset each time step
    // =========================================================================

    /** @brief N flux from surface structural litter to slow pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;

    /** @brief N flux from surface structural litter to microbes pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;

    /** @brief N flux from surface metabolic litter to microbes pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;

    /** @brief N flux from soil structural litter to slow pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;

    /** @brief N flux from soil structural litter to active pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;

    /** @brief N flux from soil metabolic litter to active pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;

    /** @brief N flux from microbes pool to slow pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilMicrobesPool_to_soilSlowPool;

    /** @brief N flux from slow pool to active pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilSlowPool_to_soilActivePool;

    /** @brief N flux from active pool to slow pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilActivePool_to_soilSlowPool;

    /** @brief N flux from slow pool to passive pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilSlowPool_to_soilPassivePool;

    /** @brief N flux from passive pool to active pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilPassivePool_to_soilActivePool;

    /** @brief N flux from active pool to passive pool (g N m⁻² d⁻¹). */
    double nitrogenFlux_soilActivePool_to_soilPassivePool;

    // =========================================================================
    // Decomposition respiratory carbon losses (g C m⁻² d⁻¹) — reset each step
    // =========================================================================

    /** @brief CO₂-C respired during surface structural litter → slow pool decomposition. */
    double respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool;

    /** @brief CO₂-C respired during surface structural litter → microbes decomposition. */
    double respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool;

    /** @brief CO₂-C respired during soil structural litter → slow pool decomposition. */
    double respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;

    /** @brief CO₂-C respired during soil structural litter → active pool decomposition. */
    double respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool;

    /** @brief CO₂-C respired during surface metabolic litter → microbes decomposition. */
    double respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;

    /** @brief CO₂-C respired during soil metabolic litter → active pool decomposition. */
    double respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;

    /** @brief CO₂-C respired during microbes pool → slow pool decomposition. */
    double respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;

    /** @brief CO₂-C respired during active pool → passive+slow pool decomposition. */
    double respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool;

    /** @brief CO₂-C respired during slow pool → passive+active pool decomposition. */
    double respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool;

    /** @brief CO₂-C respired during passive pool → active pool decomposition. */
    double respiration_decompositionCarbon_soilPassivePool_soilActivePool;

    /** @brief Total C respiration from surface litter decomposition (g C m⁻² d⁻¹). */
    double respirationCarbon_surface_litter;

    /** @brief Total C respiration from soil litter decomposition (g C m⁻² d⁻¹). */
    double respirationCarbon_soil_litter;

    /** @brief Total C respiration from all litter (surface + soil, g C m⁻² d⁻¹). */
    double respirationCarbon_litter;

    /** @brief Total C respiration from SOM pools (active, slow, passive, g C m⁻² d⁻¹). */
    double respirationCarbon_soilpools;

    // =========================================================================
    // Decomposition respiratory nitrogen losses (g N m⁻² d⁻¹) — reset each step
    // =========================================================================

    /** @brief N respired during surface structural litter → slow pool decomposition. */
    double respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool;

    /** @brief N respired during surface structural litter → microbes decomposition. */
    double respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool;

    /** @brief N respired during soil structural litter → slow pool decomposition. */
    double respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool;

    /** @brief N respired during soil structural litter → active pool decomposition. */
    double respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool;

    /** @brief N respired during surface metabolic litter → microbes decomposition. */
    double respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;

    /** @brief N respired during soil metabolic litter → active pool decomposition. */
    double respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;

    /** @brief N respired during microbes pool → slow pool decomposition. */
    double respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;

    /** @brief N respired during active pool → passive+slow pool decomposition. */
    double respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;

    /** @brief N respired during slow pool → passive+active pool decomposition. */
    double respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;

    /** @brief N respired during passive pool → active pool decomposition. */
    double respiration_decompositionNitrogen_soilPassivePool_soilActivePool;

    // =========================================================================
    // Nitrogen mineralisation, immobilisation, and loss (g N m⁻² d⁻¹)
    // =========================================================================

    /** @brief Gross N mineralisation today (g N m⁻² d⁻¹). */
    double nitrogenGrossMineralization;

    /** @brief Net N mineralisation today (gross mineralisation − immobilisation, g N m⁻² d⁻¹). */
    double nitrogenNetMineralization;

    /** @brief N volatilisation loss today (g N m⁻² d⁻¹). */
    double nitrogenVolatilization;

    /** @brief N flow associated with surface structural litter → slow pool transfer. */
    double nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool;

    /** @brief N flow associated with surface structural litter → microbes transfer. */
    double nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool;

    /** @brief N flow associated with soil structural litter → slow pool transfer. */
    double nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool;

    /** @brief N flow associated with soil structural litter → active pool transfer. */
    double nitrogenFlow_soilStructuralLitterPool_to_soilActivePool;

    /** @brief N flow associated with surface metabolic litter → microbes transfer. */
    double nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool;

    /** @brief N flow associated with soil metabolic litter → active pool transfer. */
    double nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool;

    /** @brief N flow associated with microbes pool → slow pool transfer. */
    double nitrogenFlow_soilMicrobesPool_to_soilSlowPool;

    /** @brief N flow associated with active pool → passive pool transfer. */
    double nitrogenFlow_soilActivePool_to_soilPassivePool;

    /** @brief N flow associated with active pool → slow pool transfer. */
    double nitrogenFlow_soilActivePool_to_soilSlowPool;

    /** @brief N flow associated with slow pool → passive pool transfer. */
    double nitrogenFlow_soilSlowPool_to_soilPassivePool;

    /** @brief N flow associated with slow pool → active pool transfer. */
    double nitrogenFlow_soilSlowPool_to_soilActivePool;

    /** @brief N flow associated with passive pool → active pool transfer. */
    double nitrogenFlow_soilPassivePool_to_soilActivePool;

    /** @brief N immobilised from mineral pool during surface structural → slow transfer (g N m⁻² d⁻¹). */
    double immobilize_surfaceStructuralLitterPool_to_soilSlowPool;

    /** @brief N immobilised during surface structural → microbes transfer (g N m⁻² d⁻¹). */
    double immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool;

    /** @brief N immobilised during soil structural → slow pool transfer. */
    double immobilize_soilStructuralLitterPool_to_soilSlowPool;

    /** @brief N immobilised during soil structural → active pool transfer. */
    double immobilize_soilStructuralLitterPool_to_soilActivePool;

    /** @brief N immobilised during surface metabolic → microbes transfer. */
    double immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool;

    /** @brief N immobilised during soil metabolic → active pool transfer. */
    double immobilize_soilMetabolicLitterPool_to_soilActivePool;

    /** @brief N immobilised during microbes → slow pool transfer. */
    double immobilize_soilMicrobesPool_to_soilSlowPool;

    /** @brief N immobilised during active → passive pool transfer. */
    double immobilize_soilActivePool_to_soilPassivePool;

    /** @brief N immobilised during active → slow pool transfer. */
    double immobilize_soilActivePool_to_soilSlowPool;

    /** @brief N immobilised during slow → passive pool transfer. */
    double immobilize_soilSlowPool_to_soilPassivePool;

    /** @brief N immobilised during slow → active pool transfer. */
    double immobilize_soilSlowPool_to_soilActivePool;

    /** @brief N immobilised during passive → active pool transfer. */
    double immobilize_soilPassivePool_to_soilActivePool;

    /** @brief N mineralised during surface structural → slow pool transfer (g N m⁻² d⁻¹). */
    double mineralize_surfaceStructuralLitterPool_to_soilSlowPool;

    /** @brief N mineralised during surface structural → microbes transfer. */
    double mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool;

    /** @brief N mineralised during soil structural → slow pool transfer. */
    double mineralize_soilStructuralLitterPool_to_soilSlowPool;

    /** @brief N mineralised during soil structural → active pool transfer. */
    double mineralize_soilStructuralLitterPool_to_soilActivePool;

    /** @brief N mineralised during surface metabolic → microbes transfer. */
    double mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;

    /** @brief N mineralised during soil metabolic → active pool transfer. */
    double mineralize_soilMetabolicLitterPool_to_soilActivePool;

    /** @brief N mineralised during microbes → slow pool transfer. */
    double mineralize_soilMicrobesPool_to_soilSlowPool;

    /** @brief N mineralised during active → passive pool transfer. */
    double mineralize_soilActivePool_to_soilPassivePool;

    /** @brief N mineralised during active → slow pool transfer. */
    double mineralize_soilActivePool_to_soilSlowPool;

    /** @brief N mineralised during slow → passive pool transfer. */
    double mineralize_soilSlowPool_to_soilPassivePool;

    /** @brief N mineralised during slow → active pool transfer. */
    double mineralize_soilSlowPool_to_soilActivePool;

    /** @brief N mineralised during passive → active pool transfer. */
    double mineralize_soilPassivePool_to_soilActivePool;

    /** @brief Daily C leaching loss from the active pool (g C m⁻² d⁻¹). */
    double leachingCarbon;

    /** @brief Daily N leaching loss from the mineral pool (g N m⁻² d⁻¹). */
    double leachingNitrogen;

    /** @brief Cumulative C leached from the active pool over the entire simulation (g C m⁻²). */
    double carbonContent_leachedFromSoil;

    /** @brief Cumulative N leached from the mineral pool over the entire simulation (g N m⁻²). */
    double nitrogenContent_leachedFromSoil;

    /**
     * @brief Combined temperature × water effect on decomposition rates
     *        (dimensionless scalar, 0–1).
     */
    double decompositionFactor;

    /**
     * @brief Non-symbiotic N fixation + atmospheric N deposition added to the
     *        topsoil mineral N pool today (g N m⁻² d⁻¹).
     */
    double nitrogenFixationToSoil;

    // =========================================================================
    // Methods
    // =========================================================================

    /**
     * @brief Routes dying plant biomass C/N to the appropriate litter pool.
     */
    void transferDyingPlantPartsToLitterPools(UTILS utils, PARAMETER parameter, double number, double biomass, std::string typeOfMaterial, int pft, int numberOfTargetSoilLayers = 0);

    /**
     * @brief Calculates water limitation and plant water uptake using the BODIUM
     *        Feddes pressure-head model.
     */
    void calculateBodiumSoilWaterLimitationAndWaterUptake(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Calculates plant N uptake using the BODIUM convection + diffusion model.
     */
    void calculateBodiumSoilNitrogenUptake(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Orchestrates all soil resource dynamics (water + C/N) for one day.
     */
    void calculateSoilResourceDynamics(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community, INTERACTION interaction);

    /** @brief Executes the full daily soil water pipeline. */
    void calculateSoilWaterDynamics(UTILS utils, PARAMETER parameter, WEATHER weather, INTERACTION interaction, COMMUNITY &community);

    /** @brief Freezes precipitation as snow at T ≤ 0 °C; returns remaining liquid (mm). */
    double accumulateSnowWhenFreezing(UTILS utils, PARAMETER parameter, WEATHER weather, double waterInputToSoil);

    /** @brief Melts a fraction of the snowpack at T > 0 °C; returns total liquid water (mm). */
    double meltingOfSnow(UTILS utils, PARAMETER parameter, WEATHER weather, double waterInputToSoil);

    /** @brief Sublimates snow using PET; returns remaining PET after snow evaporation (mm). */
    double evaporationOfSnow(UTILS utils, WEATHER weather, PARAMETER parameter);

    /** @brief Calculates canopy interception; returns water remaining after interception (mm). */
    double interceptionByVegetation(UTILS utils, INTERACTION interaction, double dailyWaterInputToSoil, double remainingDailyPET);

    /** @brief Calculates surface run-off using a linear threshold; returns infiltrating water (mm). */
    double runOffAtSurface(UTILS utils, PARAMETER parameter, double dailyWaterInputToSoil);

    /** @brief Simulates layer-by-layer downward water percolation. */
    void soilWaterPercolation(UTILS utils, PARAMETER parameter, double waterInputToSoil);

    /** @brief Leaches mineral nitrogen downward coupled to percolation fluxes. */
    void leachingOfNitrogenCoupledToWaterPercolation(UTILS utils, PARAMETER parameter);

    /** @brief Orchestrates bare-soil evaporation from the top soil layers. */
    void evaporationFromTopSoilLayer(UTILS utils, PARAMETER parameter, COMMUNITY &community, double remainingDailyPET);

    /** @brief Returns the open-canopy fraction (1 − canopy closure) for evaporation. */
    double estimateSoilSurfaceAffectedBySoilEvaporation(UTILS utils, COMMUNITY community);

    /** @brief Returns cumulative depth of the top N soil layers affected by evaporation (cm). */
    double calculateSoilHorizonAffectedBySoilEvaporation(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation);

    /** @brief Returns normalised per-layer evaporation fractions using exponential depth weighting. */
    std::vector<double> calculateDistributionOfEvaporationAcrossAffectedSoilLayers(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, double soilDepthAffectedByEvaporation, double remainingDailyPET);

    /** @brief Returns actual per-layer evaporation (mm) limited by PWP. */
    std::vector<double> calculateSoilEvaporationPerSoilLayer(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, std::vector<double> proportionOfEvaporationPerSoilLayer, double openCanopy, double remainingDailyPET);

    /** @brief Deducts per-layer evaporation from the soil water pool (clamped to PWP). */
    void subtractSoilEvaporationFromSoilWaterPool(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, std::vector<double> soilEvaporation);

    /** @brief Sets internal soil water variables to NaN after reading from coupling interface (get mode). */
    void setSoilParametersAndVariablesToNaN(UTILS utils, PARAMETER parameter);

    /** @brief Copies internal soil parameters to the self-coupling interface (set mode). */
    void transferSoilParametersAndVariablesToInterface(UTILS utils);

    /** @brief Loads soil parameters from the self-coupling interface into internal state (get mode). */
    void transferSoilParametersAndVariablesFromInterface(UTILS utils);

    /**
     * @brief Orchestrates soil water demand, uptake, and GPP limitation for all cohorts.
     */
    void doPlantSoilWaterUptakeAndGppLimitationBySoilWaterConditions(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community);

    /** @brief Computes per-cohort soil water demand (GPP / WUE) and per-layer distribution. */
    void calculateSoilWaterDemandPerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Selects the water limitation approach and applies it to plant GPP. */
    void calculateSoilWaterUptakeByAvailableSoilWaterContentAndLimitPlantGpp(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Computes the soil-water GPP limitation factor per cohort (onesided or twosided model). */
    void calculateSoilWaterLimitationFactorPerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Calculates actual water uptake per cohort from demand × limitation factor. */
    void calculateSoilWaterUptakePerPlant(UTILS utils, COMMUNITY &community);

    /** @brief Multiplies each cohort's GPP by `limitingFactorGppWater`. */
    void reducePlantGppBySoilWaterLimitationFactor(UTILS utils, COMMUNITY &community);

    /** @brief Applies PET upper bound on community water uptake and GPP. */
    void limitPlantGppAndSoilWaterUptakeByPotentialEvapotranspiration(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community);

    /** @brief Prevents soil water from falling below PWP; reduces uptake and GPP accordingly. */
    void limitPlantGppAndSoilWaterUptakeByPermanentWiltingPoint(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /**
     * @brief Computes per-layer PWP-based uptake reduction factors and updates totals.
     */
    std::vector<double> limitSoilWaterUptakeByPermanentWiltingPoint(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Reduces per-cohort GPP and uptake based on PWP reduction factors. */
    void reducePlantGppBasedOnLimitationByPermanentWiltingPoint(UTILS utils, COMMUNITY &community, std::vector<double> limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer);

    /** @brief Subtracts community water uptake from soil pool (internal) or exports to BODIUM. */
    void subtractPlantWaterUptakeFromSoilWaterPool(UTILS utils, COMMUNITY &community, PARAMETER parameter);

    /**
     * @brief Orchestrates soil N demand, uptake, NPP limitation, and N allocation
     *        for all cohorts.
     */
    void doPlantSoilNitrogenUptakeAndNppLimitationBySoilNitrogenConditions(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Computes the rhizobial fixation efficiency limiting factor per cohort. */
    void calculateLimitingFactorOfSymbioticNitrogenFixationByRhizobiaPerPlant(UTILS utils, COMMUNITY &community, PARAMETER parameter);

    /** @brief Computes per-organ N demands and remaining soil demand after fixation/surplus. */
    void calculateSoilNitrogenDemandPerPlant(UTILS utils, COMMUNITY &community, PARAMETER parameter);

    /**
     * @brief Computes rhizobial N uptake and deducts its C cost from potential NPP carbon.
     */
    double calculateNitrogenUptakeByRhizobiaAndSubtractCostsFromPotentialNPP(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double carbonContentOfPotentialNPP);

    /** @brief Derives per-organ N demands and fraction vectors from NPP carbon and C:N ratios. */
    void calculateNitrogenDemandOfPlantFromPotentialNPP(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double carbonContentOfPotentialNPP);

    /** @brief Raises an error if rhizobial N uptake exceeds total plant N demand. */
    void checkIfPlantNitrogenDemandIsNotExceededByRhizobiaUptake(UTILS utils, COMMUNITY &community, int cohortindex);

    /** @brief Derives remaining soil N demand after rhizobia and surplus are subtracted. */
    void calculateRemainingPlantNitrogenDemandFromSoil(UTILS utils, COMMUNITY &community, int cohortindex);

    /** @brief Accumulates per-cohort soil N demand into community-level totals. */
    void summarizeTotalSoilNitrogenDemandFromAllPlants(UTILS utils, COMMUNITY &community);

    /** @brief Copies mineral N per layer to/from the self-coupling interface. */
    void transferInterfaceVariablesForSelfCoupling_soilNitrogen(UTILS utils, PARAMETER parameter);

    /** @brief Resets internal mineral N pool to NaN after it has been read from coupling interface. */
    void resetInterfaceVariablesForSelfCoupling_soilNitrogen(UTILS utils, PARAMETER parameter);

    /** @brief Calculates actual N uptake per cohort as min(demand, equal share of available N). */
    void calculateSoilNitrogenUptakePerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Computes the N limitation factor for NPP per cohort. */
    void calculateNitrogenLimitationFactorPerPlant(UTILS utils, COMMUNITY &community);

    /** @brief Distributes allocable N to per-organ uptake fields. */
    void allocateNitrogenUptakeToPlant(UTILS utils, COMMUNITY &community);

    /** @brief Updates the plant nitrogen surplus pool after N allocation. */
    void updateNitrogenSurplusPoolOfPlant(UTILS utils, COMMUNITY &community, int cohortindex, double allocableNitrogen);

    /** @brief Accumulates per-cohort soil N uptake into community-level totals. */
    void summarizeTotalSoilNitrogenUptakeFromAllPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Deducts community N uptake from the per-layer mineral N pool. */
    void subtractPlantNitrogenUptakeFromSoilMineralNitrogenPool(UTILS utils, PARAMETER parameter, COMMUNITY &community);

    /** @brief Orchestrates the full soil C/N decomposition pipeline for one day. */
    void calculateSoilCarbonNitrogenDynamics(UTILS utils, PARAMETER parameter, INTERACTION interaction, WEATHER weather);

    /** @brief Splits surface and soil litter C/N fluxes to structural and metabolic pools. */
    void splitLitterFluxesToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather);

    /** @brief Partitions a combined litter flux into structural and metabolic fractions. */
    void addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather, std::string type);

    /**
     * @brief Calculates DIRABS — direct mineral N absorption to meet structural
     *        litter C/N requirement.
     */
    double calculateDIRABS(UTILS utils, std::string type);

    /**
     * @brief Computes the daily lignin fraction of incoming litter from precipitation.
     */
    double calculateLigninFraction(UTILS utils, WEATHER weather, PARAMETER parameter, std::string type);

    /**
     * @brief Computes the nitrogen fraction of the combined litter flux.
     */
    double calculateNitrogenFraction(UTILS utils, double dirabs, std::string type);

    /**
     * @brief Computes the metabolic litter fraction from lignin content.
     */
    double calculateFractionOfMetabolicLitter(UTILS utils, double fractionOfLignin, double ligninToNitrogenRatio, std::string type);

    /**
     * @brief Adjusts structural litter lignin content after adding new material.
     */
    double adjustLigninContentOfStructuralLitter(UTILS utils, double fractionOfLignin, double carbonAddedToStructuralLitter, double strlig, std::string type);

    /** @brief Transfers structural/metabolic C and N fluxes to pool variables; resets input pools. */
    void processLitterFluxes(UTILS utils, double dirabs, double cadds, double caddm, double eadds, double eaddm, std::string type);

    /**
     * @brief Computes temperature × water decomposition scaling factor.
     */
    double calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, INTERACTION interaction, PARAMETER parameter);

    /** @brief Calls startDecomposition() for each active pool. */
    void doDecompositionFluxesInLitterAndSoilPools(UTILS utils);

    /** @brief Initiates decomposition from one pool using its rate constant and lignin content. */
    void startDecomposition(UTILS utils, double carbonPool, double factor, double lignin, std::string typeOfSoilPool);

    /** @brief Precomputes decisive C:N ratios for mineralisation/immobilisation decisions. */
    void calculateDecisiveCarbonNitrogenRatiosForDecomposition(UTILS utils, std::string typeOfPool);

    /**
     * @brief Computes the decomposition flux and updates C/N tracking variables.
     */
    bool decompose(UTILS utils, double flow, double ligcon, std::string type);

    /**
     * @brief Returns `true` if a pool has sufficient C content to decompose.
     */
    bool decomposable(UTILS utils, std::string type);

    /** @brief Computes C respiratory loss for a pool-to-pool transfer. */
    void calculateCarbonRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool);

    /** @brief Computes N respiratory loss for a pool-to-pool transfer. */
    void calculateNitrogenRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool);

    /** @brief Determines N flow direction and routes to immobiliseOrMineralizeNitrogen(). */
    void determineNitrogenFlux(UTILS utils, std::string transferFromPoolype, std::string transferToPool);

    /** @brief Dispatches to immobilizeNitrogen() or mineralizeNitrogen() based on decisive C:N ratio. */
    void immobilizeOrMineralizeNitrogen(UTILS utils, double carbonFlux, double nitrogenFlow, double decisiveCNratio, std::string transferFromPool, std::string transferToPool);

    /** @brief Immobilises mineral N from the topsoil pool to support N-poor decomposition. */
    void immobilizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio);

    /** @brief Mineralises N from N-rich decomposing material to the mineral N pool. */
    void mineralizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio, double oldflow);

    /** @brief Computes non-symbiotic N fixation and atmospheric deposition; adds to mineral pool. */
    void calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(UTILS utils, PARAMETER parameter, WEATHER weather);

    /** @brief Computes N volatilisation loss from the topsoil mineral N pool. */
    void calculateNitrogenLossByVolatilization(UTILS utils);

    /** @brief Applies all accumulated C/N fluxes and respiratory losses to update pool contents. */
    void updateSoilPoolsByRespirationAndFluxes(UTILS utils);

    /** @brief Deducts C leached from the active soil pool. */
    void doLeaching(UTILS utils);
};
