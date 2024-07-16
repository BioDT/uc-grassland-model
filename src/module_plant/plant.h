#pragma once
#include "allometry.h"
#include "../module_parameter/parameter.h"
#include "../module_init/constants.h"
#include "../utils/utils.h"
#include <iostream>

class PLANT
{
public:
    PLANT();
    PLANT(PARAMETER param, ALLOMETRY allo, int pft, double height, int seedlings) : pft(pft), height(height), N(seedlings)
    {
        isAdult = false;     // status whether seedling (0) or adult (1)
        age = 0;             // age [days]
        isRegrowing = false; // has shoot recently been mowed? If true, height-width-ratio is not original anymore and shoot height growth is favored instead of height and width growth

        width = allo.widthFromHeightHwr(height, param.plantHeightToWidthRatio[pft]);
        coveredArea = allo.areaFromWidth(width);

        shootBiomassGreen = allo.biomassFromHeightWidthForm(height, width, param.plantShootCorrectionFactor[pft]);
        shootBiomass = shootBiomassGreen;
        shootBiomassBrown = 0.0;
        shootBiomassFractionGreen = 1.0;
        shootBiomassFractionBrown = 0.0;
        shootBiomassAboveClippingHeight = 0.0;
        rootBiomass = shootBiomass / param.plantShootRootRatio[pft];
        recruitmentBiomass = 0.0;

        rootingDepth = param.plantRootDepthParamIntercept[pft] * pow((param.plantShootRootRatio[pft] / param.plantShootCorrectionFactor[pft]), param.plantRootDepthParamExponent[pft]) * pow((double)rootBiomass, (double)param.plantRootDepthParamExponent[pft]);
        numberOfSoilLayersRooting = 1;

        shootNitrogenContent = (shootBiomass * CONVERT_ODM_TO_C) / param.plantCNRatioGreen[pft];
        rootNitrogenContent = (rootBiomass * CONVERT_ODM_TO_C) / param.plantCNRatioBrown[pft];
        recruitmentNitrogenContent = 0.0;

        laiGreen = allo.laiFromBiomassAreaSla(shootBiomassGreen, coveredArea, param.plantSpecificLeafArea[pft]);
        laiBrown = 0.0;
        lai = laiGreen + laiBrown;

        annualMortality = 0.0;
        minLayerReductionFactor = 1.0;

        laiAbove = 0.0;
        incomingRadiation = 0.0;

        gpp = 0.0;
        npp = 0.0;
        nppBuffer = 0.0;
        totalRespiration = 0.0;
        growthRespiration = 0.0;
        maintenanceRespiration = 0.0;

        nppAllocationToShoot = param.plantShootRootRatio[pft] / (1 + param.plantShootRootRatio[pft]); // Note: init with full allocation to shoot and root
        nppAllocationToRoot = (1 / (1 + param.plantShootRootRatio[pft]));
        nppAllocationToRecruitment = 0.0;

        limitingFactorGppWater = 1.0;
        limitingFactorGppNitrogen = 1.0;
        limitingFactorSymbiosisRhizobia = 1.0;

        nitrogenSurplus = 0.0;
        shootNitrogenUptake = 0.0;
        rootNitrogenUptake = 0.0;
        recruitmentNitrogenUptake = 0.0;
    }
    ~PLANT();

    int N;            // Number of plants in cohort with equal properties listed below (representative for ONE plant)
    short pft;        // number of plant functional type (PFT)
    bool isAdult;     // status whether seedling (0) or adult (1)
    double age;       // age [days]
    bool isRegrowing; // Is shoot recently mowed? So that h:w ratio is not correct or shoot cannot grow further

    double rootingDepth;           // rooting depth [m]
    double coveredArea;            // ground area covered by plant [m^2]
    double width;                  // plant width [m]
    double height;                 // plant height [m]
    int numberOfSoilLayersRooting; // number of soil layer a plant is rooting in

    double shootBiomass;                    // aboveground shoot biomass [g(organic dry matter) = 10^6 g (odm)]
    double shootBiomassGreen;               // green photosynthetic active biomass of plant shoot [g (ODM)]
    double shootBiomassBrown;               // senescent photosynthetic inactive brown biomass of plant shoot [g (ODM)]
    double shootBiomassFractionGreen;       // fraction of green biomass according to total shoot biomass
    double shootBiomassFractionBrown;       // fraction of brown biomass according to total shoot biomass
    double shootBiomassAboveClippingHeight; // shoot biomass above the clipping in field measurements (for calibration)
    double rootBiomass;                     // belowground root biomass
    double recruitmentBiomass;              // recruitment pool of biomass per plant

    double shootNitrogenContent;
    double rootNitrogenContent;
    double recruitmentNitrogenContent;

    double laiGreen; // green leaf area index of a grass individual
    double laiBrown; // senescent leaf area index of a grass individual
    double lai;      // leaf area index of single plant [m^2 m^-2]

    double minLayerReductionFactor; // minimum layer reduction fraction [-]
    double annualMortality;         // probability for a plant to die in a year [-]

    double laiAbove;          // cumulative leaf area index above plant [m^2 m^-2] including light extinction coefficient
    double incomingRadiation; // incoming radiation [micromol (photon) m^-2 s^-1]
    double gpp;               // gpp [g_odm / d]
    double npp;
    double nppBuffer;              // buffer of gpp [g_odm / d] if npp <0
    double totalRespiration;       // total respiration [g_odm / d]
    double growthRespiration;      // growth respiration [g_odm / d]
    double maintenanceRespiration; // maintanance respiration [g_odm / d]
    double airTemperatureEffectOnRespiration;
    double airTemperatureEffectOnGPP;

    double nppAllocationToShoot;
    double nppAllocationToRoot;
    double nppAllocationToRecruitment;

    double limitingFactorGppWater;          //  water reduction factor
    double limitingFactorGppNitrogen;       //  reduction factor for nitrogen
    double limitingFactorSymbiosisRhizobia; // changed from (1 - nppSymbiosisCostToRhizobia)

    // individual reduction factor for costs (fraction of NPP) invested in rhizobia to get fixed Nitrogen
    double nitrogenSurplus;
    double shootNitrogenUptake;
    double rootNitrogenUptake;
    double recruitmentNitrogenUptake;
};