#pragma once
#include "../module_init/constants.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_weather/weather.h"
#include <vector>
#include <iostream>

class SOIL
{
public:
   SOIL();
   ~SOIL();

   /* soil input parameter */
   double siltContent;
   double sandContent;
   double clayContent;
   std::vector<double> permanentWiltingPoint;
   std::vector<double> fieldCapacity;
   std::vector<double> porosity;
   std::vector<double> saturatedHydraulicConductivity;

   // water content in pools
   double snowContent;
   std::vector<double> waterContent_soilWaterPoolPerLayer;

   double soilTemperature;

   // carbon content in pools
   double carbonContent_surfaceGreenLitterPool;
   double carbonContent_surfaceBrownLitterPool;
   double carbonContent_soilRootLitterPool;
   double carbonContent_soilSeedLitterPool;

   double carbonContent_soilMetabolicLitterPool;
   double carbonContent_surfaceMetabolicLitterPool;
   double carbonContent_soilStructuralLitterPool;
   double carbonContent_surfaceStructuralLitterPool;

   double carbonContent_soilPassivePool;
   double carbonContent_soilSlowPool;
   double carbonContent_soilActivePool;
   double carbonContent_soilMicrobesPool;

   // nitrogen content in pools
   double nitrogenContent_surfaceGreenLitterPool;
   double nitrogenContent_surfaceBrownLitterPool;
   double nitrogenContent_soilRootLitterPool;
   double nitrogenContent_soilSeedLitterPool;

   double nitrogenContent_soilMetabolicLitterPool;
   double nitrogenContent_surfaceMetabolicLitterPool;
   double nitrogenContent_soilStructuralLitterPool;
   double nitrogenContent_surfaceStructuralLitterPool;

   double nitrogenContent_soilPassivePool;
   double nitrogenContent_soilSlowPool;
   double nitrogenContent_soilActivePool;
   double nitrogenContent_soilMicrobesPool;

   std::vector<double> nitrogenContent_soilMineralPoolPerSoilLayer;

   // decisive carbon-nitrogen ratios for deciding on the decomposition of pools
   // and for determining nitrogen mineralization or immobilization during decomposition
   // calculated via regression functions (independent from actual CN ratios of each pool)
   double decisiveCNRatio_soilMicrobesPool_soilSlowPool;
   double decisiveCNRatio_soilPassivePool_soilActivePool;
   double decisiveCNRatio_soilSlowPool_soilActivePool;
   double decisiveCNRatio_soilSlowPool_soilPassivePool;
   double decisiveCNRatio_soilActivePool_soilSlowPool;
   double decisiveCNRatio_soilActivePool_soilPassivePool;
   double decisiveCNRatio_soilMetabolicLitterPool_soilActivePool;
   double decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool;

   // decisive CN ratios initialized only once in the init function
   double decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool; // TODO: dependent on litter pool content -> why not move to each day update?
   double decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool;
   double decisiveCNRatio_soilStructuralLitterPool_soilActivePool;
   double decisiveCNRatio_soilStructuralLitterPool_soilSlowPool;

   // lignin contents
   double ligninContent_surfaceStructuralLitterPool;
   double ligninContent_soilStructuralLitterPool;

   // carbon fluxes
   double carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool;     //*
   double carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool; //*
   double carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;  //*

   double carbonFlux_soilStructuralLitterPool_to_soilSlowPool;   //*
   double carbonFlux_soilStructuralLitterPool_to_soilActivePool; //*
   double carbonFlux_soilMetabolicLitterPool_to_soilActivePool;  //*

   double carbonFlux_soilActivePool_to_soilPassiveAndSlowPool; //*
   double carbonFlux_soilSlowPool_to_soilPassiveAndActivePool; //*

   double carbonFlux_soilMicrobesPool_to_soilSlowPool;  //*
   double carbonFlux_soilSlowPool_to_soilPassivePool;   //*
   double carbonFlux_soilSlowPool_to_soilActivePool;    //*
   double carbonFlux_soilActivePool_to_soilPassivePool; //*
   double carbonFlux_soilActivePool_to_soilSlowPool;    //*
   double carbonFlux_soilPassivePool_to_soilActivePool; //*

   // nitrogen fluxes
   double nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;     //*
   double nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool; //*
   double nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;  //*

   double nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;   //*
   double nitrogenFlux_soilStructuralLitterPool_to_soilActivePool; //*
   double nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;  //*

   double nitrogenFlux_soilMicrobesPool_to_soilSlowPool;  //*
   double nitrogenFlux_soilSlowPool_to_soilActivePool;    //*
   double nitrogenFlux_soilActivePool_to_soilSlowPool;    //*
   double nitrogenFlux_soilSlowPool_to_soilPassivePool;   //*
   double nitrogenFlux_soilPassivePool_to_soilActivePool; //*
   double nitrogenFlux_soilActivePool_to_soilPassivePool; //*

   // respiratory carbon fluxes
   double respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool;     //*
   double respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool; //*
   double respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;        //*
   double respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool;      //*
   double respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;  //*
   double respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;       //*
   double respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;                //*
   double respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool;        //*
   double respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool;        //*
   double respiration_decompositionCarbon_soilPassivePool_soilActivePool;               //*

   double respirationCarbon_surface_litter;
   double respirationCarbon_soil_litter;
   double respirationCarbon_litter;
   double respirationCarbon_soilpools;

