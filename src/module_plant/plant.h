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
    * @param height The initial height of the plant in meters.
    * @param amount The number of plants in the cohort.
    */
   PLANT(PARAMETER parameter, ALLOMETRY allometry, int pft, double height, int amount) : pft(pft), height(height), amount(amount)
   {
      isAdult = false;
      age = 0;
      isRegrowing = false;

      width = allometry.widthFromHeightHwr(height, parameter.plantHeightToWidthRatio[pft]);
      coveredArea = allometry.areaFromWidth(width); //*

      // plant biomass pools
      shootBiomassGreenLeaves = allometry.biomassFromHeightWidthForm(height, width, parameter.plantShootCorrectionFactor[pft]);
      shootBiomassBrownLeaves = 0.0;
      shootBiomass = shootBiomassGreenLeaves + shootBiomassBrownLeaves;
      rootBiomass = shootBiomass / parameter.plantShootRootRatio[pft];
      recruitmentBiomass = 0.0;                                           //*
      shootBiomassFractionGreen = shootBiomassGreenLeaves / shootBiomass; // initially: 1.0
      shootBiomassFractionBrown = shootBiomassBrownLeaves / shootBiomass; // initially: 0.0
      // shootBiomassAboveClippingHeight = 0.0;

      // carbon content of plant pools
      shootCarbonGreenLeaves = shootBiomassGreenLeaves * carbonContentOdm;
      shootCarbonBrownLeaves = shootBiomassBrownLeaves * carbonContentOdm;
      rootCarbon = rootBiomass * carbonContentOdm;
      recruitmentCarbon = recruitmentBiomass * carbonContentOdm;

      // nitrogen content of plant pools
      shootNitrogenGreenLeaves = shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
      shootNitrogenBrownLeaves = shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
      rootNitrogen = rootCarbon / parameter.plantCNRatioRoots[pft];
      recruitmentNitrogen = recruitmentCarbon / parameter.plantCNRatioSeeds[pft];

      // root architecture
      rootingDepth = parameter.plantRootDepthParamIntercept[pft] * pow((parameter.plantShootRootRatio[pft] / parameter.plantShootCorrectionFactor[pft]), parameter.plantRootDepthParamExponent[pft]) * pow((double)rootBiomass, (double)parameter.plantRootDepthParamExponent[pft]);
      numberOfSoilLayersRooting = 1;

      // leaf area and structure
      laiGreen = allometry.laiFromBiomassAreaSla(shootBiomassGreenLeaves, coveredArea, parameter.plantSpecificLeafArea[pft]);
      laiBrown = 0.0;
      lai = laiGreen + laiBrown; //*

      annualMortality = 0.0;
      // minLayerReductionFactor = 1.0;

      cumulativeOvertoppingCommunityLAI = 0.0; //*
      availableRadiation = 0.0;                //*
      shadingIndicator = -1;                   //*

      gpp = 0.0;
      npp = 0.0;
      nppBuffer = 0.0;
      totalRespiration = 0.0;
      growthRespiration = 0.0;
      maintenanceRespiration = 0.0;

      nppAllocationShoot = parameter.plantShootRootRatio[pft] / (1 + parameter.plantShootRootRatio[pft]); // init with full allocation to shoot and root
      nppAllocationRoot = 1 / (1 + parameter.plantShootRootRatio[pft]);
      nppAllocationRecruitment = 0.0;

      limitingFactorGppWater = 1.0;
      limitingFactorGppNitrogen = 1.0;
      limitingFactorSymbiosisRhizobia = 1.0;

      nitrogenSurplus = 0.0;
      shootNitrogenUptake = 0.0;
      rootNitrogenUptake = 0.0;
      recruitmentNitrogenUptake = 0.0;
   }
   ~PLANT();

   int amount;       /// * Number of plants in cohort with equal properties listed below (representative for ONE plant)
   short pft;        /// * Number of plant functional types (PFT)
   bool isAdult;     /// Status whether seedling (0) or adult (1)
   double age;       /// Plant age (in days)
   bool isRegrowing; /// Indicates if the shoot has recently been mowed (and plant height has been reduced)

   double rootingDepth;           /// Rooting depth (in cm)
   double coveredArea;            /// Ground area covered by plant (in square cm)
   double width;                  /// Plant width (in cm)
   double height;                 /// Plant height (in cm)
   int numberOfSoilLayersRooting; /// Number of soil layer a plant is rooting down to

   double shootBiomass;                    /// Aboveground shoot biomass (in gODM)
   double shootBiomassGreenLeaves;         /// Green photosynthetic active biomass of plant shoot (in gODM)
   double shootBiomassBrownLeaves;         /// Senescent photosynthetic inactive brown biomass of plant shoot (in gODM)
   double shootBiomassFractionGreen;       /// Fraction of green biomass according to total shoot biomass
   double shootBiomassFractionBrown;       /// Fraction of brown biomass according to total shoot biomass
   double shootBiomassAboveClippingHeight; /// Shoot biomass above the clipping height of field measurements (in gODM)
   double rootBiomass;                     /// Belowground root biomass (in gODM)
   double recruitmentBiomass;              /// Recruitment biomass (in gODM)

   double shootCarbonGreenLeaves; /// Carbon content in biomass of green plant shoot (in gC)
   double shootCarbonBrownLeaves; /// Carbon content in biomass of senescent brown plant shoot (in gC)
   double rootCarbon;             /// Carbon content in belowground root biomass (in gC)
   double recruitmentCarbon;      /// Carbon content in recruitment biomass per plant (in gC)

   double shootNitrogenGreenLeaves; /// Nitrogen content in biomass of green plant shoot (in gN)
   double shootNitrogenBrownLeaves; /// Nitrogen content in biomass of senescent brown plant shoot (in gN)
   double rootNitrogen;             /// Nitrogen content in belowground root biomass (in gN)
   double recruitmentNitrogen;      /// Nitrogen content in recruitment biomass per plant (in gN)

   double laiGreen; /// Green leaf area index of a plant (in square cm per square cm)
   double laiBrown; /// Senescent leaf area index of a plant (in square cm per square cm)
   double lai;      /// * Leaf area index of plant (in square cm per square cm)

   // double minLayerReductionFactor; /// Minimum layer reduction fraction TODO: check if we want to keep that
   double annualMortality; /// Annual probability for a plant to die

   double cumulativeOvertoppingCommunityLAI; /// * Cumulative leaf area index above a plant accounting for light extinction (in square cm per square cm)
   double availableRadiation;                /// * Incoming radiation [micromol(photon) per square m per second]
   double shadingIndicator;                  /// * fraction of sunlight reaching the plant in relation to full sun light (-)

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

   double limitingFactorGppWater;          /// Limitation factor addressing the impact of soil water deficit and surplus on GPP
   double limitingFactorGppNitrogen;       /// Limitation factor addressing the impact of soil nitrogen deficits on NPP // TODO: rename
   double limitingFactorSymbiosisRhizobia; /// ...

   double nitrogenSurplus;           /// Nitrogen surplus provided by leaf senescence and nitrogen retranslocation to green leaves (in gN)
   double shootNitrogenUptake;       /// Uptake of soil nitrogen at plant shoot (in gN per day)
   double rootNitrogenUptake;        /// Uptake of soil nitrogen at plant root (in gN per day)
   double recruitmentNitrogenUptake; /// Uptake of soil nitrogen for seed production (recruitment) (in gN per day)
};