/**
 * @file plant.h
 * @brief Declares the PLANT class, which represents a single plant cohort in
 *        the grassland simulation.
 *
 * A cohort groups a number of identical plants (`amount`) that share the same
 * state variables. All quantities stored here refer to **one representative
 * individual**; community-level totals are computed by multiplying by `amount`
 * in the respective modules.
 *
 * PLANT objects are heap-allocated and managed via `std::shared_ptr<PLANT>` in
 * `COMMUNITY::allPlants`. New cohorts are created by the recruitment module;
 * dead cohorts are removed by MORTALITY after all death processes.
 *
 * Consistent units:
 * - Length / height / depth / width: **cm**
 * - Area: **cm²**
 * - Biomass: **g ODM** (organic dry matter)
 * - Carbon: **g C**
 * - Nitrogen: **g N**
 * - Fluxes (GPP, NPP, respiration, demands/uptakes): **g ODM d⁻¹** or **g C/N d⁻¹**
 * - LAI: **m² m⁻²** (dimensionless)
 * - Radiation: **µmol photons m⁻² s⁻¹**
 * - Allocation fractions, limiting factors: **dimensionless (0–1)**
 */
#pragma once
#include "allometry.h"
#include "../module_parameter/parameter.h"
#include "../module_init/constants.h"
#include "../utils/utils.h"
#include <iostream>

/**
 * @class PLANT
 * @brief Represents one plant cohort — a group of identical individual plants
 *        described by a single set of state variables.
 *
 * State variables cover geometry, biomass pools (C and N), light climate,
 * carbon-balance fluxes, NPP allocation fractions, and water/nitrogen
 * demand/uptake fields. All values refer to a single representative plant;
 * multiply by `amount` for cohort-level or community-level totals.
 */
class PLANT
{
public:
    /** @brief Default constructor. All members are value-initialised. */
    PLANT();

