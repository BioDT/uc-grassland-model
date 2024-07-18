#pragma once
#include "../utils/utils.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

class PARAMETER
{
public:
   PARAMETER();
   ~PARAMETER();

   // **** parameters used for the simulation but not listed in the input files **** //
   int numberOfDaysToSimulate;
   int day;
   int referenceJulianDayStart;
   int referenceJulianDayEnd;

   // **** parameters of the configuration file **** //
   // vector needed to have names as string-keywords for searching parameters in configuration-file
   std::vector<std::string> configParameterNames =
       {"deimsID", "latitude", "longitude", "lastYear", "firstYear",
        "weatherFile", "soilFile", "managementFile", "speciesFile",
        "outputFile", "outputWritingDatesFile", /*"clippingHeightForBiomassCalibration",*/
        "randomNumberGeneratorSeed"};

   std::string deimsID;
   float latitude;
   float longitude;
   int lastYear;
   int firstYear;
   std::string weatherFile;
   std::string soilFile;
   std::string managementFile;
   std::string speciesFile;
   bool outputFile;
   std::string outputWritingDatesFile;
   // float clippingHeightForBiomassCalibration;
   int randomNumberGeneratorSeed;

   // **** parameters of the species file **** //
   // vector needed to have names as string-keywords for searching parameters in species file
   std::vector<std::string> speciesParameterNames =
       {"numberOfSpecies", "maximumPlantHeight", "plantHeightToWidthRatio", "plantShootCorrectionFactor", "plantShootRootRatio",
        "plantRootDepthParamIntercept", "plantRootDepthParamExponent", "plantSpecificLeafArea", "plantShootOverlapFactors",
        "crowdingMortalityActivated", "brownBiomassFractionFalling", "rootLifeSpan", "leafLifeSpan", "plantLifeSpan", "plantMortalityRates",
        "seedlingMortalityRates", "seedGerminationTimes", "seedGerminationRates",
        "plantSeedProductionActivated", "seedMasses", "maturityAges", "externalSeedInfluxActivated", "externalSeedInfluxNumber", "dayOfExternalSeedInfluxStart",
        "maximumGrossLeafPhotosynthesisRate", "initialSlopeOfLightResponseCurve", "lightExtinctionCoefficients", "growthRespirationFraction",
        "maintenanceRespirationRate", "plantNPPAllocationToPlantGrowth", "plantCNRatioGreen", "plantCNRatioBrown", "nitrogenFixationAbility",
        "plantCostRhizobiaSymbiosis", "plantWaterUseEfficiency", "plantMinimalSoilWaterForGPPReduction", "plantMaximalSoilWaterForGPPReduction",
        "plantResponseToTemperatureQ10Base", "plantResponseToTemperatureQ10Reference"};

   int numberOfSpecies;
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
   bool externalSeedInfluxActivated;
   std::vector<int> externalSeedInfluxNumber;
   int dayOfExternalSeedInfluxStart; // DATE transferred to day
   std::vector<double> maximumGrossLeafPhotosynthesisRate;
   std::vector<double> initialSlopeOfLightResponseCurve;
   std::vector<double> lightExtinctionCoefficients;
   double growthRespirationFraction;
   double maintenanceRespirationRate;
   std::vector<double> plantNPPAllocationToPlantGrowth;
   std::vector<double> plantCNRatioGreen;
   std::vector<double> plantCNRatioBrown;
   std::vector<bool> nitrogenFixationAbility;
   double plantCostRhizobiaSymbiosis;
   std::vector<double> plantWaterUseEfficiency;
   std::vector<double> plantMinimalSoilWaterForGPPReduction;
   std::vector<double> plantMaximalSoilWaterForGPPReduction;
   double plantResponseToTemperatureQ10Base;
   double plantResponseToTemperatureQ10Reference;
};
