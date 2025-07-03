#include "soil.h"

SOIL::SOIL() {};
SOIL::~SOIL() {};

void SOIL::transferDyingPlantPartsToLitterPools(PARAMETER parameter, int number, double biomass, int typeOfMaterial, int pft)
{
   if (typeOfMaterial == 0)
   {
      greenCarbonSurfaceLitter += (number * (biomass * carbonContentOdm));
      greenNitrogenSurfaceLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioGreenLeaves[pft]));
   }
   else if (typeOfMaterial == 1)
   {
      brownCarbonSurfaceLitter += (number * (biomass * carbonContentOdm));
      brownNitrogenSurfaceLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioBrownLeaves[pft]));
   }
   else if (typeOfMaterial == 2)
   {
      rootCarbonSoilLitter += (number * (biomass * carbonContentOdm));
      rootNitrogenSoilLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioRoots[pft]));
   }
   else if (typeOfMaterial == 3)
   {
      seedCarbonSoilLitter += (number * (biomass * carbonContentOdm));
      seedNitrogenSoilLitter += (number * ((biomass * carbonContentOdm) / parameter.plantCNRatioSeeds[pft]));
   }
}

/*
* @cite Function and code has been reused from the CENTURY4.0 soil model
void SOIL::calculateSoilCarbonNitrogenWaterDynamics(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
   double aminrl = 0, tcflow = 0;
   const double maxflow = 5000.0 * 1E-06; // in [g/m²]

   initializeFluxes();

   // Transfer residues to litter pools
   // calculate Litter Fluxes
   calcLitterInput();

   // Transfer litter into soil pools
   litterInputToSoil();

   double decompositionFactor = calculateTemperatureAndWaterEffectsOnDecomposition(utils, parameter);
   aminrl = soilNitrogenPerLayer.at(0);

   // Surface STRUCTURAL Material
   if (CPool_Surface_litter_struc > 1E-13)
   {
      const double s1 = CPool_Surface_litter_struc;
      const double s2 = maxflow;

      // Compute total C flow out of structural Surface
      tcflow = std::min(s1, s2) * decompositionFactor * 3.9 * exp(-3.0 * strlig_SRFC);
      Decompose(tcflow, strlig_SRFC, aminrl, "surface");
   }

   // Soil STRUCTURAL Material
   if (CPool_Soil_litter_struc > 1E-13)
   {
      const double s1 = CPool_Soil_litter_struc;
      const double s2 = maxflow;

      // Compute total C flow out of structural Soil
      tcflow = std::min(s1, s2) * decompositionFactor * 4.9 * exp(-3.0 * strlig_SOIL);
      Decompose(tcflow, strlig_SOIL, aminrl, "soil");
   }

   // Surface METABOLIC Material
   if (CPool_Surface_litter_met > 1E-13)
   {
      tcflow = CPool_Surface_litter_met * decompositionFactor * 14.8;

      // CN ratio
      calcCNRatio("surface_met", aminrl);
      Decompose(tcflow, 1, aminrl, "surface_met");
   }

   // Soil METABOLIC Material
   if (CPool_Soil_litter_met > 1E-13)
   {
      tcflow = CPool_Soil_litter_met * decompositionFactor * 18.5;
      calcCNRatio("soil_met", aminrl);
      Decompose(tcflow, 1, aminrl, "soil_met");
   }

   // SOM pools - fluxes
   // microbial pool to slow soil pool
   if (CPool_Soil_microbes > 1E-13)
   {
      tcflow = CPool_Soil_microbes * decompositionFactor * 6.0;
      calcCNRatio("microbes", aminrl);
      Decompose(tcflow, 1, aminrl, "microbes");
   }

   // soil active pool to slow and passive soil pool + possible leaching (C and N)
   if (CPool_Soil_active > 1E-13)
   {
      double eftext = 0.25 + 0.75 * sandContent;
      tcflow = CPool_Soil_active * decompositionFactor * 7.3 * eftext;
      calcCNRatio("active", aminrl);
      Decompose(tcflow, 1, aminrl, "active");
   }

   // slow soil pool to active and passive soil pool
   if (CPool_Soil_slow > 1E-13)
   {
      tcflow = CPool_Soil_slow * decompositionFactor * 0.2;
      calcCNRatio("slow", aminrl);
      Decompose(tcflow, 1, aminrl, "slow");
   }

   // passive soil pool to active soil pool
   if (CPool_Soil_passive > 1E-13)
   {
      tcflow = CPool_Soil_passive * decompositionFactor * 0.0045;
      calcCNRatio("passive", aminrl);
      Decompose(tcflow, 1, aminrl, "passive");
   }

   // Process all flows
   processCarbonNitrogenFlows();

   // setting new aminrl as mean within a week
   aminrl = (aminrl + soilNitrogenPerLayer.at(0)) / 2.0;

   // N fixation nonsymbiotic and atmospheric deposition
   double wdfxms;
   double wdfxma;
   wdfxms = 0;
   wdfxma = 0.01 * (weather.pet - (30.0 / 365.0));
   if (wdfxma < 0.)
      wdfxma = 0.0;

   soilNitrogenPerLayer.at(0) += (wdfxma + wdfxms);
   NFixation += (wdfxma + wdfxms);

   // Volatilization loss of nitrogen as a function of gross mineralization
   double volgm;
   volgm = 0.0 * NMineralization_gross;
   soilNitrogenPerLayer.at(0) -= volgm;
   NVolatilization += volgm;

   if (soilNitrogenPerLayer.at(0) + tolerance < 0.0)
   {
      soilNitrogenPerLayer.at(0) = 0.0;
      utils.handleError("Plants do not have access to soil nitrogen or soil nitrogen pool is negative!");
   }

   // total fluxes of carbon and nitrogen
   // Flux = N(respiration) + N(mineralization) - N(immobilization) - volatilization
   // + fixation + fertilizer
   Nflux = NMineralization_net - NVolatilization + NFixation + NFertilization;

   // R_total = RespC_litter + RespC_soilpools + carbonContentOdm *
   // R_total_biomass_month;
   R_total = RespC_litter + RespC_soilpools +
             carbonContentOdm * R_total_biomass;
   C_flux = ((carbonContentOdm * (PB_month + ingrowth_month)) - R_total -
             LeachingC - carbonContentOdm * (HarvestBb + HarvestBg));

   R_total_biomass_month = 0;
   PB_month = 0;
   ingrowth_month = 0;
   // NUptake_month = 0;
   // RepNuptake_month = 0;

   // add Fertilization option

}

* @cite Function and code has been reused from the CENTURY4.0 soil model
double SOIL::calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, PARAMETER parameter)
{
   if (snowContent > 0.0)
   {
      soilTemperature = 0.0;
   }

   double temperatureFunction;
   temperatureFunction = (((atan(((soilTemperature - 15.4) + (2 * PI)) / (0.031 * 11.75 * 29.7))) + atan(0.031 * 29.7 * PI)) /
                          (2 * atan(0.031 * 29.7 * PI)));

   double waterFunction;
   double relativeWaterContent = (soilWater.at(0) - permanentWiltingPoint.at(0)) / (fieldCapacity.at(0) - permanentWiltingPoint.at(0));

   if (relativeWaterContent > 13.0)
      waterFunction = 1.0;
   else
      waterFunction = 1.0 / (1.0 + 4.0 * exp(-6.0 * relativeWaterContent));

   if (waterFunction >= 1.0)
      waterFunction = 1.0;

   double defac = temperatureFunction * waterFunction;
   if (defac < 0.0)
      defac = 0.0;

   return (defac);
}

bool SOIL::decomposable(PARAMETER parameter, double aminrl, std::string type)
{

   // aminrl in [g/m²]
   bool cando = true;

   if (aminrl < 1e-07)
   {
      // if decomposable to active surface or soil pool, then also to slow one
      // if not, no decomposition at all
      if (type == "surface")
      {
         if ((CPool_Surface_litter_struc / NPool_Surface_litter_struc) >
             CN_surface_active_new)
            cando = false;
      }
      else if (type == "soil")
      {
         if ((CPool_Soil_litter_struc / NPool_Soil_litter_struc) > CN_soil_active_new)
            cando = false;
      }
      else if (type == "surface_met")
      {
         if ((CPool_Surface_litter_met / NPool_Surface_litter_met) >
             CN_surface_active_met)
            cando = false;
      }
      else if (type == "soil_met")
      {
         if ((CPool_Soil_litter_met / NPool_Soil_litter_met) > CN_soil_active_met)
            cando = false;
      }
      else if (type == "microbes")
      {
         if ((CPool_Soil_microbes / NPool_Soil_microbes) > CN_microbes)
            cando = false;
      }
      else if (type == "active")
      {
         if ((CPool_Soil_active / NPool_Soil_active) > CN_active)
            cando = false;
      }
      else if (type == "slow")
      {
         if ((CPool_Soil_slow / NPool_Soil_slow) > CN_slow)
            cando = false;
      }
      else if (type == "passive")
      {
         if ((CPool_Soil_passive / NPool_Soil_passive) > CN_passive)
            cando = false;
      }
      // if CN ratio of litter is larger than predefined ratio AND if there is not enough mineral nitrogen in
      // upper soil layer, immobilization could not be processed --> so no decomposition is possible!
   }

   return cando;
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
void SOIL::calculateLitterCarbonRespiration(std::string type, std::string dest)
{
   if (type == "surface")
   {
      if (dest == "slow")
      {
         Resp_DecompC_surface_slow = Cflux_Surface_to_Soil_slow * 0.3;
         Cflux_Surface_to_Soil_slow -= Resp_DecompC_surface_slow;
      }
      else if (dest == "microbes")
      {
         Resp_DecompC_surface_microbes = Cflux_Surface_to_Soil_microbes * 0.45;
         Cflux_Surface_to_Soil_microbes -= Resp_DecompC_surface_microbes;
      }
   }
   else if (type == "soil")
   {
      if (dest == "slow")
      {
         Resp_DecompC_soil_slow = Cflux_Soil_to_Soil_slow * 0.3;
         Cflux_Soil_to_Soil_slow -= Resp_DecompC_soil_slow;
      }
      else if (dest == "active")
      {
         Resp_DecompC_soil_active = Cflux_Soil_to_Soil_active * 0.55;
         Cflux_Soil_to_Soil_active -= Resp_DecompC_soil_active;
      }
   }
   else if (type == "surface_met")
   {
      Resp_DecompC_surface_met = Cflux_Surface_to_Soil_microbes_met * 0.55;
      Cflux_Surface_to_Soil_microbes_met -= Resp_DecompC_surface_met;
   }
   else if (type == "soil_met")
   {
      Resp_DecompC_soil_met = Cflux_Soil_to_Soil_active_met * 0.55;
      Cflux_Soil_to_Soil_active_met -= Resp_DecompC_soil_met;
   }
   else if (type == "microbes")
   {
      Resp_DecompC_microbes = Cflux_Microbe_to_Slow * 0.6;
      Cflux_Microbe_to_Slow -= Resp_DecompC_microbes;
   }
   else if (type == "active")
   {
      Resp_DecompC_active = Cflux_Active_to * (0.17 + 0.68 * sandContent);
   }
   else if (type == "slow")
   {
      Resp_DecompC_slow = Cflux_Slow_to * 0.55;
   }
   else if (type == "passive")
   {
      Resp_DecompC_passive = Cflux_Passive_to_Active * 0.55;
      Cflux_Passive_to_Active -= Resp_DecompC_passive;
   }
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
void SOIL::calculateLitterNitrogenRespiration(std::string type, std::string dest)
{
   double ratio = 1;

   if (type == "surface")
   {
      ratio = (NPool_Surface_litter_struc / CPool_Surface_litter_struc);

      if (dest == "slow")
      {
         Resp_DecompN_surface_slow = Resp_DecompC_surface_slow * ratio;
         NMineralization_gross += Resp_DecompN_surface_slow;
         NMineralization_net += Resp_DecompN_surface_slow;
      }
      else if (dest == "microbes")
      {
         Resp_DecompN_surface_microbes = Resp_DecompC_surface_microbes * ratio;
         NMineralization_gross += Resp_DecompN_surface_microbes;
         NMineralization_net += Resp_DecompN_surface_microbes;
      }
   }
   else if (type == "soil")
   {
      ratio = (NPool_Soil_litter_struc / CPool_Soil_litter_struc);

      if (dest == "slow")
      {
         Resp_DecompN_soil_slow = Resp_DecompC_soil_slow * ratio;
         NMineralization_gross += Resp_DecompN_soil_slow;
         NMineralization_net += Resp_DecompN_soil_slow;
      }
      else if (dest == "active")
      {
         Resp_DecompN_soil_active = Resp_DecompC_soil_active * ratio;
         NMineralization_gross += Resp_DecompN_soil_active;
         NMineralization_net += Resp_DecompN_soil_active;
      }
   }
   else if (type == "surface_met")
   {
      ratio = (NPool_Surface_litter_met / CPool_Surface_litter_met);

      if (dest == "microbes")
      {
         Resp_DecompN_surface_met = Resp_DecompC_surface_met * ratio;
         NMineralization_gross += Resp_DecompN_surface_met;
         NMineralization_net += Resp_DecompN_surface_met;
      }
   }
   else if (type == "soil_met")
   {
      ratio = (NPool_Soil_litter_met / CPool_Soil_litter_met);

      if (dest == "active")
      {
         Resp_DecompN_soil_met = Resp_DecompC_soil_met * ratio;
         NMineralization_gross += Resp_DecompN_soil_met;
         NMineralization_net += Resp_DecompN_soil_met;
      }
   }
   else if (type == "microbes")
   {
      ratio = (NPool_Soil_microbes / CPool_Soil_microbes);

      if (dest == "slow")
      {
         Resp_DecompN_microbes = Resp_DecompC_microbes * ratio;
         NMineralization_gross += Resp_DecompN_microbes;
         NMineralization_net += Resp_DecompN_microbes;
      }
   }
   else if (type == "active")
   {
      ratio = (NPool_Soil_active / CPool_Soil_active);

      Resp_DecompN_active = Resp_DecompC_active * ratio;
      NMineralization_gross += Resp_DecompN_active;
      NMineralization_net += Resp_DecompN_active;
   }
   else if (type == "slow")
   {
      ratio = (NPool_Soil_slow / CPool_Soil_slow);

      Resp_DecompN_slow = Resp_DecompC_slow * ratio;
      NMineralization_gross += Resp_DecompN_slow;
      NMineralization_net += Resp_DecompN_slow;
   }
   else if (type == "passive")
   {
      ratio = (NPool_Soil_passive / CPool_Soil_passive);

      Resp_DecompN_passive = Resp_DecompC_passive * ratio;
      NMineralization_gross += Resp_DecompN_passive;
      NMineralization_net += Resp_DecompN_passive;
   }
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
double SOIL::calculateLitterNitrogenFlow(std::string type, std::string dest)
{
   double cmprratio = 1;

   if (dest == "slow")
   {
      if (type == "surface")
      {
         Nflux_Surface_to_Soil_slow =
             NPool_Surface_litter_struc *
             (Cflux_Surface_to_Soil_slow / CPool_Surface_litter_struc);

         cmprratio = CN_surface_slow_new;
      }
      else if (type == "soil")
      {
         Nflux_Soil_to_Soil_slow = NPool_Soil_litter_struc *
                                   (Cflux_Soil_to_Soil_slow / CPool_Soil_litter_struc);

         cmprratio = CN_soil_slow_new;
      }
      else if (type == "microbes")
      {
         Nflux_Microbe_to_Slow =
             NPool_Soil_microbes * (Cflux_Microbe_to_Slow / CPool_Soil_microbes);

         cmprratio = CN_microbes;
      }
      else if (type == "active")
      {
         Nflux_Active_to_Slow =
             NPool_Soil_active * (Cflux_Active_to_Slow / CPool_Soil_active);

         cmprratio = CN_active;
      }
   }
   else if (dest == "microbes")
   {
      if (type == "surface")
      {
         Nflux_Surface_to_Soil_microbes =
             NPool_Surface_litter_struc *
             (Cflux_Surface_to_Soil_microbes / CPool_Surface_litter_struc);

         cmprratio = CN_surface_active_new;
      }

      if (type == "surface_met")
      {
         Nflux_Surface_to_Soil_microbes_met =
             NPool_Surface_litter_met *
             (Cflux_Surface_to_Soil_microbes_met / CPool_Surface_litter_met);

         cmprratio = CN_surface_active_met;
      }
   }
   else if (dest == "active")
   {
      if (type == "soil")
      {
         Nflux_Soil_to_Soil_active =
             NPool_Soil_litter_struc *
             (Cflux_Soil_to_Soil_active / CPool_Soil_litter_struc);

         cmprratio = CN_soil_active_new;
      }

      if (type == "soil_met")
      {
         Nflux_Soil_to_Soil_active_met =
             NPool_Soil_litter_met *
             (Cflux_Soil_to_Soil_active_met / CPool_Soil_litter_met);

         cmprratio = CN_soil_active_met;
      }

      if (type == "slow")
      {
         Nflux_Slow_to_Active =
             NPool_Soil_slow * (Cflux_Slow_to_Active / CPool_Soil_slow);

         cmprratio = CN_slow;
      }

      if (type == "passive")
      {
         Nflux_Passive_to_Active =
             NPool_Soil_passive * (Cflux_Passive_to_Active / CPool_Soil_passive);

         cmprratio = CN_passive;
      }
   }
   else if (dest == "passive")
   {
      if (type == "active")
      {
         Nflux_Active_to_Passive =
             NPool_Soil_active * (Cflux_Active_to_Passive / CPool_Soil_active);

         cmprratio = CN_active_passive;
      }
      if (type == "slow")
      {
         Nflux_Slow_to_Passive =
             NPool_Soil_slow * (Cflux_Slow_to_Passive / CPool_Soil_slow);

         cmprratio = CN_slow_passive;
      }
   }

   return cmprratio;
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
double SOIL::immobilizeNitrogen(std::string type, std::string dest, double cmprratio)
{
   double immo = 0;

   if (type == "surface")
   {
      if (dest == "slow")
         immo = (Cflux_Surface_to_Soil_slow / cmprratio) - Nflux_Surface_to_Soil_slow;

      if (dest == "microbes")
         immo = (Cflux_Surface_to_Soil_microbes / cmprratio) - Nflux_Surface_to_Soil_microbes;
   }
   else if (type == "soil")
   {
      if (dest == "slow")
         immo = (Cflux_Soil_to_Soil_slow / cmprratio) - Nflux_Soil_to_Soil_slow;

      if (dest == "active")
         immo = (Cflux_Soil_to_Soil_active / cmprratio) - Nflux_Soil_to_Soil_active;
   }
   else if (type == "surface_met")
   {
      if (dest == "microbes")
         immo = (Cflux_Surface_to_Soil_microbes_met / cmprratio) -
                Nflux_Surface_to_Soil_microbes_met;
   }
   else if (type == "soil_met")
   {
      if (dest == "active")
         immo = (Cflux_Soil_to_Soil_active_met / cmprratio) - Nflux_Soil_to_Soil_active_met;
   }
   else if (type == "microbes")
   {
      if (dest == "slow")
         immo = (Cflux_Microbe_to_Slow / cmprratio) - Nflux_Microbe_to_Slow;
   }
   else if (type == "active")
   {
      if (dest == "passive")
         immo = (Cflux_Active_to_Passive / cmprratio) - Nflux_Active_to_Passive;

      if (dest == "slow")
         immo = (Cflux_Active_to_Slow / cmprratio) - Nflux_Active_to_Slow;
   }
   else if (type == "slow")
   {
      if (dest == "passive")
         immo = (Cflux_Slow_to_Passive / cmprratio) - Nflux_Slow_to_Passive;

      if (dest == "active")
         immo = (Cflux_Slow_to_Active / cmprratio) - Nflux_Slow_to_Active;
   }
   else if (type == "passive")
   {
      if (dest == "active")
         immo = (Cflux_Passive_to_Active / cmprratio) - Nflux_Passive_to_Active;
   }

   if (immo < 0)
      NMineralization_gross -= immo;
   NMineralization_net -= immo;

   return immo;
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
double SOIL::mineralizeNitrogen(std::string type, std::string dest, double cmprratio, double oldflow)
{
   double minflow = 0;

   if (type == "surface")
   {
      if (dest == "slow")
      {
         Nflux_Surface_to_Soil_slow = Cflux_Surface_to_Soil_slow / cmprratio;
         minflow = oldflow - Nflux_Surface_to_Soil_slow;
      }
      if (dest == "microbes")
      {
         Nflux_Surface_to_Soil_microbes = Cflux_Surface_to_Soil_microbes / cmprratio;
         minflow = oldflow - Nflux_Surface_to_Soil_microbes;
      }
   }
   else if (type == "soil")
   {
      if (dest == "slow")
      {
         Nflux_Soil_to_Soil_slow = Cflux_Soil_to_Soil_slow / cmprratio;
         minflow = oldflow - Nflux_Soil_to_Soil_slow;
      }
      if (dest == "active")
      {
         Nflux_Soil_to_Soil_active = Cflux_Soil_to_Soil_active / cmprratio;
         minflow = oldflow - Nflux_Soil_to_Soil_active;
      }
   }
   else if (type == "surface_met")
   {
      if (dest == "microbes")
      {
         Nflux_Surface_to_Soil_microbes_met = Cflux_Surface_to_Soil_microbes_met / cmprratio;
         minflow = oldflow - Nflux_Surface_to_Soil_microbes_met;
      }
   }
   else if (type == "soil_met")
   {
      if (dest == "active")
      {
         Nflux_Soil_to_Soil_active_met = Cflux_Soil_to_Soil_active_met / cmprratio;
         minflow = oldflow - Nflux_Soil_to_Soil_active_met;
      }
   }
   else if (type == "microbes")
   {
      if (dest == "slow")
      {
         Nflux_Microbe_to_Slow = Cflux_Microbe_to_Slow / cmprratio;
         minflow = oldflow - Nflux_Microbe_to_Slow;
      }
   }
   else if (type == "active")
   {
      if (dest == "passive")
      {
         Nflux_Active_to_Passive = Cflux_Active_to_Passive / cmprratio;
         minflow = oldflow - Nflux_Active_to_Passive;
      }

      if (dest == "slow")
      {
         Nflux_Active_to_Slow = Cflux_Active_to_Slow / cmprratio;
         minflow = oldflow - Nflux_Active_to_Slow;
      }
   }
   else if (type == "slow")
   {
      if (dest == "passive")
      {
         Nflux_Slow_to_Passive = Cflux_Slow_to_Passive / cmprratio;
         minflow = oldflow - Nflux_Slow_to_Passive;
      }

      if (dest == "active")
      {
         Nflux_Slow_to_Active = Cflux_Slow_to_Active / cmprratio;
         minflow = oldflow - Nflux_Slow_to_Active;
      }
   }
   else if (type == "passive")
   {
      if (dest == "active")
      {
         Nflux_Passive_to_Active = Cflux_Passive_to_Active / cmprratio;
         minflow = oldflow - Nflux_Passive_to_Active;
      }
   }

   if (minflow > 0)
      NMineralization_gross += minflow;
   NMineralization_net += minflow;

   return minflow;
}

* @cite Function and code has been reused from the CENTURY4.0 soil model
bool SOIL::decompose(double flow, double ligcon, double aminrl, std::string type)
{
   double cmprratio = 1;

   if (decomposable(aminrl, type))
   {
      if (type == "surface")
      {
         // -------  to slow soil pool
         Cflux_Surface_to_Soil_slow = flow * ligcon;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "slow");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "slow");

         // proportional nitrogen flow from litter to soil slow pool
         cmprratio = calculateLitterNitrogenFlow(type, "slow");

         Nflow_Surface_slow = Nflux_Surface_to_Soil_slow;
         if (Cflux_Surface_to_Soil_slow > 0.0 && Nflow_Surface_slow > 0.0)
         {
            if ((Cflux_Surface_to_Soil_slow / Nflow_Surface_slow) >
                cmprratio)
            { // immobilization occurs
               immob_Surface_slow = immobilizeNitrogen(type, "slow", cmprratio);
               miner_Surface_slow = -immob_Surface_slow;
            }
            else
            { // mineralization occurs, patch->Nflux_Surface_to_Soil_slow will be changed
               miner_Surface_slow =
                   mineralizeNitrogen(type, "slow", cmprratio, Nflow_Surface_slow);
            }
         }

         // -------  to mirobial soil pool

         Cflux_Surface_to_Soil_microbes =
             flow - Cflux_Surface_to_Soil_slow - Resp_DecompC_surface_slow;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "microbes");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "microbes");

         // proportional nitrogen flow from litter to soil microbial pool
         cmprratio = calculateLitterNitrogenFlow(type, "microbes");

         Nflow_microbes = Nflux_Surface_to_Soil_microbes;
         if (Cflux_Surface_to_Soil_microbes > 0.0 && Nflow_microbes > 0.0)
         {
            if ((Cflux_Surface_to_Soil_microbes / Nflow_microbes) >
                cmprratio)
            { // immobilization occurs
               immob_microbes = immobilizeNitrogen(type, "microbes", cmprratio);
               miner_microbes = -immob_microbes;
            }
            else
            { // mineralization occurs, patch->Nflux_Surface_to_Soil_microbes will be changed
               miner_microbes =
                   mineralizeNitrogen(type, "microbes", cmprratio, Nflow_microbes);
            }
         }
      }

      // -----------------------------------------------------
      if (type == "soil")
      {
         // -------  to slow soil pool
         Cflux_Soil_to_Soil_slow = flow * ligcon;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "slow");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "slow");

         // proportional nitrogen flow from litter to soil slow pool
         cmprratio = calculateLitterNitrogenFlow(type, "slow");

         Nflow_Soil_slow = Nflux_Soil_to_Soil_slow;
         if (Cflux_Soil_to_Soil_slow > 0.0 && Nflow_Soil_slow > 0.0)
         {
            if ((Cflux_Soil_to_Soil_slow / Nflow_Soil_slow) >
                cmprratio)
            { // immobilization occurs
               immob_Soil_slow = immobilizeNitrogen(type, "slow", cmprratio);
               miner_Soil_slow = -immob_Soil_slow;
            }
            else
            { // mineralization occurs, patch->Nflux_Soil_to_Soil_slow will be changed
               miner_Soil_slow =
                   mineralizeNitrogen(type, "slow", cmprratio, Nflow_Soil_slow);
            }
         }

         // -------  to active soil pool

         Cflux_Soil_to_Soil_active =
             flow - Cflux_Soil_to_Soil_slow - Resp_DecompC_soil_slow;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "active");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "active");

         // proportional nitrogen flow from litter to soil active pool
         cmprratio = calculateLitterNitrogenFlow(type, "active");

         Nflow_active = Nflux_Soil_to_Soil_active;
         if (Cflux_Soil_to_Soil_active > 0.0 && Nflow_active > 0.0)
         {
            if ((Cflux_Soil_to_Soil_active / Nflow_active) >
                cmprratio)
            { // immobilization occurs
               immob_active = immobilizeNitrogen(type, "active", cmprratio);
               miner_active = -immob_active;
            }
            else
            { // mineralization occurs, patch->Nflux_Soil_to_Soil_active will be changed
               miner_active =
                   mineralizeNitrogen(type, "active", cmprratio, Nflow_active);
            }
         }
      }

      // ---------------------------------------------------------
      if (type == "surface_met")
      {
         if (flow > CPool_Surface_litter_met)
            flow = CPool_Surface_litter_met;

         Cflux_Surface_to_Soil_microbes_met = flow;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "microbes");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "microbes");

         // proportional nitrogen flow from litter to microbial pool
         cmprratio = calculateLitterNitrogenFlow(type, "microbes");

         Nflow_microbes_met = Nflux_Surface_to_Soil_microbes_met;
         if (Cflux_Surface_to_Soil_microbes_met > 0.0 && Nflow_microbes_met > 0.0)
         {
            if ((Cflux_Surface_to_Soil_microbes_met / Nflow_microbes_met) >
                cmprratio)
            { // immobilization occurs
               immob_microbes_met = immobilizeNitrogen(type, "microbes", cmprratio);
               miner_microbes_met = -immob_microbes_met;
            }
            else
            { // mineralization occurs, patch->Nflux_Surface_to_Soil_microbes_met will be changed
               miner_microbes_met =
                   mineralizeNitrogen(type, "microbes", cmprratio, Nflow_microbes_met);
            }
         }
      }

      // ------------------------------------------------
      if (type == "soil_met")
      {
         if (flow > CPool_Soil_litter_met)
         {
            flow = CPool_Soil_litter_met;
         }
         Cflux_Soil_to_Soil_active_met = flow;

         // respiration asscociated with litter decomposition
         calculateLitterCarbonRespiration(type, "active");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "active");

         // proportional nitrogen flow from litter to soil active pool
         cmprratio = calculateLitterNitrogenFlow(type, "active");

         Nflow_active_met = Nflux_Soil_to_Soil_active_met;
         if (Cflux_Soil_to_Soil_active_met > 0.0 && Nflow_active_met > 0.0)
         {
            if ((Cflux_Soil_to_Soil_active_met / Nflow_active_met) >
                cmprratio)
            { // immobilization occurs
               immob_active_met = immobilizeNitrogen(type, "active", cmprratio);
               miner_active_met = -immob_active_met;
            }
            else
            { // mineralization occurs, patch->Nflux_Soil_to_Soil_active_met will be changed
               miner_active_met =
                   mineralizeNitrogen(type, "active", cmprratio, Nflow_active_met);
            }
         }
      }

      // -------------------------------------------
      if (type == "microbes")
      {
         Cflux_Microbe_to_Slow = flow;

         // respiration asscociated with microbial decomposition
         calculateLitterCarbonRespiration(type, "slow");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "slow");

         // proportional nitrogen flow from microbes to soil slow pool
         cmprratio = calculateLitterNitrogenFlow(type, "slow");

         Nflow_Microbe_Slow = Nflux_Microbe_to_Slow;
         if (Cflux_Microbe_to_Slow > 0.0 && Nflow_Microbe_Slow > 0.0)
         {
            if ((Cflux_Microbe_to_Slow / Nflow_Microbe_Slow) >
                cmprratio)
            { // immobilization occurs
               immob_microbe_slow = immobilizeNitrogen(type, "slow", cmprratio);
               miner_microbe_slow = -immob_microbe_slow;
            }
            else
            { // mineralization occurs, patch->Nflux_Microbe_to_Slow will be changed
               miner_microbe_slow =
                   mineralizeNitrogen(type, "slow", cmprratio, Nflow_Microbe_Slow);
            }
         }
      }

      // -------------------------------------------
      if (type == "active")
      {
         Cflux_Active_to = flow;

         // respiration asscociated with microbial decomposition
         calculateLitterCarbonRespiration(type, "");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "");

         // -------------- flux to passive soil pool incl. leaching
         Cflux_Active_to_Passive = Cflux_Active_to * (0.003 + 0.032 * Clay);

         // proportional nitrogen flow from microbes to soil slow pool
         calcCNRatio("active2passive", aminrl);
         cmprratio = calculateLitterNitrogenFlow(type, "passive");

         Nflow_Active_Passive = Nflux_Active_to_Passive;
         if (Cflux_Active_to_Passive > 0.0 && Nflow_Active_Passive > 0.0)
         {
            if ((Cflux_Active_to_Passive / Nflow_Active_Passive) >
                cmprratio)
            { // immobilization occurs
               immob_active_passive = immobilizeNitrogen(type, "passive", cmprratio);
               miner_active_passive = -immob_active_passive;
            }
            else
            { // mineralization occurs, Nflux_Active_to_Passive will be changed
               miner_active_passive =
                   mineralizeNitrogen(type, "passive", cmprratio, Nflow_Active_Passive);
            }
         }

         doLeaching();

         // -------------- flux to slow soil pool
         Cflux_Active_to_Slow = Cflux_Active_to - Resp_DecompC_active -
                                Cflux_Active_to_Passive - CLeach;

         cmprratio = calculateLitterNitrogenFlow(type, "slow");

         Nflow_Active_Slow = Nflux_Active_to_Slow;
         if (Cflux_Active_to_Slow > 0.0 && Nflow_Active_Slow > 0.0)
         {
            if ((Cflux_Active_to_Slow / Nflow_Active_Slow) >
                cmprratio)
            { // immobilization occurs
               immob_active_slow = immobilizeNitrogen(type, "slow", cmprratio);
               miner_active_slow = -immob_active_slow;
            }
            else
            { // mineralization occurs, Nflux_Active_to_Slow will be changed
               miner_active_slow =
                   mineralizeNitrogen(type, "slow", cmprratio, Nflow_Active_Slow);
            }
         }
      }

      // -------------------------------------------
      if (type == "slow")
      {
         Cflux_Slow_to = flow;

         // respiration asscociated with slow soil pool fluxes
         calculateLitterCarbonRespiration(type, "");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "");

         // --------- to passive pool
         Cflux_Slow_to_Passive = Cflux_Slow_to * (0.003 + 0.009 * clayContent);

         // proportional nitrogen flow from soil slow pool to passive pool
         calcCNRatio("slow2passive", aminrl);
         cmprratio = calculateLitterNitrogenFlow(type, "passive");

         Nflow_Slow_Passive = Nflux_Slow_to_Passive;
         if (Cflux_Slow_to_Passive > 0.0 && Nflow_Slow_Passive > 0.0)
         {
            if ((Cflux_Slow_to_Passive / Nflow_Slow_Passive) >
                cmprratio)
            { // immobilization occurs
               immob_slow_passive = immobilizeNitrogen(type, "passive", cmprratio);
               miner_slow_passive = -immob_slow_passive;
            }
            else
            { // mineralization occurs, patch->Nflux_Slow_to_Passive will be changed
               miner_slow_passive =
                   mineralizeNitrogen(type, "passive", cmprratio, Nflow_Slow_Passive);
            }
         }

         // --------- to active pool
         // proportional nitrogen flow from soil slow pool to active pool
         Cflux_Slow_to_Active =
             Cflux_Slow_to - Resp_DecompC_slow - Cflux_Slow_to_Passive;

         cmprratio = calculateLitterNitrogenFlow(type, "active");

         Nflow_Slow_Active = Nflux_Slow_to_Active;
         if (Cflux_Slow_to_Active > 0.0 && Nflow_Slow_Active > 0.0)
         {
            if ((Cflux_Slow_to_Active / Nflow_Slow_Active) >
                cmprratio)
            { // immobilization occurs
               immob_slow_active = immobilizeNitrogen(type, "active", cmprratio);
               miner_slow_active = -immob_slow_active;
            }
            else
            { // mineralization occurs, patch->Nflux_Slow_to_Active will be changed
               miner_slow_active =
                   mineralizeNitrogen(type, "active", cmprratio, Nflow_Slow_Active);
            }
         }
      }

      // -------------------------------------------
      if (type == "passive")
      {
         Cflux_Passive_to_Active = flow;

         // respiration asscociated with slow soil pool fluxes
         calculateLitterCarbonRespiration(type, "active");

         // nitrogen flow asscociated with respiration
         calculateLitterNitrogenRespiration(type, "active");

         // proportional nitrogen flow from soil passive pool to active pool
         cmprratio = calculateLitterNitrogenFlow(type, "active");

         Nflow_Passive_Active = Nflux_Passive_to_Active;
         if (Cflux_Passive_to_Active > 0.0 && Nflow_Passive_Active > 0.0)
         {
            if ((Cflux_Passive_to_Active / Nflow_Passive_Active) >
                cmprratio)
            { // immobilization occurs
               immob_passive_active = immobilizeNitrogen(type, "active", cmprratio);
               miner_passive_active = -immob_passive_active;
            }
            else
            { // mineralization occurs, Nflux_Passive_to_Active will be changed
               miner_passive_active =
                   mineralizeNitrogen(type, "active", cmprratio, Nflow_Passive_Active);
            }
         }
      }
   }
   return false;
}
*/