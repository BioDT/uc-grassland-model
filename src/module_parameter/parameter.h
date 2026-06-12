/**
 * @file parameter.h
 * @brief Declares the PARAMETER class, which holds all model parameters for a simulation run.
 *
 * Parameters are organised into four groups:
 * 1. **Runtime variables** — day counter and Julian-day boundaries computed at start-up.
 * 2. **Configuration file parameters** — site metadata, input/output file names, output
 *    flags, and general simulation settings (read by INPUT from the config file).
 * 3. **Plant traits file parameters** — PFT-specific physiological, allometric, and
 *    phenological parameters (read by INPUT from the plant traits file).
 * 4. **Process setup file parameters** — sub-model activation flags (read by INPUT from
 *    the process setup file).
 *
 * A set of derived parameters (soil geometry) is also stored here to avoid circular
 * include dependencies between the PARAMETER and SOIL headers.
 *
 * All parameter values are populated by the corresponding
 * `INPUT::transfer*ParameterValueToModelParameter()` methods after the input files
 * have been parsed.
 */
#pragma once
#include "../utils/utils.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

/**
 * @class PARAMETER
 * @brief Plain-data container for all model parameters and runtime state variables.
 *
 * No logic is implemented here; the class is a structured store filled by INPUT and
 * read throughout the model. Per-PFT parameters are stored as `std::vector` with one
 * element per PFT (indexed 0 … pftCount−1).
 */
class PARAMETER
{
public:
    PARAMETER();
    ~PARAMETER();

    // =========================================================================
    // Runtime variables (not read from input files)
    // =========================================================================

    /** @brief Current simulation day (1-based, incremented each time step). */
    int day;

    /**
     * @brief Julian day number of the first day of the simulation
     *        (1 January of `firstYear`).
     * Used as the reference offset for converting calendar dates to day-count
     * indices throughout the model.
     */
    int referenceJulianDayStart;

    /**
     * @brief Julian day number of the last day of the simulation
     *        (31 December of `lastYear`).
     */
    int referenceJulianDayEnd;

    /** @brief Total length of the simulation in days
     *         (`referenceJulianDayEnd − referenceJulianDayStart + 1`). */
    int simulationTimeInDays;

    // =========================================================================
    // Configuration file parameters
    // =========================================================================

    /**
     * @brief Keyword list used by INPUT to locate configuration parameters in the
     *        config file.
     *
     * Each string is the exact keyword that INPUT searches for in the config file.
     * The order does not affect parsing.
     */
    std::vector<std::string> configParameterNames =
        {"deimsID", "latitude", "longitude", "lastYear", "firstYear",
         "weatherFile", "soilFile", "managementFile", "plantTraitsFile", "processSetupFile",
         "communityOutputFile", "pftOutputFile", "plantCohortOutputFile", "soilCarbonOutputFile",
         "soilNitrogenOutputFile", "soilWaterOutputFile", "soilResourcesPerSoilLayerOutputFile",
         "soilFluxesDetailsOutputFile", "outputWritingDatesFile", "clippingHeightOfBiomassMeasurement",
         "randomNumberGeneratorSeed"};

    /** @brief DEIMS.iD site identifier string (may be "NA" if unavailable). */
    std::string deimsID;

    /** @brief Site latitude as a string (used in output file naming). */
    std::string latitude;

    /** @brief Site longitude as a string (used in output file naming). */
    std::string longitude;

    /** @brief Last calendar year of the simulation (inclusive). */
    int lastYear;

    /** @brief First calendar year of the simulation (inclusive). */
    int firstYear;

    /** @brief File name of the daily weather time-series input file (`.txt`). */
    std::string weatherFile;

    /** @brief File name of the soil properties input file (`.txt`). */
    std::string soilFile;

    /** @brief File name of the management events input file (`.txt`). */
    std::string managementFile;

    /** @brief File name of the plant traits parameter file (`.txt`). */
    std::string plantTraitsFile;

    /** @brief File name of the process setup parameter file (`.txt`). */
    std::string processSetupFile;

    /** @brief File name of the soil parameters file (`.txt`); currently unused at runtime. */
    std::string soilParametersFile;

    /** @brief `true` if the community-level output file should be written. */
    bool communityOutputFile;

    /** @brief `true` if the PFT population output file should be written. */
    bool pftOutputFile;

    /** @brief `true` if the plant-cohort output file should be written. */
    bool plantCohortOutputFile;

    /** @brief `true` if the soil carbon output file should be written. */
    bool soilCarbonOutputFile;

    /** @brief `true` if the soil nitrogen output file should be written. */
    bool soilNitrogenOutputFile;

