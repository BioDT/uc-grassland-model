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
   int simulationTimeInDays;

   // **** parameters of the configuration file **** //
   // vector needed to have names as string-keywords for searching parameters in configuration-file
   std::vector<std::string> configParameterNames =
       {"deimsID", "latitude", "longitude", "lastYear", "firstYear",
        "weatherFile", "soilFile", "managementFile", "plantTraitsFile",
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
   std::string plantTraitsFile;
   bool outputFile;
   std::string outputWritingDatesFile;
   // float clippingHeightForBiomassCalibration;
   int randomNumberGeneratorSeed;

   // **** parameters of the plant traits file **** //
   // vector needed to have names as string-keywords for searching parameters in plant traits file
   std::vector<std::string> plantTraitsParameterNames =
       {"pftCount", "maximumPlantHeight", "plantHeightToWidthRatio", "plantShootCorrectionFactor", "plantShootRootRatio",
        "plantRootDepthParamIntercept", "plantRootDepthParamExponent", "plantSpecificLeafArea", "plantShootOverlapFactors",
        "crowdingMortalityActivated", "brownBiomassFractionFalling", "rootLifeSpan", "leafLifeSpan", "plantLifeSpan", "plantMortalityRates",
        "seedlingMortalityRates", "seedGerminationTimes", "seedGerminationRates",
        "plantSeedProductionActivated", "seedMasses", "maturityAges", "externalSeedInfluxActivated", "externalSeedInfluxNumber", "dayOfExternalSeedInfluxStart",
        "maximumGrossLeafPhotosynthesisRate", "initialSlopeOfLightResponseCurve", "lightExtinctionCoefficients", "growthRespirationFraction",
        "maintenanceRespirationRate", "plantNppAllocationGrowth", "plantCNRatioGreen", "plantCNRatioBrown", "nitrogenFixationAbility",
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
   bool externalSeedInfluxActivated;
   std::vector<int> externalSeedInfluxNumber;
   int dayOfExternalSeedInfluxStart; // DATE transferred to day
   std::vector<double> maximumGrossLeafPhotosynthesisRate;
   std::vector<double> initialSlopeOfLightResponseCurve;
   std::vector<double> lightExtinctionCoefficients;
   double growthRespirationFraction;
   double maintenanceRespirationRate;
   std::vector<double> plantNppAllocationGrowth;
   std::vector<double> plantCNRatioGreen;
   std::vector<double> plantCNRatioBrown;
   std::vector<bool> nitrogenFixationAbility;
   double plantCostRhizobiaSymbiosis;
   std::vector<double> plantWaterUseEfficiency;
   std::vector<double> plantMinimalSoilWaterForGppReduction;
   std::vector<double> plantMaximalSoilWaterForGppReduction;
   double plantResponseToTemperatureQ10Base;
   double plantResponseToTemperatureQ10Reference;
};