   // respiratory nitrogen fluxes
   double respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool;     //*
   double respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool; //*
   double respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool;        //*
   double respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool;      //*
   double respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;  //*
   double respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;       //*
   double respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;                //*
   double respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;        //*
   double respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;        //*
   double respiration_decompositionNitrogen_soilPassivePool_soilActivePool;               //*

   // Nitrogen mineralization and loss
   double nitrogenGrossMineralization;
   double nitrogenNetMineralization;
   double nitrogenVolatilization;

   double nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool;     //*
   double nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool; //*
   double nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool;        //*
   double nitrogenFlow_soilStructuralLitterPool_to_soilActivePool;      //*
   double nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool;  //*
   double nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool;       //*

   double nitrogenFlow_soilMicrobesPool_to_soilSlowPool;  //*
   double nitrogenFlow_soilActivePool_to_soilPassivePool; //*
   double nitrogenFlow_soilActivePool_to_soilSlowPool;    //*
   double nitrogenFlow_soilSlowPool_to_soilPassivePool;   //*
   double nitrogenFlow_soilSlowPool_to_soilActivePool;    //*
   double nitrogenFlow_soilPassivePool_to_soilActivePool; //*

   double immobilize_surfaceStructuralLitterPool_to_soilSlowPool;     //*
   double immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool; //*
   double immobilize_soilStructuralLitterPool_to_soilSlowPool;        //*
   double immobilize_soilStructuralLitterPool_to_soilActivePool;      //*
   double immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool;  //*
   double immobilize_soilMetabolicLitterPool_to_soilActivePool;       //*

   double immobilize_soilMicrobesPool_to_soilSlowPool;  //*
   double immobilize_soilActivePool_to_soilPassivePool; //*
   double immobilize_soilActivePool_to_soilSlowPool;    //*
   double immobilize_soilSlowPool_to_soilPassivePool;   //*
   double immobilize_soilSlowPool_to_soilActivePool;    //*
   double immobilize_soilPassivePool_to_soilActivePool; //*

   double mineralize_surfaceStructuralLitterPool_to_soilSlowPool;     //*
   double mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool; //*
   double mineralize_soilStructuralLitterPool_to_soilSlowPool;        //*
   double mineralize_soilStructuralLitterPool_to_soilActivePool;      //*
   double mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;  //*
   double mineralize_soilMetabolicLitterPool_to_soilActivePool;       //*

   double mineralize_soilMicrobesPool_to_soilSlowPool;  //*
   double mineralize_soilActivePool_to_soilPassivePool; //*
   double mineralize_soilActivePool_to_soilSlowPool;    //*
   double mineralize_soilSlowPool_to_soilPassivePool;   //*
   double mineralize_soilSlowPool_to_soilActivePool;    //*
   double mineralize_soilPassivePool_to_soilActivePool; //*

   double carbonLeaching; //**
   double LeachingC;
   double NLeach;
   double LeachingN;

   double C_flux;
   double Nflux;
   double R_total;

   double decompositionFactor;
   double nitrogenFixationToSoil;

   void transferDyingPlantPartsToLitterPools(UTILS utils, PARAMETER parameter, int number, double biomass, std::string typeOfMaterial, int pft);
   void calculateSoilResourceDynamics(UTILS utils, PARAMETER parameter, WEATHER weather);

   void splitLitterFluxesToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather);
   void addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather, std::string type);
   double calculateDIRABS(UTILS utils, std::string type);
   double calculateLigninFraction(UTILS utils, WEATHER weather, PARAMETER parameter, std::string type);
   double calculateNitrogenFraction(UTILS utils, double dirabs, std::string type);
   double calculateFractionOfMetabolicLitter(UTILS utils, double fractionOfLignin, double ligninToNitrogenRatio, std::string type);
   double adjustLigninContentOfStructuralLitter(UTILS utils, double fractionOfLignin, double carbonAddedToStructuralLitter, double strlig, std::string type);
   void processLitterFluxes(UTILS utils, double dirabs, double cadds, double caddm, double eadds, double eaddm, std::string type);

   double calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, PARAMETER parameter);
   void doDecompositionFluxesInLitterAndSoilPools(UTILS utils);
   void startDecomposition(UTILS utils, double carbonPool, double factor, double lignin, std::string typeOfSoilPool);
   void calculateDecisiveCarbonNitrogenRatiosForDecomposition(UTILS utils, std::string typeOfPool);
   bool decompose(UTILS utils, double flow, double ligcon, std::string type);

   bool decomposable(UTILS utils, std::string type);
   void calculateRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool);
   void calculateCarbonRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool);
   void calculateNitrogenRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool);
   void determineNitrogenFlux(UTILS utils, std::string transferFromPoolype, std::string transferToPool);
   void immobilizeOrMineralizeNitrogen(UTILS utils, double carbonFlux, double nitrogenFlow, double decisiveCNratio, std::string transferFromPool, std::string transferToPool);
   void immobilizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio);
   void mineralizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio, double oldflow);

   void calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(UTILS utils, PARAMETER parameter, WEATHER weather);
   void calculateNitrogenLossByVolatilization(UTILS utils);
   void updateSoilPoolsByRespirationAndFluxes(UTILS utils);
   void doLeaching(UTILS utils);
};