    /** @brief `true` if the soil water output file should be written. */
    bool soilWaterOutputFile;

    /** @brief `true` if the per-soil-layer resources output file should be written. */
    bool soilResourcesPerSoilLayerOutputFile;

    /** @brief `true` if the detailed soil-flux output file should be written. */
    bool soilFluxesDetailsOutputFile;

    /**
     * @brief File name of the optional output-writing-dates file, or `"NaN"` / `""`
     *        if output should be written at daily resolution.
     */
    std::string outputWritingDatesFile;

    /**
     * @brief Height above which shoot biomass is counted as clipped biomass during
     *        biomass measurement (cm).
     */
    double clippingHeightOfBiomassMeasurement;

    /**
     * @brief Seed for the pseudo-random number generator.
     */
    unsigned int randomNumberGeneratorSeed;

    // =========================================================================
    // Plant traits file parameters
    // =========================================================================

    /**
     * @brief Keyword list used by INPUT to locate plant-trait parameters in the
     *        plant traits file.
     */
    std::vector<std::string> plantTraitsParameterNames =
        {"pftCount", "maximumPlantHeight", "plantHeightToWidthRatio", "plantShootCorrectionFactor", "plantShootRootRatio",
         "plantRootDepthParamIntercept", "plantRootDepthParamExponent", "plantSpecificLeafArea", "plantShootOverlapFactors",
         "crowdingMortalityActivated", "brownBiomassFractionFalling", "rootLifeSpan", "leafLifeSpan", "plantLifeSpan", "plantMortalityProbability",
         "seedlingMortalityProbability", "seedGerminationTimes", "seedGerminationRates",
         "seedsFromMaturePlantsActivated", "seedMasses", "maturityAges", "maturityHeights", "externalSeedInfluxActivated", "externalSeedInfluxNumber", "dayOfExternalSeedInfluxStart",
         "communityShadingInGppCalculation", "maximumGrossLeafPhotosynthesisRate", "initialSlopeOfLightResponseCurve", "lightExtinctionCoefficients", "growthRespirationFraction",
         "maintenanceRespirationRate", "plantNppAllocationGrowth", "plantNppAllocationExudation", "useStaticShootRootAllocationRates", "plantCNRatioGreenLeaves", "plantCNRatioBrownLeaves", "plantCNRatioRoots", "plantCNRatioSeeds", "plantCNRatioExudates", "symbioticNitrogenFixationFraction",
         "rhizobiaExchangeRateCToN", "plantWaterUseEfficiency",
         "plantGppReductionBySoilWaterApproach", "lowerSoilWaterFractionForPlantGppReduction",
         "lowerSoilWaterContentForPlantGppReduction", "upperSoilWaterContentForPlantGppReduction",
         "plantResponseToTemperatureQ10Base", "plantResponseToTemperatureQ10Reference",
         "h2L", "h2H", "crowdingCalculationFromPlantTopLayer", "minLayerReductionFactorFromAverage", "disableRunoff", "tresholdCohortDeathDeterministic"};

    /** @brief Number of plant functional types (PFTs) in the simulation. */
    int pftCount;

    /** @brief Maximum attainable plant height per PFT (cm). */
    std::vector<double> maximumPlantHeight;

    /** @brief Allometric height-to-width ratio per PFT (cm cm⁻¹). */
    std::vector<double> plantHeightToWidthRatio;

    /**
     * @brief Shoot correction factor per PFT used in allometric height/width
     *        calculations (dimensionless).
     */
    std::vector<double> plantShootCorrectionFactor;

    /** @brief Target shoot-to-root biomass ratio per PFT (g g⁻¹). */
    std::vector<double> plantShootRootRatio;

    /** @brief Intercept of the root-depth allometric equation per PFT (cm). */
    std::vector<double> plantRootDepthParamIntercept;

    /** @brief Exponent of the root-depth allometric equation per PFT (dimensionless). */
    std::vector<double> plantRootDepthParamExponent;

    /** @brief Specific leaf area per PFT (cm² g⁻¹ ODM). */
    std::vector<double> plantSpecificLeafArea;

    /**
     * @brief Shoot canopy overlap factor per PFT (dimensionless, 0–1).
     */
    std::vector<double> plantShootOverlapFactors;

    /**
     * @brief `true` if density-dependent (crowding) mortality is active.
     */
    bool crowdingMortalityActivated;

    /**
     * @brief Daily fraction of brown shoot biomass that falls to the surface
     *        litter pool (dimensionless, 0–1).
     */
    double brownBiomassFractionFalling;

