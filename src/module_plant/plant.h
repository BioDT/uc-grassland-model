#pragma once
#include "allometry.h"
#include "../module_parameter/parameter.h"
#include "../module_init/constants.h"
#include "../utils/utils.h"
#include <iostream>

/**
 * @brief Represents a plant in the simulation.
 *
 * The `PLANT` class encapsulates the attributes and behaviors of a plant,
 * including its biological and ecological properties. This class is used
 * to manage individual plant instances in the community.
 */
class PLANT
{
public:
    PLANT();

    /**
     * @brief Constructor for the PLANT class.
     *
     * Initializes a new instance of the PLANT class. This constructor sets
     * up the plant's initial state and attributes based on the provided
     * parameters.
     *
     * @param parameter The simulation parameters that define plant properties.
     * @param allometry The allometry object that provides methods for calculating
     *              plant dimensions and biomass based on allometric relationships.
     * @param pft The plant functional type identifier.
     * @param plantBiomass The initial biomass of the plant in g(ODM).
     * @param amount The number of plants in the cohort.
     */
    PLANT(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, int pft, int amount) : pft(pft), amount(amount)
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
        shootCarbonGreenLeaves = shootBiomassGreenLeaves * carbonContentOdm;
        shootCarbonBrownLeaves = shootBiomassBrownLeaves * carbonContentOdm;
        shootCarbon = shootCarbonBrownLeaves + shootCarbonGreenLeaves;
        rootCarbon = rootBiomass * carbonContentOdm;
        recruitmentCarbon = recruitmentBiomass * carbonContentOdm;
        exudationCarbon = exudationBiomass * carbonContentOdm;
        plantCarbon = plantBiomass * carbonContentOdm;

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

        /* leaf area and structure */
        laiGreen = allometry.laiFromShootBiomassAreaSla(utils, shootBiomassGreenLeaves, coveredArea, parameter.plantSpecificLeafArea[pft]);
        laiBrown = 0.0;
        lai = laiGreen + laiBrown;

        /* mortality */
        annualMortality = 0.0;
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
    ~PLANT();

    int amount; /// Number of plants in cohort with equal properties listed below (representative for ONE plant)
    short pft;  /// Number of plant functional types (PFT)
    double age; /// Plant age (in days)

    double coveredArea;            /// Ground area covered by plant (in square cm)
    double width;                  /// Plant width (in cm)
    double height;                 /// Plant height (in cm)
    double laiGreen;               /// Green leaf area index of a plant (in square cm per square cm)
    double laiBrown;               /// Senescent leaf area index of a plant (in square cm per square cm)
    double lai;                    /// Leaf area index of plant (in square cm per square cm)
    double rootingDepth;           /// Rooting depth (in cm)
    int numberOfSoilLayersRooting; /// Number of soil layer a plant is rooting down to

    double shootBiomass;                    /// Aboveground shoot biomass (in gODM)
    double shootBiomassGreenLeaves;         /// Green photosynthetic active biomass of plant shoot (in gODM)
    double shootBiomassBrownLeaves;         /// Senescent photosynthetic inactive brown biomass of plant shoot (in gODM)
    double shootBiomassAboveClippingHeight; /// Shoot biomass above the clipping height of field measurements (in gODM)
    double rootBiomass;                     /// Belowground root biomass (in gODM)
    double recruitmentBiomass;              /// Recruitment biomass (in gODM)
    double exudationBiomass;                /// Exudation biomass (in gODM)
    double plantBiomass;                    /// Total plant biomass of shoot and root (in gODM)

    double shootCarbonGreenLeaves; /// Carbon content in biomass of green plant shoot (in gC)
    double shootCarbonBrownLeaves; /// Carbon content in biomass of senescent brown plant shoot (in gC)
    double shootCarbon;            /// Carbon content in biomass of plant shoot (in gC)
    double rootCarbon;             /// Carbon content in belowground root biomass (in gC)
    double recruitmentCarbon;      /// Carbon content in recruitment biomass per plant (in gC)
    double exudationCarbon;        /// Carbon content in exudation biomass per plant (in gC)
    double plantCarbon;            /// Carbon content in biomass per plant of root and shoot (in gC)

    double shootNitrogenGreenLeaves; /// Nitrogen content in biomass of green plant shoot (in gN)
    double shootNitrogenBrownLeaves; /// Nitrogen content in biomass of senescent brown plant shoot (in gN)
    double shootNitrogen;            /// Nitrogen content in biomass of plant shoot (in gN)
    double rootNitrogen;             /// Nitrogen content in belowground root biomass (in gN)
    double recruitmentNitrogen;      /// Nitrogen content in recruitment biomass per plant (in gN)
    double exudationNitrogen;        /// Nitrogen content in exudation biomass per plant (in gN)
    double plantNitrogen;            /// Nitrogen content in biomass per plant of root and shoot (in gN)

