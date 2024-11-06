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
        "weatherFile", "soilFile", "managementFile", "plantTraitsFile",
        "outputFile", "outputWritingDatesFile", /*"clippingHeightForBiomassCalibration",*/
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
   bool outputFile;
   std::string outputWritingDatesFile;
   // float clippingHeightForBiomassCalibration;
   unsigned int randomNumberGeneratorSeed;

   // **** parameters of the plant traits file **** //
   // vector needed to have names as string-keywords for searching parameters in plant traits file
   std::vector<std::string> plantTraitsParameterNames =
       {"pftCount", "maximumPlantHeight", "plantHeightToWidthRatio", "plantShootCorrectionFactor", "plantShootRootRatio",
        "plantRootDepthParamIntercept", "plantRootDepthParamExponent", "plantSpecificLeafArea", "plantShootOverlapFactors",
        "crowdingMortalityActivated", "brownBiomassFractionFalling", "rootLifeSpan", "leafLifeSpan", "plantLifeSpan", "plantMortalityRates",
        "seedlingMortalityRates", "seedGerminationTimes", "seedGerminationRates",
        "plantSeedProductionActivated", "seedMasses", "maturityAges", "maturityHeights", "externalSeedInfluxActivated", "externalSeedInfluxNumber", "dayOfExternalSeedInfluxStart",
        "maximumGrossLeafPhotosynthesisRate", "initialSlopeOfLightResponseCurve", "lightExtinctionCoefficients", "growthRespirationFraction",
        "maintenanceRespirationRate", "plantNppAllocationGrowth", "plantCNRatioGreenLeaves", "plantCNRatioBrownLeaves", "plantCNRatioRoots", "plantCNRatioSeeds", "nitrogenFixationAbility",
        "plantCostRhizobiaSymbiosis", "plantWaterUseEfficiency", "plantMinimalSoilWaterForGppReduction", "plantMaximalSoilWaterForGppReduction",
        "plantResponseToTemperatureQ10Base", "plantResponseToTemperatureQ10Reference"};

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
   std::vector<double> plantMortalityRates;
   std::vector<double> seedlingMortalityRates;
   std::vector<int> seedGerminationTimes;
   std::vector<double> seedGerminationRates;
   bool plantSeedProductionActivated;
   std::vector<double> seedMasses;
   std::vector<double> maturityAges;
   std::vector<double> maturityHeights;
   bool externalSeedInfluxActivated;
   std::vector<int> externalSeedInfluxNumber;
   int dayOfExternalSeedInfluxStart; // DATE transferred to day
   std::vector<double> maximumGrossLeafPhotosynthesisRate;
   std::vector<double> initialSlopeOfLightResponseCurve;
   std::vector<double> lightExtinctionCoefficients;
   double growthRespirationFraction;
   double maintenanceRespirationRate;
   std::vector<double> plantNppAllocationGrowth;
   std::vector<double> plantCNRatioGreenLeaves;
   std::vector<double> plantCNRatioBrownLeaves;
   std::vector<double> plantCNRatioRoots;
   std::vector<double> plantCNRatioSeeds;
   std::vector<bool> nitrogenFixationAbility;
   double plantCostRhizobiaSymbiosis;
   std::vector<double> plantWaterUseEfficiency;
   std::vector<double> plantMinimalSoilWaterForGppReduction;
   std::vector<double> plantMaximalSoilWaterForGppReduction;
   double plantResponseToTemperatureQ10Base;
   double plantResponseToTemperatureQ10Reference;
};