    /** @brief Mean root life span per PFT (days); turnover rate = 1 / rootLifeSpan. */
    std::vector<double> rootLifeSpan;

    /** @brief Mean leaf life span per PFT (days); browning rate = 1 / leafLifeSpan. */
    std::vector<double> leafLifeSpan;

    /**
     * @brief Life-span category per PFT: `"annual"` or `"perennial"`.
     */
    std::vector<std::string> plantLifeSpan;

    /** @brief Daily adult mortality probability per PFT (d⁻¹, 0–1). */
    std::vector<double> plantMortalityProbability;

    /** @brief Daily seedling mortality probability per PFT (d⁻¹, 0–1). */
    std::vector<double> seedlingMortalityProbability;

    /** @brief Required number of days between seed dispersal and germination per PFT. */
    std::vector<int> seedGerminationTimes;

    /** @brief Daily seed germination probability per PFT (d⁻¹, 0–1). */
    std::vector<double> seedGerminationRates;

    /**
     * @brief `true` if mature plants produce seeds for recruitment.
     */
    bool seedsFromMaturePlantsActivated;

    /** @brief Individual seed mass per PFT (g ODM seed⁻¹). */
    std::vector<double> seedMasses;

    /** @brief Age at which a plant cohort transitions from seedling to adult (days). */
    std::vector<double> maturityAges;

    /** @brief Height at which a plant cohort is considered mature for NPP allocation (cm). */
    std::vector<double> maturityHeights;

    /**
     * @brief `true` if an external seed source (influx) is active.
     */
    bool externalSeedInfluxActivated;

    /** @brief Number of externally supplied seeds per PFT and influx event. */
    std::vector<int> externalSeedInfluxNumber;

    /**
     * @brief Simulation day on which the external seed influx begins (1-based
     *        day-count offset from `referenceJulianDayStart`; converted from date).
     */
    int dayOfExternalSeedInfluxStart;

    /**
     * @brief `true` if the community-shading photosynthesis model is used;
     *        `false` for the simpler individual-plant model.
     */
    bool communityShadingInGppCalculation;

    /**
     * @brief Maximum gross leaf photosynthesis rate per PFT
     *        (µmol CO₂ m⁻² s⁻¹ at saturating light).
     */
    std::vector<double> maximumGrossLeafPhotosynthesisRate;

    /**
     * @brief Initial slope of the light-response curve per PFT
     *        (µmol CO₂ µmol⁻¹ photons; quantum yield).
     */
    std::vector<double> initialSlopeOfLightResponseCurve;

    /** @brief Light extinction coefficient per PFT (dimensionless; Beer-Lambert k). */
    std::vector<double> lightExtinctionCoefficients;

    /**
     * @brief Fraction of (GPP − maintenance respiration) lost as growth respiration
     *        (dimensionless, 0–1).
     */
    double growthRespirationFraction;

    /**
     * @brief Base maintenance respiration rate (g ODM g⁻¹ ODM d⁻¹) applied to
     *        the sum of green shoot and root biomass.
     */
    double maintenanceRespirationRate;

    /**
     * @brief Fraction of NPP allocated to vegetative growth (shoot + root) per PFT
     *        (dimensionless, 0–1).
     */
    std::vector<double> plantNppAllocationGrowth;

    /** @brief Fraction of NPP allocated to root exudates per PFT (dimensionless, 0–1). */
    std::vector<double> plantNppAllocationExudation;

    /**
     * @brief `true` if the shoot-to-root allocation split uses a fixed ratio;
     *        `false` for the dynamic mode that adjusts to reach the target ratio.
     */
    bool useStaticShootRootAllocationRates;

    /** @brief Carbon-to-nitrogen ratio of green leaves per PFT (g C g⁻¹ N). */
    std::vector<double> plantCNRatioGreenLeaves;

    /** @brief Carbon-to-nitrogen ratio of brown (senescent) leaves per PFT (g C g⁻¹ N). */
    std::vector<double> plantCNRatioBrownLeaves;

    /** @brief Carbon-to-nitrogen ratio of roots per PFT (g C g⁻¹ N). */
    std::vector<double> plantCNRatioRoots;

    /** @brief Carbon-to-nitrogen ratio of seeds per PFT (g C g⁻¹ N). */
    std::vector<double> plantCNRatioSeeds;

    /** @brief Carbon-to-nitrogen ratio of root exudates per PFT (g C g⁻¹ N). */
    std::vector<double> plantCNRatioExudates;

    /**
     * @brief Fraction of plant nitrogen demand met by symbiotic N₂ fixation per PFT
     *        (dimensionless, 0–1).
     */
    std::vector<double> symbioticNitrogenFixationFraction;

