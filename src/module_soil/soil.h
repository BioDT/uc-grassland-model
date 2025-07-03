#pragma once
#include "../module_init/constants.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
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

   double snowContent;
   double soilTemperature;

   double greenCarbonSurfaceLitter;
   double brownCarbonSurfaceLitter;
   double rootCarbonSoilLitter;
   double seedCarbonSoilLitter;

   double greenNitrogenSurfaceLitter;
   double brownNitrogenSurfaceLitter;
   double rootNitrogenSoilLitter;
   double seedNitrogenSoilLitter;

   double CPool_Soil_passive;
   double CPool_Soil_slow;
   double CPool_Soil_active;
   double CPool_Soil_microbes;
   double CPool_Soil_litter_met;
   double CPool_Surface_litter_met;
   double CPool_Soil_litter_struc;
   double CPool_Surface_litter_struc;

   double NPool_Soil_passive;
   double NPool_Soil_slow;
   double NPool_Soil_active;
   double NPool_Soil_microbes;
   double NPool_Soil_litter_met;
   double NPool_Surface_litter_met;
   double NPool_Soil_litter_struc;
   double NPool_Surface_litter_struc;

   double CN_passive;
   double CN_slow;
   double CN_active;
   double CN_microbes;
   double CN_soil_active_met;
   double CN_surface_active_met;
   double CN_soil_active_new;
   double CN_surface_active_new;
   double CN_surface_slow_new;
   double CN_soil_slow_new;
   double CN_active_passive;
   double CN_slow_passive;

   double Cflux_Surface_to_Soil_slow;
   double Cflux_Surface_to_Soil_microbes;
   double Cflux_Soil_to_Soil_slow;
   double Cflux_Soil_to_Soil_active;
   double Cflux_Surface_to_Soil_microbes_met;
   double Cflux_Soil_to_Soil_active_met;
   double Cflux_Microbe_to_Slow;
   double Cflux_Active_to;
   double Cflux_Slow_to;
   double Cflux_Passive_to_Active;
   double Cflux_Slow_to_Passive;
   double Cflux_Active_to_Passive;
   double Cflux_Slow_to_Active;
   double Cflux_Active_to_Slow;

   double Resp_DecompC_surface_slow;
   double Resp_DecompC_surface_microbes;
   double Resp_DecompC_soil_slow;
   double Resp_DecompC_soil_active;
   double Resp_DecompC_surface_met;
   double Resp_DecompC_soil_met;
   double Resp_DecompC_microbes;
   double Resp_DecompC_active;
   double Resp_DecompC_slow;
   double Resp_DecompC_passive;

   double Resp_DecompN_surface_slow;
   double Resp_DecompN_surface_microbes;
   double Resp_DecompN_soil_slow;
   double Resp_DecompN_soil_active;
   double Resp_DecompN_surface_met;
   double Resp_DecompN_soil_met;
   double Resp_DecompN_microbes;
   double Resp_DecompN_active;
   double Resp_DecompN_slow;
   double Resp_DecompN_passive;

   double NMineralization_gross;
   double NMineralization_net;
   double Nflux_Surface_to_Soil_slow;
   double Nflux_Soil_to_Soil_slow;
   double Nflux_Microbe_to_Slow;
   double Nflux_Active_to_Slow;
   double Nflux_Surface_to_Soil_microbes;
   double Nflux_Surface_to_Soil_microbes_met;
   double Nflux_Soil_to_Soil_active;
   double Nflux_Soil_to_Soil_active_met;
   double Nflux_Slow_to_Active;
   double Nflux_Passive_to_Active;
   double Nflux_Active_to_Passive;
   double Nflux_Slow_to_Passive;

   double Nflow_Surface_slow;
   double Nflow_microbes;
   double Nflow_Soil_slow;
   double Nflow_active;
   double Nflow_microbes_met;
   double Nflow_active_met;
   double Nflow_Microbe_Slow;
   double Nflow_Active_Passive;
   double Nflow_Active_Slow;
   double Nflow_Slow_Passive;
   double Nflow_Slow_Active;
   double Nflow_Passive_Active;

   double immob_Surface_slow;
   double immob_microbes;
   double immob_Soil_slow;
   double immob_active;
   double immob_microbes_met;
   double immob_active_met;
   double immob_microbe_slow;
   double immob_active_passive;
   double immob_active_slow;
   double immob_slow_passive;
   double immob_slow_active;
   double immob_passive_active;

   double miner_Surface_slow;
   double miner_microbes;
   double miner_Soil_slow;
   double miner_active;
   double miner_microbes_met;
   double miner_active_met;
   double miner_microbe_slow;
   double miner_active_passive;
   double miner_active_slow;
   double miner_slow_passive;
   double miner_slow_active;
   double miner_passive_active;

   double CLeach;

   void transferDyingPlantPartsToLitterPools(PARAMETER parameter, int number, double biomass, int typeOfMaterial, int pft);
   /*void calculateSoilCarbonNitrogenWaterDynamics(UTILS utils, PARAMETER parameter, COMMUNITY &community);
   double calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, PARAMETER parameter);
   bool decomposable(PARAMETER parameter, double aminrl, std::string type);
   void calculateLitterCarbonRespiration(std::string type, std::string dest);
   void calculateLitterNitrogenRespiration(std::string type, std::string dest);
   double calculateLitterNitrogenFlow(std::string type, std::string dest);
   double immobilizeNitrogen(std::string type, std::string dest, double cmprratio);
   double mineralizeNitrogen(std::string type, std::string dest, double cmprratio, double oldflow);
   bool decompose(double flow, double ligcon, double aminrl, std::string type);*/
};