    /**
     * @brief Constructs and fully initialises a new plant cohort at seedling stage.
     *
     * Sets all geometry, biomass, carbon, nitrogen, allocation, light, and
     * demand/uptake fields from the seed mass and PFT-specific parameters.
     * Initial biomass equals `parameter.seedMasses[pft]`; height, width,
     * covered area, and LAI are computed via the provided allometry object.
     * NPP allocation fractions are initialised from the shoot-root ratio and
     * exudation fraction; recruitment allocation is set to zero (seedlings do
     * not produce seeds). All per-layer water and nitrogen vectors are sized to
     * `parameter.numberOfSoilLayers` and filled with zero.
     *
     * Raises an error if:
     * - `plantLifeSpan == "annual"` but `maturityAges[pft] > 365`.
     * - The shoot-root ratio would cause division by zero in NPP allocation.
     *
     * @param utils     Utility object for error handling.
     * @param parameter PFT-specific and global model parameters.
     * @param allometry Allometric helper for geometry and biomass conversions.
     * @param pft       Plant functional type index (0-based).
     * @param amount    Number of individual plants represented by this cohort.
     */
    PLANT(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, int pft, double amount) : pft(pft), amount(amount)
    {
        /* properties and geometry */
        age = 0;
        plantBiomass = parameter.seedMasses[pft];
        height = allometry.heightFromPlantBiomassShootCorrectionAndByRatios(utils, plantBiomass, parameter.plantHeightToWidthRatio[pft], parameter.plantShootCorrectionFactor[pft], parameter.plantShootRootRatio[pft]);
        width = allometry.widthFromHeightByRatio(utils, height, parameter.plantHeightToWidthRatio[pft]);
        coveredArea = allometry.areaFromWidth(width);

        /* plant biomass pools */
        shootBiomassGreenLeaves = allometry.shootBiomassFromHeightWidthShootCorrection(height, width, parameter.plantShootCorrectionFactor[pft]);
        shootBiomassBrownLeaves = 0.0;
        shootBiomass = shootBiomassGreenLeaves + shootBiomassBrownLeaves;
        rootBiomass = allometry.rootBiomassFromShootBiomass(utils, shootBiomass, parameter.plantShootRootRatio[pft]);
        recruitmentBiomass = 0.0;
        exudationBiomass = 0.0;
        shootBiomassAboveClippingHeight = 0.0;

        /* carbon content of plant pools */
        shootCarbonGreenLeaves = shootBiomassGreenLeaves * CARBON_CONTENT_ODM;
        shootCarbonBrownLeaves = shootBiomassBrownLeaves * CARBON_CONTENT_ODM;
        shootCarbon = shootCarbonBrownLeaves + shootCarbonGreenLeaves;
        rootCarbon = rootBiomass * CARBON_CONTENT_ODM;
        recruitmentCarbon = recruitmentBiomass * CARBON_CONTENT_ODM;
        exudationCarbon = exudationBiomass * CARBON_CONTENT_ODM;
        plantCarbon = plantBiomass * CARBON_CONTENT_ODM;

        /* nitrogen content of plant pools */
        if (parameter.plantCNRatioGreenLeaves[pft] > 0.0)
        {
            shootNitrogenGreenLeaves = shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
        }
        if (parameter.plantCNRatioBrownLeaves[pft] > 0.0)
        {
            shootNitrogenBrownLeaves = shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
        }
        shootNitrogen = shootNitrogenBrownLeaves + shootNitrogenGreenLeaves;
        if (parameter.plantCNRatioRoots[pft] > 0.0)
        {
            rootNitrogen = rootCarbon / parameter.plantCNRatioRoots[pft];
        }
        if (parameter.plantCNRatioSeeds[pft] > 0.0)
        {
            recruitmentNitrogen = recruitmentCarbon / parameter.plantCNRatioSeeds[pft];
        }
        if (parameter.plantCNRatioExudates[pft] > 0.0)
        {
            exudationNitrogen = exudationCarbon / parameter.plantCNRatioExudates[pft];
        }
        plantNitrogen = shootNitrogen + rootNitrogen;

        /* root architecture */
        rootingDepth = allometry.rootDepthFromRootBiomassParametersRatioAndShootCorrection(utils, rootBiomass, parameter.plantRootDepthParamIntercept[pft], parameter.plantRootDepthParamExponent[pft], parameter.plantShootRootRatio[pft], parameter.plantShootCorrectionFactor[pft]);
        numberOfSoilLayersRooting = allometry.calculateNumberOfRootingSoillayer(parameter.soilLayerWidth, rootingDepth);
        if (numberOfSoilLayersRooting > parameter.numberOfSoilLayers)
        {
            utils.handleError("numberOfSoilLayersRooting (" + std::to_string(numberOfSoilLayersRooting) + ") exceeds parameter.numberOfSoilLayers (" + std::to_string(parameter.numberOfSoilLayers) + ").");
            // utils.handleWarning("numberOfSoilLayersRooting (" + std::to_string(numberOfSoilLayersRooting) + ") exceeds parameter.numberOfSoilLayers (" + std::to_string(parameter.numberOfSoilLayers) + "). Capping to maximum.");
            // numberOfSoilLayersRooting = parameter.numberOfSoilLayers;
        }

        /* leaf area and structure */
        laiGreen = allometry.laiFromShootBiomassAreaSla(utils, shootBiomassGreenLeaves, coveredArea, parameter.plantSpecificLeafArea[pft]);
        laiBrown = 0.0;
        lai = laiGreen + laiBrown;

        /* mortality */
        if (parameter.plantLifeSpan[pft] == "annual" && parameter.maturityAges[pft] > 365)
        {
            utils.handleError("Plant species is defined as annual, but their maturity age is set to an age larger than one year. Please adjust maturityAges in the plant traits file!");
        }

        /* light climate */
        cumulativeOvertoppingCommunityLAI = 0.0;
        availableRadiation = 0.0;
        shadingIndicator = -1;

        /* growth and physiology */
        gpp = 0.0;
        npp = 0.0;
        nppBuffer = 0.0;
        totalRespiration = 0.0;
        growthRespiration = 0.0;
        maintenanceRespiration = 0.0;

        /* NPP allocation fractions */
        nppAllocationShoot = 0.0;
        nppAllocationRoot = 0.0;
        if ((1.0 + parameter.plantShootRootRatio[pft]) > 0.0)
        {
            nppAllocationShoot = (1.0 - parameter.plantNppAllocationExudation[pft]) * (parameter.plantShootRootRatio[pft] / (1.0 + parameter.plantShootRootRatio[pft])); // init with full allocation to shoot and root
            nppAllocationRoot = (1.0 - parameter.plantNppAllocationExudation[pft]) / (1.0 + parameter.plantShootRootRatio[pft]);
        }
        else
        {
            utils.handleError("Error (plant constructor): shoot-root ratio results in division by zero for NPP allocation.");
        }
        nppAllocationRecruitment = 0.0;
        nppAllocationExudation = parameter.plantNppAllocationExudation[pft];

        /* water demand and uptake */
        limitingFactorGppWater = 1.0;
        soilWaterDemand = 0.0;
        soilWaterUptake = 0.0;
        soilWaterDemandPerSoilLayer.clear();
        soilWaterUptakePerSoilLayer.clear();
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            soilWaterDemandPerSoilLayer.push_back(0.0);
            soilWaterUptakePerSoilLayer.push_back(0.0);
        }