    /**
     * @brief Carbon-to-nitrogen exchange rate for rhizobial symbiosis
     *        (g C g⁻¹ N); governs the carbon cost of symbiotic N fixation.
     */
    double rhizobiaExchangeRateCToN;

    /**
     * @brief Plant water-use efficiency per PFT (g ODM mm⁻¹ H₂O); used to
     *        convert transpiration demand to GPP limitation.
     */
    std::vector<double> plantWaterUseEfficiency;

    /**
     * @brief Identifier string for the soil-water GPP-reduction approach per PFT
     *        (e.g. `"fraction"` or `"content"`).
     */
    std::string plantGppReductionBySoilWaterApproach;

    /**
     * @brief Lower soil-water fraction threshold below which GPP begins to be
     *        reduced per PFT (fraction of field capacity, 0–1).
     */
    std::vector<double> lowerSoilWaterFractionForPlantGppReduction;

    /**
     * @brief Absolute lower soil-water content threshold for GPP reduction per PFT
     *        (mm); used when the `"content"` approach is selected.
     */
    std::vector<double> lowerSoilWaterContentForPlantGppReduction;

    /**
     * @brief Absolute upper soil-water content threshold above which GPP is no
     *        longer limited per PFT (mm).
     */
    std::vector<double> upperSoilWaterContentForPlantGppReduction;

    /**
     * @brief Q₁₀ base value for the plant temperature-response function
     *        (dimensionless; default ≈ 2).
     */
    double plantResponseToTemperatureQ10Base;

    /**
     * @brief Reference temperature for the Q₁₀ respiration model (°C).
     */
    double plantResponseToTemperatureQ10Reference;

    // ---- Coupling-specific parameters ----------------------------------------

    /**
     * @brief Lower radiation threshold for the coupling light-scaling function
     *        (MJ m⁻² d⁻¹ or matching weather input units).
     */
    double h2L;

    /**
     * @brief Upper radiation threshold for the coupling light-scaling function
     *        (same units as `h2L`).
     */
    double h2H;

    /**
     * @brief `1` if crowding mortality uses per-height-layer covered area;
     *        `0` for community-wide total area (flag stored as int for input compatibility).
     */
    int crowdingCalculationFromPlantTopLayer;

    /**
     * @brief Minimum number of height layers used when averaging the layer-reduction
     *        factor in the coupling interface (dimensionless).
     */
    int minLayerReductionFactorFromAverage;

    /**
     * @brief `1` if surface and subsurface run-off are disabled in the soil water
     *        module (flag stored as int for input compatibility).
     */
    int disableRunoff;

    // =========================================================================
    // Process setup file parameters
    // =========================================================================

    /**
     * @brief Keyword list used by INPUT to locate process-setup parameters in the
     *        process setup file.
     */
    std::vector<std::string> processSetupParameterNames =
        {"useInternalSoilModule", "useExternalSoilModule_BODIUM", "useExternalSoilModule_selfCoupled_getVariables", "useInternalSoilModule_selfCoupled_setVariables",
         "stochasticSimulation"};

    /** @brief `true` if the internal soil C/N/water module is active. */
    bool useInternalSoilModule;

    /** @brief `true` if the external BODIUM soil module is active (mutually exclusive with `useInternalSoilModule`). */
    bool useExternalSoilModule_BODIUM;

    /**
     * @brief `true` if the self-coupling interface is in *get* mode (reads soil
     *        variables from the coupling interface instead of computing them).
     */
    bool useExternalSoilModule_selfCoupled_getVariables;

    /**
     * @brief `true` if the self-coupling interface is in *set* mode (writes soil
     *        variables to the coupling interface for retrieval by another instance).
     */
    bool useInternalSoilModule_selfCoupled_setVariables;

    /**
     * @brief `true` if stochastic processes (recruitment, crowding, basic mortality)
     *        use random draws; `false` for fully deterministic execution.
     */
    bool stochasticSimulation;

    /**
     * @brief Minimum cohort size below which deterministic mortality kills the
     *        entire cohort (individuals).
     */
    float tresholdCohortDeathDeterministic;

    // =========================================================================
    // Derived parameters (not in input files; set from soil file via INPUT)
    // =========================================================================

    /**
     * @brief Total soil depth (cm); sum of all `soilLayerWidth` entries.
     */
    double soilDepth;

    /** @brief Number of soil layers; equals the number of rows in the soil file. */
    int numberOfSoilLayers;

    /** @brief Width (thickness) of each soil layer (cm), from top to bottom. */
    std::vector<double> soilLayerWidth;
};