    double annualMortality;                   /// Annual probability for a plant to die
    double cumulativeOvertoppingCommunityLAI; /// Cumulative leaf area index above a plant accounting for light extinction (in square cm per square cm)
    double availableRadiation;                /// Incoming radiation [micromol(photon) per square m per second]
    double shadingIndicator;                  /// Fraction of sunlight reaching the plant in relation to full sun light (-)

    double gpp;                               /// Gross primary productivity GPP (in gODM per day)
    double npp;                               /// Net primary productivity NPP (in gODM per day)
    double nppBuffer;                         /// Buffer of GPP (in gODM per d) if NPP < 0
    double totalRespiration;                  /// Total respiration (in gODM per day)
    double growthRespiration;                 /// Growth respiration (in gODM per day)
    double maintenanceRespiration;            /// Maintanance respiration (in gODM per day)
    double airTemperatureEffectOnRespiration; /// Effect of full-day air tempature on maintenance respiration
    double airTemperatureEffectOnGpp;         /// Effect of daytime air temperature on GPP

    double nppAllocationShoot;       /// Allocation rate of NPP to shoot growth
    double nppAllocationRoot;        /// Allocation rate of NPP to root growth
    double nppAllocationRecruitment; /// Allocation rate of NPP to seed production (recruitment biomass)
    double nppAllocationExudation;   /// Allocation rate of NPP to exudates

    double limitingFactorGppWater; /// Limitation factor addressing the impact of soil water deficit and surplus on GPP
    double limitingFactorGppPET;
    double limitingFactorGppPWP;
    double limitingFactorGppTotal;

    double soilWaterDemand;
    double soilWaterUptake;
    std::vector<double> soilWaterDemandPerSoilLayer;
    std::vector<double> soilWaterUptakePerSoilLayer;

    double rhizobiaNitrogenUptake;          /// Uptake of nitrogen through rhizobia symbiosis (in gN per day)
    double limitingFactorSymbiosisRhizobia; /// Limitation factor addressing the impact of rhizobia symbiosis efficiency on nitrogen fixation
    double nitrogenSurplus;                 /// Nitrogen surplus provided by leaf senescence and nitrogen retranslocation to green leaves (in gN)

    double nitrogenDemandForGrowthOfShoot; /// Nitrogen demand for shoot growth (in gN)
    double nitrogenDemandForGrowthOfRoot;  /// Nitrogen demand for root growth (in gN)
    double nitrogenDemandForReproduction;  /// Nitrogen demand for recruitment growth (in gN)
    double nitrogenDemandForExudation;     /// Nitrogen demand for exudation (in gN)
    double totalPlantNitrogenDemand;       /// Total nitrogen demand of plant (in gN)

    double nitrogenDemandGrowthFractionShoot;        /// Fraction of nitrogen demand for growth allocated to shoot growth
    double nitrogenDemandGrowthFractionRoot;         /// Fraction of nitrogen demand for growth allocated to root growth
    double nitrogenDemandGrowthFractionReproduction; /// Fraction of nitrogen demand for growth allocated to recruitment growth
    double nitrogenDemandGrowthFractionExudation;    /// Fraction of nitrogen demand for growth allocated to exudation

    double totalSoilNitrogenDemand;                     /// Total nitrogen demand of plant from soil (in gN)
    std::vector<double> soilNitrogenDemandPerSoilLayer; /// Nitrogen demand of plant from soil per soil layer (in gN)
    double totalSoilNitrogenUptake;                     /// Total nitrogen uptake of plant from soil (in gN)
    std::vector<double> soilNitrogenUptakePerSoilLayer; /// Uptake of soil nitrogen per soil layer (in gN per day)

    double shootNitrogenUptakeForGreenLeaves; /// Uptake of soil nitrogen at plant shoot (in gN per day)
    double rootNitrogenUptake;                /// Uptake of soil nitrogen at plant root (in gN per day)
    double recruitmentNitrogenUptake;         /// Uptake of soil nitrogen for seed production (recruitment) (in gN per day)
    double exudationNitrogenUptake;           /// Uptake of soil nitrogen for exudation (in gN per day)
    double limitingFactorNppNitrogen;         /// Limitation factor addressing the impact of soil nitrogen deficits on NPP
};