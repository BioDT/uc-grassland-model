#pragma once
#include "../utils/utils.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

/**
 * @brief Represents the parameters for the simulation.
 *
 * The `PARAMETER` class encapsulates various parameters that control the
 * simulation, including simulation settings, configuration
 * parameters, and plant traits.
 */
class PARAMETER
{
public:
    PARAMETER();
    ~PARAMETER();

    // **** parameters used for the simulation but not listed in the input files **** //
    int day;                     /// Current simulation day.
    int referenceJulianDayStart; /// Start of the reference Julian day for simulation.
    int referenceJulianDayEnd;   /// End of the reference Julian day for simulation.
    int simulationTimeInDays;    /// Total time of the simulation in days.

    // **** parameters of the configuration file **** //
    /// Names of configuration parameters.
    std::vector<std::string> configParameterNames =
        {"deimsID", "latitude", "longitude", "lastYear", "firstYear",
         "weatherFile", "soilFile", "managementFile", "plantTraitsFile", "processSetupFile",
         "communityOutputFile", "pftOutputFile", "plantCohortOutputFile", "soilCarbonOutputFile",
         "soilNitrogenOutputFile", "soilWaterOutputFile", "soilResourcesPerSoilLayerOutputFile", "outputWritingDatesFile", "clippingHeightOfBiomassMeasurement",
         "randomNumberGeneratorSeed"};

    std::string deimsID;
    std::string latitude;
    std::string longitude;
    int lastYear;
    int firstYear;
    std::string weatherFile;
    std::string soilFile;
    std::string managementFile;
    std::string plantTraitsFile;
    std::string processSetupFile;
    std::string soilParametersFile;

    bool communityOutputFile;
    bool pftOutputFile;
    bool plantCohortOutputFile;
    bool soilCarbonOutputFile;
    bool soilNitrogenOutputFile;
    bool soilWaterOutputFile;
    bool soilResourcesPerSoilLayerOutputFile;
    std::string outputWritingDatesFile;
    double clippingHeightOfBiomassMeasurement;
    unsigned int randomNumberGeneratorSeed;

    // **** parameters of the plant traits file **** //
    // vector needed to have names as string-keywords for searching parameters in plant traits file
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
         "h2L", "h2H", "crowdingCalculationFromPlantTopLayer", "minLayerReductionFactorFromAverage", "disableRunoff"};

    int pftCount;
    std::vector<double> maximumPlantHeight;
    std::vector<double> plantHeightToWidthRatio;
    std::vector<double> plantShootCorrectionFactor;
    std::vector<double> plantShootRootRatio;
    std::vector<double> plantRootDepthParamIntercept;
    std::vector<double> plantRootDepthParamExponent;
    std::vector<double> plantSpecificLeafArea;
    std::vector<double> plantShootOverlapFactors;
    bool crowdingMortalityActivated;
    double brownBiomassFractionFalling;
    std::vector<double> rootLifeSpan;
    std::vector<double> leafLifeSpan;
    std::vector<std::string> plantLifeSpan;
    std::vector<double> plantMortalityProbability;
    std::vector<double> seedlingMortalityProbability;
    std::vector<int> seedGerminationTimes;
    std::vector<double> seedGerminationRates;
    bool seedsFromMaturePlantsActivated;
    std::vector<double> seedMasses;
    std::vector<double> maturityAges;
    std::vector<double> maturityHeights;
    bool externalSeedInfluxActivated;
    std::vector<int> externalSeedInfluxNumber;
    int dayOfExternalSeedInfluxStart; // DATE transferred to day
    bool communityShadingInGppCalculation;
    std::vector<double> maximumGrossLeafPhotosynthesisRate;
    std::vector<double> initialSlopeOfLightResponseCurve;
    std::vector<double> lightExtinctionCoefficients;
    double growthRespirationFraction;
    double maintenanceRespirationRate;
    std::vector<double> plantNppAllocationGrowth;
    std::vector<double> plantNppAllocationExudation;
    bool useStaticShootRootAllocationRates;
    std::vector<double> plantCNRatioGreenLeaves;
    std::vector<double> plantCNRatioBrownLeaves;
    std::vector<double> plantCNRatioRoots;
    std::vector<double> plantCNRatioSeeds;
    std::vector<double> plantCNRatioExudates;
    std::vector<double> symbioticNitrogenFixationFraction;
    double rhizobiaExchangeRateCToN;
    std::vector<double> plantWaterUseEfficiency;
    std::string plantGppReductionBySoilWaterApproach;
    std::vector<double> lowerSoilWaterFractionForPlantGppReduction;
    std::vector<double> lowerSoilWaterContentForPlantGppReduction;
    std::vector<double> upperSoilWaterContentForPlantGppReduction;

    double plantResponseToTemperatureQ10Base;
    double plantResponseToTemperatureQ10Reference;

    /* parameters relevant for coupling */
    double h2L;
    double h2H;
    int crowdingCalculationFromPlantTopLayer;
    int minLayerReductionFactorFromAverage;
    int disableRunoff;

    // **** parameters of the process setup file **** //
    // vector needed to have names as string-keywords for searching parameters in process setup file
    std::vector<std::string> processSetupParameterNames =
        {"useInternalSoilModule", "useExternalSoilModule_BODIUM", "useExternalSoilModule_selfCoupled_getVariables", "useInternalSoilModule_selfCoupled_setVariables",
         "stochasticSimulation"};

    bool useInternalSoilModule;
    bool useExternalSoilModule_BODIUM;
    bool useExternalSoilModule_selfCoupled_getVariables;
    bool useInternalSoilModule_selfCoupled_setVariables;
    bool stochasticSimulation;

    /* parameters derived from other input parameters in the code (not listed in the input files) */
    // these are derived from the soilFile
    // they cannot be listed as soil class parameters (due to circular dependency issues)
    double soilDepth;
    int numberOfSoilLayers;
    std::vector<double> soilLayerWidth;
};