        /* nitrogen demand and uptake */
        limitingFactorSymbiosisRhizobia = 1.0;
        rhizobiaNitrogenUptake = 0.0;
        nitrogenSurplus = 0.0;

        nitrogenDemandForGrowthOfShoot = 0.0;
        nitrogenDemandForGrowthOfRoot = 0.0;
        nitrogenDemandForReproduction = 0.0;
        nitrogenDemandForExudation = 0.0;
        totalPlantNitrogenDemand = 0.0;
        nitrogenDemandGrowthFractionShoot = 0.0;
        nitrogenDemandGrowthFractionRoot = 0.0;
        nitrogenDemandGrowthFractionReproduction = 0.0;
        nitrogenDemandGrowthFractionExudation = 0.0;

        totalSoilNitrogenDemand = 0.0;
        totalSoilNitrogenUptake = 0.0;
        soilNitrogenDemandPerSoilLayer.clear();
        soilNitrogenUptakePerSoilLayer.clear();
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            soilNitrogenDemandPerSoilLayer.push_back(0.0);
            soilNitrogenUptakePerSoilLayer.push_back(0.0);
        }

        shootNitrogenUptakeForGreenLeaves = 0.0;
        rootNitrogenUptake = 0.0;
        recruitmentNitrogenUptake = 0.0;
        exudationNitrogenUptake = 0.0;
        limitingFactorNppNitrogen = 1.0;
    }

    /** @brief Destructor. No dynamic resources are owned; all members destroyed automatically. */
    ~PLANT();

    // =========================================================================
    // Identity and cohort size
    // =========================================================================

    /** @brief Number of identical individual plants represented by this cohort. */
    double amount;

    /** @brief Plant functional type index (0-based); indexes into all PFT parameter vectors. */
    short pft;

    /** @brief Plant age (days since germination; incremented each time step). */
    double age;

    // =========================================================================
    // Geometry
    // =========================================================================

    /** @brief Ground area covered by the plant canopy (cm²). */
    double coveredArea;

    /** @brief Canopy diameter / width (cm). */
    double width;

    /** @brief Plant height (cm). */
    double height;

    /** @brief Green (photosynthetically active) leaf area index (m² m⁻²). */
    double laiGreen;

    /** @brief Brown (senescent) leaf area index (m² m⁻²). */
    double laiBrown;

    /** @brief Total leaf area index = laiGreen + laiBrown (m² m⁻²). */
    double lai;

    /** @brief Root-zone depth (cm); computed from root biomass via the allometric power-law equation. */
    double rootingDepth;

    /** @brief Number of soil layers reached by plant roots (integer ≥ 1). */
    int numberOfSoilLayersRooting;

    // =========================================================================
    // Biomass pools (g ODM per plant)
    // =========================================================================

    /** @brief Total aboveground shoot biomass = green + brown leaves (g ODM). */
    double shootBiomass;

    /** @brief Photosynthetically active green shoot biomass (g ODM). */
    double shootBiomassGreenLeaves;

    /** @brief Senescent brown shoot biomass (g ODM). */
    double shootBiomassBrownLeaves;

    /**
     * @brief Shoot biomass above the biomass-measurement clipping height (g ODM).
     * Represents the harvestable fraction for comparison with field clip data.
     */
    double shootBiomassAboveClippingHeight;

    /** @brief Total belowground root biomass (g ODM). */
    double rootBiomass;

    /**
     * @brief Accumulated biomass in the seed/recruitment pool (g ODM).
     * Grows via NPP allocation; seeds are drawn from this pool by the
     * recruitment module.
     */
    double recruitmentBiomass;

    /**
     * @brief Daily root exudate biomass produced (g ODM d⁻¹).
     */
    double exudationBiomass;

    /** @brief Total plant biomass = shoot + root (g ODM). */
    double plantBiomass;

    // =========================================================================
    // Carbon pools (g C per plant)
    // =========================================================================

    /** @brief Carbon in green shoot biomass (g C). */
    double shootCarbonGreenLeaves;

    /** @brief Carbon in brown shoot biomass (g C). */
    double shootCarbonBrownLeaves;

    /** @brief Total carbon in shoot biomass (g C). */
    double shootCarbon;

    /** @brief Carbon in root biomass (g C). */
    double rootCarbon;

    /** @brief Carbon in recruitment (seed) biomass (g C). */
    double recruitmentCarbon;

    /** @brief Carbon in exudate biomass (g C). */
    double exudationCarbon;

    /** @brief Total carbon in shoot + root biomass (g C). */
    double plantCarbon;

    // =========================================================================
    // Nitrogen pools (g N per plant)
    // =========================================================================

    /** @brief Nitrogen in green shoot biomass (g N). */
    double shootNitrogenGreenLeaves;

    /** @brief Nitrogen in brown shoot biomass (g N). */
    double shootNitrogenBrownLeaves;

    /** @brief Total nitrogen in shoot biomass (g N). */
    double shootNitrogen;

    /** @brief Nitrogen in root biomass (g N). */
    double rootNitrogen;

    /** @brief Nitrogen in recruitment (seed) biomass (g N). */
    double recruitmentNitrogen;

    /** @brief Nitrogen in exudate biomass (g N). */
    double exudationNitrogen;

    /** @brief Total nitrogen in shoot + root biomass (g N). */
    double plantNitrogen;

    // =========================================================================
    // Mortality
    // =========================================================================

    // =========================================================================
    // Light climate
    // =========================================================================

    /**
     * @brief Extinction-weighted cumulative community LAI above the plant's top
     *        layer (dimensionless).
     */
    double cumulativeOvertoppingCommunityLAI;

    /**
     * @brief Photosynthetically active radiation available to this plant after
     *        canopy attenuation (µmol photons m⁻² s⁻¹).
     */
    double availableRadiation;

    /**
     * @brief Dimensionless shading indicator (0–1): ratio of available to
     *        full-sun radiation.
     */
    double shadingIndicator;

    // =========================================================================
    // Carbon-balance fluxes (g ODM d⁻¹ per plant unless noted)
    // =========================================================================

    /** @brief Gross primary productivity (g ODM d⁻¹). */
    double gpp;

    /** @brief Net primary productivity (g ODM d⁻¹). */
    double npp;

    /**
     * @brief Carry-over buffer for negative NPP (g ODM).
     * When GPP < maintenance respiration, the deficit is stored here and
     * deducted from NPP in the next time step.
     */
    double nppBuffer;

    /** @brief Total respiration = maintenance + growth respiration (g ODM d⁻¹). */
    double totalRespiration;

    /** @brief Growth respiration (g ODM d⁻¹). */
    double growthRespiration;

    /** @brief Maintenance respiration (g ODM d⁻¹). */
    double maintenanceRespiration;

    /**
     * @brief Temperature-based scaling factor for maintenance respiration (0–1).
     */
    double airTemperatureEffectOnRespiration;

    /**
     * @brief Temperature-based reduction factor for GPP (0–1).
     */
    double airTemperatureEffectOnGpp;

    // =========================================================================
    // NPP allocation fractions (dimensionless, sum to 1)
    // =========================================================================

    /** @brief Fraction of NPP allocated to shoot growth (0–1). */
    double nppAllocationShoot;

    /** @brief Fraction of NPP allocated to root growth (0–1). */
    double nppAllocationRoot;

    /** @brief Fraction of NPP allocated to seed production (0–1; 0 for immature plants). */
    double nppAllocationRecruitment;

    /** @brief Fraction of NPP allocated to root exudates (0–1). */
    double nppAllocationExudation;

    // =========================================================================
    // Soil water demand and uptake
    // =========================================================================

    /**
     * @brief GPP reduction factor due to soil water stress (0–1).
     * 1 = no limitation; < 1 = water-limited.
     */
    double limitingFactorGppWater;

    /** @brief Intermediate water-limitation factor based on potential evapotranspiration (reserved). */
    double limitingFactorGppPET;

    /** @brief Intermediate water-limitation factor based on permanent wilting point (reserved). */
    double limitingFactorGppPWP;

    /** @brief Combined total water-limitation factor on GPP (reserved). */
    double limitingFactorGppTotal;

    /** @brief Total daily soil water demand of this plant (mm d⁻¹). */
    double soilWaterDemand;

    /** @brief Total daily soil water uptake of this plant (mm d⁻¹). */
    double soilWaterUptake;

    /**
     * @brief Per-layer soil water demand (mm d⁻¹ per layer).
     */
    std::vector<double> soilWaterDemandPerSoilLayer;

    /**
     * @brief Per-layer soil water uptake (mm d⁻¹ per layer).
     */
    std::vector<double> soilWaterUptakePerSoilLayer;

    // =========================================================================
    // Soil nitrogen demand and uptake
    // =========================================================================

    /**
     * @brief Daily nitrogen taken up via rhizobial symbiosis (g N d⁻¹).
     */
    double rhizobiaNitrogenUptake;

    /**
     * @brief Scaling factor for rhizobial nitrogen fixation efficiency (0–1).
     * Modulates how much of the theoretical fixation capacity is realised.
     */
    double limitingFactorSymbiosisRhizobia;

    /**
     * @brief Internal nitrogen surplus accumulated from leaf-senescence
     *        retranslocation (g N).
     */
    double nitrogenSurplus;

    /** @brief Daily nitrogen demand for shoot growth (g N d⁻¹). */
    double nitrogenDemandForGrowthOfShoot;

    /** @brief Daily nitrogen demand for root growth (g N d⁻¹). */
    double nitrogenDemandForGrowthOfRoot;

    /** @brief Daily nitrogen demand for seed production (g N d⁻¹). */
    double nitrogenDemandForReproduction;

    /** @brief Daily nitrogen demand for exudate production (g N d⁻¹). */
    double nitrogenDemandForExudation;

    /** @brief Total daily nitrogen demand of this plant across all pools (g N d⁻¹). */
    double totalPlantNitrogenDemand;

    /** @brief Fraction of total growth N demand attributed to shoot growth (0–1). */
    double nitrogenDemandGrowthFractionShoot;

    /** @brief Fraction of total growth N demand attributed to root growth (0–1). */
    double nitrogenDemandGrowthFractionRoot;

    /** @brief Fraction of total growth N demand attributed to seed production (0–1). */
    double nitrogenDemandGrowthFractionReproduction;

    /** @brief Fraction of total growth N demand attributed to exudation (0–1). */
    double nitrogenDemandGrowthFractionExudation;

    /** @brief Total daily nitrogen demand met from soil mineral pools (g N d⁻¹). */
    double totalSoilNitrogenDemand;

    /**
     * @brief Per-layer soil nitrogen demand (g N d⁻¹ per layer).
     */
    std::vector<double> soilNitrogenDemandPerSoilLayer;

    /** @brief Total daily nitrogen uptake from soil mineral pools (g N d⁻¹). */
    double totalSoilNitrogenUptake;

    /**
     * @brief Per-layer soil nitrogen uptake (g N d⁻¹ per layer).
     */
    std::vector<double> soilNitrogenUptakePerSoilLayer;

    /** @brief Nitrogen uptake allocated to green shoot growth (g N d⁻¹). */
    double shootNitrogenUptakeForGreenLeaves;

    /** @brief Nitrogen uptake allocated to root growth (g N d⁻¹). */
    double rootNitrogenUptake;

    /** @brief Nitrogen uptake allocated to seed production (g N d⁻¹). */
    double recruitmentNitrogenUptake;

    /** @brief Nitrogen uptake allocated to exudate production (g N d⁻¹). */
    double exudationNitrogenUptake;

    /**
     * @brief NPP reduction factor due to soil nitrogen limitation (0–1).
     * 1 = no limitation; < 1 = N-limited.
     */
    double limitingFactorNppNitrogen;
};
