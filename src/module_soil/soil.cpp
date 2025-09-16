#include "soil.h"

SOIL::SOIL() {};
SOIL::~SOIL() {};

void SOIL::transferDyingPlantPartsToLitterPools(UTILS utils, PARAMETER parameter, int number, double biomass, std::string typeOfMaterial, int pft)
{

   double carbonFlux = number * (biomass * carbonContentOdm);

   if (typeOfMaterial == "surface_green")
   {
      carbonContent_surfaceGreenLitterPool += carbonFlux;
      nitrogenContent_surfaceGreenLitterPool += (carbonFlux / parameter.plantCNRatioGreenLeaves[pft]);
   }
   else if (typeOfMaterial == "surface_brown")
   {
      carbonContent_surfaceBrownLitterPool += carbonFlux;
      nitrogenContent_surfaceBrownLitterPool += (carbonFlux / parameter.plantCNRatioBrownLeaves[pft]);
   }
   else if (typeOfMaterial == "soil_root")
   {
      carbonContent_soilRootLitterPool += carbonFlux;
      nitrogenContent_soilRootLitterPool += (carbonFlux / parameter.plantCNRatioRoots[pft]);
   }
   else if (typeOfMaterial == "soil_seed")
   {
      carbonContent_soilSeedLitterPool += carbonFlux;
      nitrogenContent_soilSeedLitterPool += (carbonFlux / parameter.plantCNRatioSeeds[pft]);
   }
   else
   {
      utils.handleError("Wrong type of material of litter.");
   }
}

/**
 * @brief Main function of soil dynamics of water, carbon and nitrogen
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::calculateSoilResourceDynamics(UTILS utils, PARAMETER parameter, WEATHER weather)
{
   splitLitterFluxesToStructuralAndMetabolicLitterPools(utils, parameter, weather);

   decompositionFactor = calculateTemperatureAndWaterEffectsOnDecomposition(utils, parameter);
   doDecompositionFluxesInLitterAndSoilPools(utils, decompositionFactor);

   updateSoilPoolsByRespirationAndFluxes(utils);

   // nonsymbiotic nitrogen fixation and atmospheric nitrogen deposition
   calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(utils, parameter, weather);

   // Volatilization loss of nitrogen as a function of gross mineralization
   calculateNitrogenLossByVolatilization(utils);

   // balance = N(respiration) + N(mineralization) - N(immobilization) - volatilization + fixation + fertilizer
   ecosystemNitrogenBalance = nitrogenNetMineralization - nitrogenVolatilization + nitrogenFixation + nitrogenFertilization;

   // R_total = RespC_litter + RespC_soilpools + carbonContentOdm *
   // R_total_biomass_month;
   ecosystemRespiration = respirationCarbon_litter + respirationCarbon_soilpools + carbonContentOdm * R_total_biomass;
   ecosystemCarbonBalance = ((carbonContentOdm * (PB_month + ingrowth_month)) - ecosystemRespiration -
                             LeachingC - carbonContentOdm * (HarvestBb + HarvestBg));

   // add Fertilization option
}

void SOIL::splitLitterFluxesToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather)
{
   addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(utils, parameter, weather, "surface");
   addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(utils, parameter, weather, "soil");
}

void SOIL::addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather, std::string typeOfPool)
{
   double dirabs = 0;
   double fractionOfLignin, fractionOfNitrogen, ligninToNitrogenRatio;
   double fractionOfMetabolicLitter;
   double carbonAddedToMetabolicLitter, carbonAddedToStructuralLitter;
   double nitrogenAddedToMetabolicLitter, nitrogenAddedToStructuralLitter;
   double carbonFlux, nitrogenFlux;

   // define fluxes of litter pools
   if (typeOfPool == "surface")
   {
      carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
      nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
   }
   else if (typeOfPool == "soil")
   {
      carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
      nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
   }

   // split fluxes into metabolic and structural litter
   if (carbonFlux > 1E-13)
   {
      // ensure there is enough nitrogen
      dirabs = calculateDIRABS(utils, typeOfPool);

      // calculate lignin to nitrogen ratio
      fractionOfLignin = calculateLigninFraction(utils, weather, parameter, typeOfPool);
      fractionOfNitrogen = calculateNitrogenFraction(utils, dirabs, typeOfPool);
      ligninToNitrogenRatio = fractionOfLignin / fractionOfNitrogen;

      // Carbon added to metabolic and structural litter carbon pools
      fractionOfMetabolicLitter = calculateFractionOfMetabolicLitter(utils, fractionOfLignin, ligninToNitrogenRatio, typeOfPool);
      carbonAddedToMetabolicLitter = carbonFlux * fractionOfMetabolicLitter;
      carbonAddedToStructuralLitter = carbonFlux - carbonAddedToMetabolicLitter;

      // adjust lignin content
      if (typeOfPool == "surface")
      {
         ligninContent_surfaceStructuralLitterPool = adjustLigninContentOfStructuralLitter(utils, fractionOfLignin, carbonAddedToStructuralLitter, ligninContent_surfaceStructuralLitterPool, typeOfPool);
      }
      else if (typeOfPool == "soil")
      {
         ligninContent_soilStructuralLitterPool = adjustLigninContentOfStructuralLitter(utils, fractionOfLignin, carbonAddedToStructuralLitter, ligninContent_soilStructuralLitterPool, typeOfPool);
      }

      // Nitrogen added to metabolic and structural surface litter N pools
      nitrogenAddedToStructuralLitter = carbonAddedToStructuralLitter / 200.0;
      nitrogenAddedToMetabolicLitter = nitrogenFlux + dirabs - nitrogenAddedToStructuralLitter;

      processLitterFluxes(utils, dirabs, carbonAddedToStructuralLitter, carbonAddedToMetabolicLitter, nitrogenAddedToStructuralLitter, nitrogenAddedToMetabolicLitter, typeOfPool);
   }
}

double SOIL::calculateDIRABS(UTILS utils, std::string typeOfPool)
{
   double rcetot = 0.0;
   double dirabs = 0.0;
   double carbonFlux = 0;
   double nitrogenFlux = 0;

   double damr;
   if (typeOfPool == "surface")
   {
      damr = 0.0; // TODO: parameter for testing
      carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
      nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
   }
   else if (typeOfPool == "soil")
   {
      damr = 0.02;
      carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
      nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
   }

   dirabs = damr * nitrogenContent_soilMineralPoolPerSoilLayer.at(0) * std::max(carbonFlux / 100.0, 1.);

   if ((nitrogenFlux + dirabs) > 0.0)
   {
      rcetot = carbonFlux / (nitrogenFlux + dirabs);
   }

   if (rcetot < 15.0)
   {
      dirabs = (carbonFlux / 15.0) - nitrogenFlux;
      if (dirabs < 0.0)
      {
         dirabs = 0.0;
      }
   }

   return (dirabs);
}

double SOIL::calculateLigninFraction(UTILS utils, WEATHER weather, PARAMETER parameter, std::string typeOfPool)
{
   double ligninFraction;
   double param1, param2; // TODO: add as parameter

   if (typeOfPool == "surface")
   {
      param1 = 0.02 / 365.0;
      param2 = 0.0012;
   }
   else if (typeOfPool == "soil")
   {
      param1 = 0.26 / 365.0;
      param2 = 0.0015;
   }

   ligninFraction = param1 + (param2 * (weather.precipitation.at(parameter.day) / 10.0));

   double lowerLimit = 0.02 / 365.0;
   double upperLimit = 0.5 / 365.0;
   ligninFraction = std::max(lowerLimit, ligninFraction);
   ligninFraction = std::min(upperLimit, ligninFraction);

   return (ligninFraction);
}

double SOIL::calculateNitrogenFraction(UTILS utils, double dirabs, std::string typeOfPool)
{

   double fractionOfNitrogen;
   double carbonFlux, nitrogenFlux;

   if (typeOfPool == "surface")
   {
      carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
      nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
   }
   else if (typeOfPool == "soil")
   {
      carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
      nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
   }

   fractionOfNitrogen = (nitrogenFlux + dirabs) / (carbonFlux / carbonContentOdm);
   return (fractionOfNitrogen);
}

double SOIL::calculateFractionOfMetabolicLitter(UTILS utils, double fractionOfLignin, double ligninToNitrogenRatio, std::string type)
{
   double fractionOfMetabolicLitter;

   fractionOfMetabolicLitter = 0.85 - 0.013 * ligninToNitrogenRatio;

   if (fractionOfLignin > (1.0 - fractionOfMetabolicLitter))
   {
      fractionOfMetabolicLitter = (1.0 - fractionOfLignin);
   }

   // Make sure at least 1% goes to metabolic
   if (fractionOfMetabolicLitter < 0.20)
   {
      fractionOfMetabolicLitter = 0.20;
   }

   if (fractionOfMetabolicLitter < 0.0)
   {
      utils.handleError("Fraction of added carbon to metabolic litter pool is negative.");
   }

   return (fractionOfMetabolicLitter);
}

double SOIL::adjustLigninContentOfStructuralLitter(UTILS utils, double fractionOfLignin, double carbonAddedToStructuralLitter, double strlig, std::string typeOfPool)
{
   double adjustedFractionOfLignin;
   double carbonFlux, carbonPool;
   double ligninContent;

   if (typeOfPool == "surface")
   {
      carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
      carbonPool = carbonContent_surfaceStructuralLitterPool;
   }
   else if (typeOfPool == "soil")
   {
      carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
      carbonPool = carbonContent_soilStructuralLitterPool;
   }

   adjustedFractionOfLignin = fractionOfLignin / (carbonAddedToStructuralLitter / carbonFlux);
   if (adjustedFractionOfLignin > 1.0)
      adjustedFractionOfLignin = 1.0;

   double previousLigninContent = ligninContent * carbonPool;
   double newLigninContent = adjustedFractionOfLignin * carbonAddedToStructuralLitter;
   ligninContent = (previousLigninContent + newLigninContent) / (carbonPool + carbonAddedToStructuralLitter);

   return (ligninContent);
}

void SOIL::processLitterFluxes(UTILS utils, double dirabs, double carbonAddedToStructuralLitter, double carbonAddedToMetabolicLitter, double nitrogenAddedToStructuralLitter, double nitrogenAddedToMetabolicLitter, std::string typeOfPool)
{
   if (typeOfPool == "surface")
   {
      carbonContent_surfaceStructuralLitterPool += carbonAddedToStructuralLitter;
      carbonContent_surfaceMetabolicLitterPool += carbonAddedToMetabolicLitter;
      nitrogenContent_surfaceStructuralLitterPool += nitrogenAddedToStructuralLitter;
      nitrogenContent_surfaceMetabolicLitterPool += nitrogenAddedToMetabolicLitter;
   }
   else if (typeOfPool == "soil")
   {
      carbonContent_soilStructuralLitterPool += carbonAddedToStructuralLitter;
      carbonContent_soilMetabolicLitterPool += carbonAddedToMetabolicLitter;
      nitrogenContent_soilStructuralLitterPool += nitrogenAddedToStructuralLitter;
      nitrogenContent_soilMetabolicLitterPool += nitrogenAddedToMetabolicLitter;
   }

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -= dirabs;
   if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + tolerance < 0.0)
   {
      utils.handleError("Plants do not have access to soil nitrogen or soil nitrogen pool is negative!");
   }
}

void SOIL::doDecompositionFluxesInLitterAndSoilPools(UTILS utils)
{
   startDecomposition(utils, carbonContent_surfaceStructuralLitterPool, 3.9 / 365.0, ligninContent_surfaceStructuralLitterPool, "surface_structural");
   startDecomposition(utils, carbonContent_soilStructuralLitterPool, 4.9 / 365.0, ligninContent_soilStructuralLitterPool, "soil_structural");
   startDecomposition(utils, carbonContent_surfaceMetabolicLitterPool, 14.8 / 365.0, 1, "surface_metabolic");
   startDecomposition(utils, carbonContent_soilMetabolicLitterPool, 18.5 / 365.0, 1, "soil_metabolic");
   startDecomposition(utils, carbonContent_soilMicrobesPool, 6.0 / 365.0, 1, "microbes");
   startDecomposition(utils, carbonContent_soilActivePool, 7.3 / 365.0, 1, "active");
   startDecomposition(utils, carbonContent_soilSlowPool, 0.2 / 365.0, 1, "slow");
   startDecomposition(utils, carbonContent_soilPassivePool, 0.0045 / 365.0, 1, "passive");
}

void SOIL::startDecomposition(UTILS utils, double carbonContentOfPool, double constFactor, double lignin, std::string transferFromPool)
{
   const double maximumCarbonFlux = 5000.0; // in [g/m²/day]
   double potentialCarbonFlux, actualCarbonFlux;
   double textureFactor = 1.0;

   if (carbonContentOfPool > 0)
   {
      potentialCarbonFlux = carbonContentOfPool;

      // modify the potential carbon flux
      if (transferFromPool == "surface_structural" || transferFromPool == "soil_structural")
      {
         potentialCarbonFlux = std::min(carbonContentOfPool, maximumCarbonFlux); // TODO: why this upper limit?
         textureFactor = exp(-3.0 * lignin);                                     // factor between 0 and 1
      }

      if (transferFromPool == "active")
      {
         textureFactor = 0.25 + 0.75 * sandContent; // factor between 0 and 1
      }

      // calculate the actual carbon flux
      actualCarbonFlux = potentialCarbonFlux * decompositionFactor * constFactor * textureFactor;

      // calculate decisive CN ratios to decide on decomposition, nitrogen immobilization or mineralization
      calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, transferFromPool);

      // transfer carbon and nitrogen from one pool to another
      decompose(utils, actualCarbonFlux, lignin, transferFromPool);
   }
}

void SOIL::calculateDecisiveCarbonNitrogenRatiosForDecomposition(UTILS utils, std::string transferFromPool)
{
   // decisive CN ratios depend on daily dynamically changing state variables (e.g. mineral soil nitrogen content or carbon content of pool)
   double mineralNitrogenContentTopSoilLayer = nitrogenContent_soilMineralPoolPerSoilLayer.at(0);
   double factorMineralNitrogenTopSoilLayer = (1.0 - (mineralNitrogenContentTopSoilLayer / 2.0));

   if (transferFromPool == "surface_structural")
   {
      double biomassContent = soil.carbonContent_surfaceStructuralLitterPool / carbonContentOdm;
      double nitrogenContentInBiomass = (biomassContent > 0) ? (soil.nitrogenContent_surfaceStructuralLitterPool / biomassContent) : 0;

      // for decomposition of surface structural litter to soil microbes pool
      soil.decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool = std::min(16.0 + nitrogenContentInBiomass * ((10.0 - 16.0) / 0.02), 10.0);

      // for decomposition of surface structural litter to soil slow pool
      double auxillaryVariable = soil.decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool + (12.0 + 3.0 * (soil.decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool - 10.0));
      soil.decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool = std::max(5.0, auxillaryVariable);
   }
   else if (transferFromPool == "soil_structural")
   {
      // transfer to soil active pool
      soil.decisiveCNRatio_soilStructuralLitterPool_soilActivePool = 14.0;

      // transfer to soil slow pool
      soil.decisiveCNRatio_soilStructuralLitterPool_soilSlowPool = 20.0;
   }
   else if (transferFromPool == "surface_metabolic")
   {
      // transfer to soil microbes pool
      // decisive carbon nitrogen ratio is dependent on nitrogen content (% of biomass) of surface metabolic litter pool
      double biomassContent_surfaceMetabolicLitterPool = carbonContent_surfaceMetabolicLitterPool / carbonContentOdm;
      double nitrogenContentInBiomass = (biomassContent_surfaceMetabolicLitterPool > 0) ? (nitrogenContent_surfaceMetabolicLitterPool / biomassContent_surfaceMetabolicLitterPool) : 0;
      decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool = std::min(16.0 + nitrogenContentInBiomass * ((10.0 - 16.0) / 0.02), 10.0);
   }
   else if (transferFromPool == "soil_metabolic")
   {
      // transfer to soil active pool
      // decisive carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = 14.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = 3.0;
      }
      else
      {
         decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
      }
   }
   else if (transferFromPool == "microbes")
   {
      // transfer to soil slow pool
      // decisive carbon nitrogen ratio is dependent on cnRatio of contents in the pool and modifications
      double actualCNRatio_soilMicrobesPool = (carbonContent_soilMicrobesPool / nitrogenContent_soilMicrobesPool);
      double auxillaryVariable = actualCNRatio_soilMicrobesPool + 12.0 + 3.0 * (actualCNRatio_soilMicrobesPool - 10.0);
      decisiveCNRatio_soilMicrobesPool_soilSlowPool = std::max(auxillaryVariable, 5.0);
   }
   else if (transferFromPool == "active")
   {
      // transfer to soil slow pool
      // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilActivePool_soilSlowPool = 20.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilActivePool_soilSlowPool = 12.0;
      }
      else
      {
         decisiveCNRatio_soilActivePool_soilSlowPool = factorMineralNitrogenTopSoilLayer * (20.0 - 12.0) + 12.0;
      }

      // transfer from soil active to soil passive pool
      // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilActivePool_soilPassivePool = 8.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilActivePool_soilPassivePool = 6.0;
      }
      else
      {
         decisiveCNRatio_soilActivePool_soilPassivePool = factorMineralNitrogenTopSoilLayer * (8.0 - 6.0) + 6.0;
      }
   }
   else if (transferFromPool == "slow")
   {
      // transfer to soil active pool
      // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilSlowPool_soilActivePool = 14.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilSlowPool_soilActivePool = 3.0;
      }
      else
      {
         decisiveCNRatio_soilSlowPool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
      }

      // transfer to soil passive pool
      // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilSlowPool_soilPassivePool = 8.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilSlowPool_soilPassivePool = 6.0;
      }
      else
      {
         decisiveCNRatio_soilSlowPool_soilPassivePool = factorMineralNitrogenTopSoilLayer * (8.0 - 6.0) + 6.0;
      }
   }
   else if (transferFromPool == "passive")
   {
      // transfer to soil active pool
      // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
      if (mineralNitrogenContentTopSoilLayer <= 0)
      {
         decisiveCNRatio_soilPassivePool_soilActivePool = 14.0;
      }
      else if (mineralNitrogenContentTopSoilLayer > 2.0)
      {
         decisiveCNRatio_soilPassivePool_soilActivePool = 3.0;
      }
      else
      {
         decisiveCNRatio_soilPassivePool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
      }
   }
   else
   {
      utils.handleError("No correct type of pool provided.");
   }
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
double SOIL::calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, PARAMETER parameter)

{
   if (snowContent > 0.0)
   {
      soilTemperature = 0.0;
   }

   double temperatureFunction = (((atan(((soilTemperature - 15.4) + (2 * PI)) / (0.031 * 11.75 * 29.7))) + atan(0.031 * 29.7 * PI)) /
                                 (2 * atan(0.031 * 29.7 * PI)));

   double relativeWaterContent = (waterContent_soilWaterPoolPerLayer.at(0) - permanentWiltingPoint.at(0)) / (fieldCapacity.at(0) - permanentWiltingPoint.at(0));
   double waterFunction = (relativeWaterContent > 13.0) ? (1.0) : (1.0 / (1.0 + 4.0 * exp(-6.0 * relativeWaterContent)));
   waterFunction = std::min(waterFunction, 1.0);

   double factor = temperatureFunction * waterFunction;
   factor = std::max(factor, 0.0);

   return (factor);
}

bool SOIL::decomposable(UTILS utils, std::string typeOfPool)
{
   bool doDecomposition = true;

   // if there is not enough mineral nitrogen in the top soil layer AND the carbon-nitrogen ratio of a pool is exceeding an upper ratio limit, no decomposition is possible due to limited nitrogen resources
   if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) < 1e-07) // in [g/m²]
   {
      if (typeOfPool == "surface_structural")
      {
         if ((carbonContent_surfaceStructuralLitterPool / nitrogenContent_surfaceStructuralLitterPool) > decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "soil_structural")
      {
         if ((carbonContent_soilStructuralLitterPool / nitrogenContent_soilStructuralLitterPool) > decisiveCNRatio_soilStructuralLitterPool_soilActivePool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "surface_metabolic")
      {
         if ((carbonContent_surfaceMetabolicLitterPool / nitrogenContent_surfaceMetabolicLitterPool) > decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "soil_metabolic")
      {
         if ((carbonContent_soilMetabolicLitterPool / nitrogenContent_soilMetabolicLitterPool) > decisiveCNRatio_soilMetabolicLitterPool_soilActivePool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "microbes")
      {
         if ((carbonContent_soilMicrobesPool / nitrogenContent_soilMicrobesPool) > decisiveCNRatio_soilMicrobesPool_soilSlowPool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "active")
      {
         // soil active pool is decomposed to slow and passive pool
         // here, for the decision, the decisive CN ratio to the slow pool is used
         if ((carbonContent_soilActivePool / nitrogenContent_soilActivePool) > decisiveCNRatio_soilActivePool_soilSlowPool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "slow")
      {
         // soil slow pool is decomposed to active and passive pool
         // here, for the decision, the decisive CN ratio to the active pool is used
         if ((carbonContent_soilSlowPool / nitrogenContent_soilSlowPool) > decisiveCNRatio_soilSlowPool_soilActivePool)
         {
            doDecomposition = false;
         }
      }
      else if (typeOfPool == "passive")
      {
         if ((carbonContent_soilPassivePool / nitrogenContent_soilPassivePool) > decisiveCNRatio_soilPassivePool_soilActivePool)
         {
            doDecomposition = false;
         }
      }
   }

   return (doDecomposition);
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
bool SOIL::decompose(UTILS utils, double carbonFlux, double ligninContent, std::string transferFromPool)
{
   double compareRatio = 1;
   std::string transferToPool;
   double nitrogenContentTopSoilLayer = nitrogenContent_soilMineralPoolPerSoilLayer.at(0);

   if (decomposable(utils, transferFromPool))
   {
      if (transferFromPool == "surface_structural")
      {
         // ******* to soil slow pool ********
         transferToPool = "slow";

         // determine carbon flux
         carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool = carbonFlux * ligninContent;

         // subtract carbon respiration from carbon flux
         // calculate respiratory nitrogen flow proportional to carbon respiration (based on actual CN ratio of origin pool)
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // determine nitrogen flux proportional to remaining carbon flux (based on actual CN ratio of origin pool)
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         // save nitrogen flux in extra state variable for storage (as nitrogenFlux will be changed in case of mineralization)
         nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool = nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;
         // decision based on actual CN ratio of fluxes compared to decisiveCNRatio
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool,
                                        nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool, decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool,
                                        transferFromPool, transferToPool);

         // ******  to mirobial soil pool *******
         transferToPool = "microbes";
         // respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool has already been subtracted from carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool in previous call of calculateRespirationOfDecomposition
         // therefore, it needs to be accounted here to calculate the remaining carbon flux transferred to the microbes pool
         carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = carbonFlux - (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool);
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from litter to soil microbial pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);
         nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool = nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool,
                                        nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool, decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool,
                                        transferFromPool, transferToPool);
      }

      // -----------------------------------------------------
      if (transferFromPool == "soil_structural")
      {
         //*** to slow soil pool ****
         transferToPool = "slow";
         carbonFlux_soilStructuralLitterPool_to_soilSlowPool = carbonFlux * ligninContent;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from litter to soil slow pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool = nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilStructuralLitterPool_to_soilSlowPool,
                                        nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool,
                                        decisiveCNRatio_soilStructuralLitterPool_soilSlowPool, transferFromPool, transferToPool);

         // **** to active soil pool ****
         transferToPool = "active";
         carbonFlux_soilStructuralLitterPool_to_soilActivePool =
             carbonFlux - carbonFlux_soilStructuralLitterPool_to_soilSlowPool - respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from litter to soil active pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilStructuralLitterPool_to_soilActivePool = nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilStructuralLitterPool_to_soilActivePool,
                                        nitrogenFlow_soilStructuralLitterPool_to_soilActivePool,
                                        decisiveCNRatio_soilStructuralLitterPool_soilActivePool, transferFromPool, transferToPool);
      }

      // ---------------------------------------------------------
      if (transferFromPool == "surface_metabolic")
      {
         transferToPool = "microbes";
         carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = std::min(carbonFlux, carbonContent_surfaceMetabolicLitterPool);
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from litter to microbial pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool = nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool,
                                        nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool,
                                        decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool, transferFromPool, transferToPool);
      }

      // ------------------------------------------------
      if (transferFromPool == "soil_metabolic")
      {
         transferToPool = "active";
         carbonFlux_soilMetabolicLitterPool_to_soilActivePool = std::min(carbonFlux, carbonContent_soilMetabolicLitterPool);
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from litter to soil active pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool = nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilMetabolicLitterPool_to_soilActivePool,
                                        nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool,
                                        decisiveCNRatio_soilMetabolicLitterPool_soilActivePool, transferFromPool, transferToPool);
      }

      // -------------------------------------------
      if (transferFromPool == "microbes")
      {
         transferToPool = "slow";
         carbonFlux_soilMicrobesPool_to_soilSlowPool = carbonFlux;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from microbes to soil slow pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilMicrobesPool_to_soilSlowPool = nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilMicrobesPool_to_soilSlowPool,
                                        nitrogenFlow_soilMicrobesPool_to_soilSlowPool,
                                        decisiveCNRatio_soilMicrobesPool_soilSlowPool, transferFromPool, transferToPool);
      }

      // -------------------------------------------
      if (transferFromPool == "active")
      {
         transferToPool = "slow_passive";
         carbonFlux_soilActivePool_to_soilPassiveAndSlowPool = carbonFlux;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // -------------- flux to passive soil pool incl. leaching
         transferToPool = "passive";
         carbonFlux_soilActivePool_to_soilPassivePool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool * (0.003 + 0.032 * clayContent);

         // proportional nitrogen flow from microbes to soil slow pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilActivePool_to_soilPassivePool = nitrogenFlux_soilActivePool_to_soilPassivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilActivePool_to_soilPassivePool,
                                        nitrogenFlow_soilActivePool_to_soilPassivePool,
                                        decisiveCNRatio_soilActivePool_soilPassivePool, transferFromPool, transferToPool);

         // leaching
         doLeaching();

         // -------------- flux to slow soil pool
         transferToPool = "slow";
         carbonFlux_soilActivePool_to_soilSlowPool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool - respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool -
                                                     carbonFlux_soilActivePool_to_soilPassivePool - carbonLeaching;

         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilActivePool_to_soilSlowPool = nitrogenFlux_soilActivePool_to_soilSlowPool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilActivePool_to_soilSlowPool,
                                        nitrogenFlow_soilActivePool_to_soilSlowPool,
                                        decisiveCNRatio_soilActivePool_soilSlowPool, transferFromPool, transferToPool);
      }

      // -------------------------------------------
      if (transferFromPool == "slow")
      {
         transferToPool = "active_passive";
         carbonFlux_soilSlowPool_to_soilPassiveAndActivePool = carbonFlux;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // --------- to passive pool
         transferToPool = "passive";
         carbonFlux_soilSlowPool_to_soilPassivePool = carbonFlux_soilSlowPool_to_soilPassiveAndActivePool * (0.003 + 0.009 * clayContent);

         // proportional nitrogen flow from soil slow pool to passive pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilSlowPool_to_soilPassivePool = nitrogenFlux_soilSlowPool_to_soilPassivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilSlowPool_to_soilPassivePool,
                                        nitrogenFlow_soilSlowPool_to_soilPassivePool,
                                        decisiveCNRatio_soilSlowPool_soilPassivePool, transferFromPool, transferToPool);

         // --------- to active pool
         // proportional nitrogen flow from soil slow pool to active pool
         transferToPool = "active";
         carbonFlux_soilSlowPool_to_soilActivePool =
             carbonFlux_soilSlowPool_to_soilPassiveAndActivePool - respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool - carbonFlux_soilSlowPool_to_soilPassivePool;

         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilSlowPool_to_soilActivePool = nitrogenFlux_soilSlowPool_to_soilActivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilSlowPool_to_soilActivePool,
                                        nitrogenFlow_soilSlowPool_to_soilActivePool,
                                        decisiveCNRatio_soilSlowPool_soilActivePool, transferFromPool, transferToPool);
      }

      // -------------------------------------------
      if (transferFromPool == "passive")
      {
         transferToPool = "active";
         carbonFlux_soilPassivePool_to_soilActivePool = carbonFlux;
         calculateRespirationOfDecomposition(utils, transferFromPool, transferToPool);

         // proportional nitrogen flow from soil passive pool to active pool
         determineNitrogenFlux(utils, transferFromPool, transferToPool);

         nitrogenFlow_soilPassivePool_to_soilActivePool = nitrogenFlux_soilPassivePool_to_soilActivePool;
         immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilPassivePool_to_soilActivePool,
                                        nitrogenFlow_soilPassivePool_to_soilActivePool,
                                        decisiveCNRatio_soilPassivePool_soilActivePool, transferFromPool, transferToPool);
      }
   }
   return false;
}

void SOIL::calculateRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool)
{
   // carbon respiration asscociated with decomposition
   calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

   // nitrogen flow asscociated with carbon respiration of decomposition (proportional to CN ratio of origin pool)
   calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::calculateCarbonRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool)
{
   // respiratory carbon flow for the transfer from active or slow soil pools is calculated jointly for both fluxes (to slow & passive, to active and passive)
   if (transferFromPool == "surface_structural")
   {
      if (transferToPool == "slow")
      {
         respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool = carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool * 0.3;
         carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool -= respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool;
      }
      else if (transferToPool == "microbes")
      {
         respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool = carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool * 0.45;
         carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool -= respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_structural")
   {
      if (transferToPool == "slow")
      {
         respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool = carbonFlux_soilStructuralLitterPool_to_soilSlowPool * 0.3;
         carbonFlux_soilStructuralLitterPool_to_soilSlowPool -= respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;
      }
      else if (transferToPool == "active")
      {
         respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool = carbonFlux_soilStructuralLitterPool_to_soilActivePool 0.55;
         carbonFlux_soilStructuralLitterPool_to_soilActivePool -= respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool;
      }
   }
   else if (transferFromPool == "surface_metabolic")
   {
      if (transferToPool == "microbes")
      {
         respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool = carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool * 0.55;
         carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool -= respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_metabolic")
   {
      if (transferToPool == "active")
      {
         respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool = carbonFlux_soilMetabolicLitterPool_to_soilActivePool * 0.55;
         carbonFlux_soilMetabolicLitterPool_to_soilActivePool -= respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;
      }
   }
   else if (transferFromPool == "microbes")
   {
      if (transferToPool == "slow")
      {
         respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool = carbonFlux_soilMicrobesPool_to_soilSlowPool * 0.6;
         carbonFlux_soilMicrobesPool_to_soilSlowPool -= respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;
      }
   }
   else if (transferFromPool == "active")
   {
      if (transferToPool == "slow_passive")
      {
         respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool * (0.17 + 0.68 * sandContent);
      }
   }
   else if (transferFromPool == "slow")
   {
      if (transferToPool == "active_passive")
      {
         respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool = carbonFlux_soilSlowPool_to_soilPassiveAndActivePool * 0.55;
      }
   }
   else if (transferFromPool == "passive")
   {
      if (transferToPool == "active")
      {
         respiration_decompositionCarbon_soilPassivePool_soilActivePool = carbonFlux_soilPassivePool_to_soilActivePool * 0.55;
         carbonFlux_soilPassivePool_to_soilActivePool -= respiration_decompositionCarbon_soilPassivePool_soilActivePool;
      }
   }
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::calculateNitrogenRespirationOfDecomposition(UTILS utils, std::string transferPoolFrom, std::string transferPoolTo)
{
   // respiratory nitrogen flow for the transfer from active or slow soil pools is calculated jointly for both fluxes (to slow & passive, to active and passive)
   double actualCNRatioOfPool = 1;
   double respirationNitrogen = 0.0;

   if (transferPoolFrom == "surface_structural")
   {
      actualCNRatioOfPool = (nitrogenContent_surfaceStructuralLitterPool / carbonContent_surfaceStructuralLitterPool);

      if (transferPoolTo == "slow")
      {
         respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool = respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool;
      }
      else if (transferPoolTo == "microbes")
      {
         respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool = respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool;
      }
   }
   else if (transferPoolFrom == "soil_structural")
   {
      actualCNRatioOfPool = (nitrogenContent_soilStructuralLitterPool / carbonContent_soilStructuralLitterPool);

      if (transferPoolTo == "slow")
      {
         respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool = respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool;
      }
      else if (transferPoolTo == "active")
      {
         respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool = respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool;
      }
   }
   else if (transferPoolFrom == "surface_metabolic")
   {
      actualCNRatioOfPool = (nitrogenContent_surfaceMetabolicLitterPool / carbonContent_surfaceMetabolicLitterPool);

      if (transferPoolTo == "microbes")
      {
         respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool = respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;
      }
   }
   else if (transferPoolFrom == "soil_metabolic")
   {
      actualCNRatioOfPool = (nitrogenContent_soilMetabolicLitterPool / carbonContent_soilMetabolicLitterPool);

      if (transferPoolTo == "active")
      {
         respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool = respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;
      }
   }
   else if (transferPoolFrom == "microbes")
   {
      actualCNRatioOfPool = (nitrogenContent_soilMicrobesPool / carbonContent_soilMicrobesPool);

      if (transferPoolTo == "slow")
      {
         respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool = respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;
      }
   }
   else if (transferPoolFrom == "active")
   {
      if (transferPoolTo = "slow_passive")
      {
         actualCNRatioOfPool = (nitrogenContent_soilActivePool / carbonContent_soilActivePool);

         respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool = respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;
      }
   }
   else if (transferPoolFrom == "slow")
   {
      if (transferPoolTo = "active_passive")
      {
         actualCNRatioOfPool = (nitrogenContent_soilSlowPool / carbonContent_soilSlowPool);

         respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool = respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;
      }
   }
   else if (transferPoolFrom == "passive")
   {
      if (transferPoolTo = "active")
      {
         actualCNRatioOfPool = (nitrogenContent_soilPassivePool / carbonContent_soilPassivePool);

         respiration_decompositionNitrogen_soilPassivePool_soilActivePool = respiration_decompositionCarbon_soilPassivePool_soilActivePool * actualCNRatioOfPool;
         respirationNitrogen = respiration_decompositionNitrogen_soilPassivePool_soilActivePool;
      }
   }

   // added to mineralization rates as respiratory nitrogen fluxes will be added to soil mineral nitrogen pool later
   nitrogenGrossMineralization += respirationNitrogen;
   nitrogenNetMineralization += respirationNitrogen;
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::determineNitrogenFlux(UTILS utils, std::string transferFromPool, std::string transferToPool)
{
   if (transferFromPool == "surface_structural")
   {
      if (transferToPool == "microbes")
      {
         nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = nitrogenContent_surfaceStructuralLitterPool * (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / carbonContent_surfaceStructuralLitterPool);
      }
      if (transferToPool == "slow")
      {
         nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = nitrogenContent_surfaceStructuralLitterPool * (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / carbonContent_surfaceStructuralLitterPool);
      }
   }
   else if (transferFromPool == "soil_structural")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = nitrogenContent_soilStructuralLitterPool * (carbonFlux_soilStructuralLitterPool_to_soilActivePool / carbonContent_soilStructuralLitterPool);
      }
      if (transferToPool == "slow")
      {
         nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = nitrogenContent_soilStructuralLitterPool * (carbonFlux_soilStructuralLitterPool_to_soilSlowPool / carbonContent_soilStructuralLitterPool);
      }
   }
   else if (transferFromPool == "surface_metabolic")
   {
      if (transferToPool == "microbes")
      {
         nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = nitrogenContent_surfaceMetabolicLitterPool * (carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / carbonContent_surfaceMetabolicLitterPool);
      }
   }
   else if (transferFromPool == "soil_metabolic")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = nitrogenContent_soilMetabolicLitterPool * (carbonFlux_soilMetabolicLitterPool_to_soilActivePool / carbonContent_soilMetabolicLitterPool);
      }
   }
   else if (transferFromPool == "microbes")
   {
      if (transferToPool == "slow")
      {
         nitrogenFlux_soilMicrobesPool_to_soilSlowPool = nitrogenContent_soilMicrobesPool * (carbonFlux_soilMicrobesPool_to_soilSlowPool / carbonContent_soilMicrobesPool);
      }
   }
   else if (transferFromPool == "active")
   {
      if (transferToPool == "slow")
      {
         nitrogenFlux_soilActivePool_to_soilSlowPool = nitrogenContent_soilActivePool * (carbonFlux_soilActivePool_to_soilSlowPool / carbonContent_soilActivePool);
      }
      if (transferToPool == "passive")
      {
         nitrogenFlux_soilActivePool_to_soilPassivePool = nitrogenContent_soilActivePool * (carbonFlux_soilActivePool_to_soilPassivePool / carbonContent_soilActivePool);
      }
   }
   else if (transferFromPool == "slow")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilSlowPool_to_soilActivePool = nitrogenContent_soilSlowPool * (carbonFlux_soilSlowPool_to_soilActivePool / carbonContent_soilSlowPool);
      }
      if (transferToPool == "passive")
      {
         nitrogenFlux_soilSlowPool_to_soilPassivePool = nitrogenContent_soilSlowPool * (carbonFlux_soilSlowPool_to_soilPassivePool / carbonContent_soilSlowPool);
      }
   }
   else if (transferFromPool == "passive")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilPassivePool_to_soilActivePool = nitrogenContent_soilPassivePool * (carbonFlux_soilPassivePool_to_soilActivePool / carbonContent_soilPassivePool);
      }
   }
}

void SOIL::immobilizeOrMineralizeNitrogen(UTILS utils, double carbonFlux, double nitrogenFlow, double decisiveCNratio, std::string transferFromPool, std::string transferToPool)
{
   double mineralize_fromPool_toPool, immobilize_fromPool_toPool;

   if (carbonFlux > 0.0 && nitrogenFlow > 0.0)
   {
      double actualCNRatioOfFluxes = carbonFlux / nitrogenFlow;

      if (actualCNRatioOfFluxes > decisiveCNratio)
      { // immobilization occurs
         // nitrogen resources are required from soil mineral nitrogen pool for decomposition
         // and will be used from top soil mineral nitrogen pool
         immobilizeNitrogen(utils, transferFromPool, transferToPool, decisiveCNratio);
      }
      else
      { // mineralization occurs as sufficient nitrogen resources are available in the material itself for decomposition
         // nitrogenFlux state variable will be changed (reduced to amount that is required for decomposition only)
         // nitrogen surplus will be added to soil mineral nitrogen pool
         mineralizeNitrogen(utils, transferFromPool, transferToPool, decisiveCNratio, nitrogenFlow);
      }
   }
   else
   {
      utils.handleError("Carbon or nitrogen fluxes are negative! No CN ratio can be calculated for deciding on immobilization or mineralization.");
   }
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::immobilizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio)
{
   if (transferFromPool == "surface_structural")
   {
      if (transferToPool == "slow")
      {
         immobilize_surfaceStructuralLitterPool_to_soilSlowPool = (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;
         nitrogenNetMineralization -= immobilize_surfaceStructuralLitterPool_to_soilSlowPool;
      }

      if (transferToPool == "microbes")
      {
         immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool = (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / decisiveCNratio) - nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
         nitrogenNetMineralization -= immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_structural")
   {
      if (transferToPool == "slow")
      {
         immobilize_soilStructuralLitterPool_to_soilSlowPool = (carbonFlux_soilStructuralLitterPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
         nitrogenNetMineralization -= immobilize_soilStructuralLitterPool_to_soilSlowPool;
      }

      if (transferToPool == "active")
      {
         immobilize_soilStructuralLitterPool_to_soilActivePool = (carbonFlux_soilStructuralLitterPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
         nitrogenNetMineralization -= immobilize_soilStructuralLitterPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "surface_metabolic")
   {
      if (transferToPool == "microbes")
      {
         immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool = (carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / decisiveCNratio) - nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
         nitrogenNetMineralization -= immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_metabolic")
   {
      if (transferToPool == "active")
      {
         immobilize_soilMetabolicLitterPool_to_soilActivePool = (carbonFlux_soilMetabolicLitterPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
         nitrogenNetMineralization -= immobilize_soilMetabolicLitterPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "microbes")
   {
      if (transferToPool == "slow")
      {
         immobilize_soilMicrobesPool_to_soilSlowPool = (carbonFlux_soilMicrobesPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
         nitrogenNetMineralization -= immobilize_soilMicrobesPool_to_soilSlowPool;
      }
   }
   else if (transferFromPool == "active")
   {
      if (transferToPool == "passive")
      {
         immobilize_soilActivePool_to_soilPassivePool = (carbonFlux_soilActivePool_to_soilPassivePool / decisiveCNratio) - nitrogenFlux_soilActivePool_to_soilPassivePool;
         nitrogenNetMineralization -= immobilize_soilActivePool_to_soilPassivePool;
      }

      if (transferToPool == "slow")
      {
         immobilize_soilActivePool_to_soilSlowPool = (carbonFlux_soilActivePool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilActivePool_to_soilSlowPool;
         nitrogenNetMineralization -= immobilize_soilActivePool_to_soilSlowPool;
      }
   }
   else if (transferFromPool == "slow")
   {
      if (transferToPool == "passive")
      {
         immobilize_soilSlowPool_to_soilPassivePool = (carbonFlux_soilSlowPool_to_soilPassivePool / decisiveCNratio) - nitrogenFlux_soilSlowPool_to_soilPassivePool;
         nitrogenNetMineralization -= immobilize_soilSlowPool_to_soilPassivePool;
      }

      if (transferToPool == "active")
      {
         immobilize_soilSlowPool_to_soilActivePool = (carbonFlux_soilSlowPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilSlowPool_to_soilActivePool;
         nitrogenNetMineralization -= immobilize_soilSlowPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "passive")
   {
      if (transferToPool == "active")
      {
         immobilize_soilPassivePool_to_soilActivePool = (carbonFlux_soilPassivePool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilPassivePool_to_soilActivePool;
         nitrogenNetMineralization -= immobilize_soilPassivePool_to_soilActivePool;
      }
   }
}

/**
 * @cite Function and code has been reused from the CENTURY4.0 soil model
 */
void SOIL::mineralizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio, double previousNitrogenFlow)
{
   if (transferFromPool == "surface_structural")
   {
      if (transferToPool == "slow")
      {
         // nitrogenFlux is reduced to only required amounts for decomposition
         nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / decisiveCNratio;

         // remaining nitrogen can be mineralized
         mineralize_surfaceStructuralLitterPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;

         nitrogenGrossMineralization += mineralize_surfaceStructuralLitterPool_to_soilSlowPool;
         nitrogenNetMineralization += mineralize_surfaceStructuralLitterPool_to_soilSlowPool;
      }
      if (transferToPool == "microbes")
      {
         nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / decisiveCNratio;
         mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool = previousNitrogenFlow - nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
         nitrogenGrossMineralization += mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool;
         nitrogenNetMineralization += mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_structural")
   {
      if (transferToPool == "slow")
      {
         nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = carbonFlux_soilStructuralLitterPool_to_soilSlowPool / decisiveCNratio;
         mineralize_soilStructuralLitterPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
         nitrogenGrossMineralization += mineralize_soilStructuralLitterPool_to_soilSlowPool;
         nitrogenNetMineralization += mineralize_soilStructuralLitterPool_to_soilSlowPool;
      }
      if (transferToPool == "active")
      {
         nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = carbonFlux_soilStructuralLitterPool_to_soilActivePool / decisiveCNratio;
         mineralize_soilStructuralLitterPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
         nitrogenGrossMineralization += mineralize_soilStructuralLitterPool_to_soilActivePool;
         nitrogenNetMineralization += mineralize_soilStructuralLitterPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "surface_metabolic")
   {
      if (transferToPool == "microbes")
      {
         nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / decisiveCNratio;
         mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool = previousNitrogenFlow - nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
         nitrogenGrossMineralization += mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
         nitrogenNetMineralization += mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
      }
   }
   else if (transferFromPool == "soil_metabolic")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = carbonFlux_soilMetabolicLitterPool_to_soilActivePool / decisiveCNratio;
         mineralize_soilMetabolicLitterPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
         nitrogenGrossMineralization += mineralize_soilMetabolicLitterPool_to_soilActivePool;
         nitrogenNetMineralization += mineralize_soilMetabolicLitterPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "microbes")
   {
      if (transferToPool == "slow")
      {
         nitrogenFlux_soilMicrobesPool_to_soilSlowPool = carbonFlux_soilMicrobesPool_to_soilSlowPool / decisiveCNratio;
         mineralize_soilMicrobesPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
         nitrogenGrossMineralization += mineralize_soilMicrobesPool_to_soilSlowPool;
         nitrogenNetMineralization += mineralize_soilMicrobesPool_to_soilSlowPool;
      }
   }
   else if (transferFromPool == "active")
   {
      if (transferToPool == "passive")
      {
         nitrogenFlux_soilActivePool_to_soilPassivePool = carbonFlux_soilActivePool_to_soilPassivePool / decisiveCNratio;
         mineralize_soilActivePool_to_soilPassivePool = previousNitrogenFlow - nitrogenFlux_soilActivePool_to_soilPassivePool;
         nitrogenGrossMineralization += mineralize_soilActivePool_to_soilPassivePool;
         nitrogenNetMineralization += mineralize_soilActivePool_to_soilPassivePool;
      }

      if (transferToPool == "slow")
      {
         nitrogenFlux_soilActivePool_to_soilSlowPool = carbonFlux_soilActivePool_to_soilSlowPool / decisiveCNratio;
         mineralize_soilActivePool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilActivePool_to_soilSlowPool;
         nitrogenGrossMineralization += mineralize_soilActivePool_to_soilSlowPool;
         nitrogenNetMineralization += mineralize_soilActivePool_to_soilSlowPool;
      }
   }
   else if (transferFromPool == "slow")
   {
      if (transferToPool == "passive")
      {
         nitrogenFlux_soilSlowPool_to_soilPassivePool = carbonFlux_soilSlowPool_to_soilPassivePool / decisiveCNratio;
         mineralize_soilSlowPool_to_soilPassivePool = previousNitrogenFlow - nitrogenFlux_soilSlowPool_to_soilPassivePool;
         nitrogenGrossMineralization += mineralize_soilSlowPool_to_soilPassivePool;
         nitrogenNetMineralization += mineralize_soilSlowPool_to_soilPassivePool;
      }

      if (transferToPool == "active")
      {
         nitrogenFlux_soilSlowPool_to_soilActivePool = carbonFlux_soilSlowPool_to_soilActivePool / decisiveCNratio;
         mineralize_soilSlowPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilSlowPool_to_soilActivePool;
         nitrogenGrossMineralization += mineralize_soilSlowPool_to_soilActivePool;
         nitrogenNetMineralization += mineralize_soilSlowPool_to_soilActivePool;
      }
   }
   else if (transferFromPool == "passive")
   {
      if (transferToPool == "active")
      {
         nitrogenFlux_soilPassivePool_to_soilActivePool = carbonFlux_soilPassivePool_to_soilActivePool / decisiveCNratio;
         mineralize_soilPassivePool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilPassivePool_to_soilActivePool;
         nitrogenGrossMineralization += mineralize_soilPassivePool_to_soilActivePool;
         nitrogenNetMineralization += mineralize_soilPassivePool_to_soilActivePool;
      }
   }
}

void SOIL::updateSoilPoolsByRespirationAndFluxes(UTILS utils)
{

   // ############## Respiration ####################
   // carbon respiration: subtracted from pools and added to cumulative output variables
   carbonContent_surfaceStructuralLitterPool -= (respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool);
   respirationCarbon_surface_litter += (respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool);

   carbonContent_surfaceMetabolicLitterPool -= respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;
   respirationCarbon_surface_litter += respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;

   carbonContent_soilStructuralLitterPool -= (respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool);
   respirationCarbon_soil_litter += (respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool);

   carbonContent_soilMetabolicLitterPool -= respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;
   respirationCarbon_soil_litter += respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;

   respirationCarbon_litter += respirationCarbon_soil_litter + respirationCarbon_surface_litter;

   carbonContent_soilMicrobesPool -= respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;
   carbonContent_soilActivePool -= respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool;
   carbonContent_soilSlowPool -= respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool;
   carbonContent_soilPassivePool -= respiration_decompositionCarbon_soilPassivePool_soilActivePool;

   respirationCarbon_soilpools += (respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool + respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool +
                                   respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool + respiration_decompositionCarbon_soilPassivePool_soilActivePool);

   // nitrogen respiratory fluxes: subtracted from pools and added to cumulative output variables
   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) +=
       (respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool +
        respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool +
        respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool +
        respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool);

   nitrogenContent_surfaceStructuralLitterPool -= (respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool);
   nitrogenContent_surfaceMetabolicLitterPool -= respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;

   nitrogenContent_soilStructuralLitterPool -= (respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool);
   nitrogenContent_soilMetabolicLitterPool -= respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += (respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool + respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool +
                                                         respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool + respiration_decompositionNitrogen_soilPassivePool_soilActivePool);
   nitrogenContent_soilActivePool -= respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;
   nitrogenContent_soilMicrobesPool -= respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;
   nitrogenContent_soilSlowPool -= respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;
   nitrogenContent_soilPassivePool -= respiration_decompositionNitrogen_soilPassivePool_soilActivePool;

   // ############## Fluxes between soil pools ####################
   // carbon fluxes: added and subtracted to/from pools
   carbonContent_soilSlowPool += (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + carbonFlux_soilStructuralLitterPool_to_soilSlowPool);
   carbonContent_soilMicrobesPool += (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool + carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool);
   carbonContent_soilActivePool += (carbonFlux_soilStructuralLitterPool_to_soilActivePool + carbonFlux_soilMetabolicLitterPool_to_soilActivePool);

   carbonContent_surfaceStructuralLitterPool -= (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool);
   carbonContent_soilStructuralLitterPool -= (carbonFlux_soilStructuralLitterPool_to_soilSlowPool + carbonFlux_soilStructuralLitterPool_to_soilActivePool);
   carbonContent_surfaceMetabolicLitterPool -= carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
   carbonContent_soilMetabolicLitterPool -= carbonFlux_soilMetabolicLitterPool_to_soilActivePool;

   carbonContent_soilSlowPool += (carbonFlux_soilMicrobesPool_to_soilSlowPool + carbonFlux_soilActivePool_to_soilSlowPool);
   carbonContent_soilSlowPool -= (carbonFlux_soilSlowPool_to_soilActivePool + carbonFlux_soilSlowPool_to_soilPassivePool);

   carbonContent_soilPassivePool += (carbonFlux_soilActivePool_to_soilPassivePool + carbonFlux_soilSlowPool_to_soilPassivePool);
   carbonContent_soilPassivePool -= carbonFlux_soilPassivePool_to_soilActivePool;

   carbonContent_soilActivePool += (carbonFlux_soilSlowPool_to_soilActivePool + carbonFlux_soilPassivePool_to_soilActivePool);
   carbonContent_soilActivePool -= (carbonFlux_soilActivePool_to_soilSlowPool + carbonFlux_soilActivePool_to_soilPassivePool + carbonLeaching);

   carbonContent_soilMicrobesPool -= carbonFlux_soilMicrobesPool_to_soilSlowPool;

   LeachingC += carbonLeaching;

   // nitrogen fluxes: added and subtracted to/from pools
   // nitrogenFlow is subtracted from pools (contains nitrogenFlux + mineralizableNitrogen in case of mineralization; in case of immobilization nitrogenFlow = nitrogenFlux)
   // nitrogenFlux is added to soil pools (proportional to added carbon; might be reduced in case of mineralization; in case of immobilization nitrogenFlow = nitrogenFlux)
   nitrogenContent_surfaceStructuralLitterPool -= (nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool + nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool);
   nitrogenContent_soilStructuralLitterPool -= nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool + nitrogenFlow_soilStructuralLitterPool_to_soilActivePool;
   nitrogenContent_surfaceMetabolicLitterPool -= nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool;
   nitrogenContent_soilMetabolicLitterPool -= nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool;

   nitrogenContent_soilSlowPool += (nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool + nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool);
   nitrogenContent_soilSlowPool += (nitrogenFlux_soilMicrobesPool_to_soilSlowPool + nitrogenFlux_soilActivePool_to_soilSlowPool);
   nitrogenContent_soilSlowPool -= (nitrogenFlow_soilSlowPool_to_soilActivePool + nitrogenFlow_soilSlowPool_to_soilPassivePool);

   nitrogenContent_soilPassivePool += (nitrogenFlux_soilActivePool_to_soilPassivePool + nitrogenFlux_soilSlowPool_to_soilPassivePool);
   nitrogenContent_soilPassivePool -= nitrogenFlow_soilPassivePool_to_soilActivePool;

   nitrogenContent_soilActivePool += (nitrogenFlux_soilStructuralLitterPool_to_soilActivePool + nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool);
   nitrogenContent_soilActivePool += (nitrogenFlux_soilSlowPool_to_soilActivePool + nitrogenFlux_soilPassivePool_to_soilActivePool);
   nitrogenContent_soilActivePool -= (nitrogenFlow_soilActivePool_to_soilSlowPool + nitrogenFlow_soilActivePool_to_soilPassivePool + NLeach);

   nitrogenContent_soilMicrobesPool += (nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool + nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool);
   nitrogenContent_soilMicrobesPool -= nitrogenFlow_soilMicrobesPool_to_soilSlowPool;

   LeachingN += NLeach;

   // ############## Immobilization/Mineralization ####################
   // either immobilization or mineralization occurs per soil pool flux
   // if immobilization occurs, nitrogen is added to the target soil pool (n)
   // and subtracted from mineral N pool in upper soil layer
   // (i.e. miner_... is negative immobilization rate)
   // if mineralization occurs, immob_... is zero

   nitrogenContent_soilSlowPool += (immobilize_surfaceStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilSlowPool +
                                    immobilize_soilMicrobesPool_to_soilSlowPool + immobilize_soilActivePool_to_soilSlowPool);

   nitrogenContent_soilMicrobesPool += (immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool + immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool);

   nitrogenContent_soilActivePool += (immobilize_soilMetabolicLitterPool_to_soilActivePool + immobilize_soilPassivePool_to_soilActivePool +
                                      immobilize_soilSlowPool_to_soilActivePool + immobilize_soilStructuralLitterPool_to_soilActivePool);

   nitrogenContent_soilPassivePool += (immobilize_soilActivePool_to_soilPassivePool + immobilize_soilSlowPool_to_soilPassivePool);

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -=
       (immobilize_surfaceStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilActivePool +
        immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool + immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool + immobilize_soilMetabolicLitterPool_to_soilActivePool +
        immobilize_soilMicrobesPool_to_soilSlowPool + immobilize_soilActivePool_to_soilSlowPool + immobilize_soilPassivePool_to_soilActivePool +
        immobilize_soilSlowPool_to_soilActivePool + immobilize_soilActivePool_to_soilPassivePool + immobilize_soilSlowPool_to_soilPassivePool);

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) +=
       (mineralize_surfaceStructuralLitterPool_to_soilSlowPool + mineralize_soilStructuralLitterPool_to_soilSlowPool +
        mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool + mineralize_soilStructuralLitterPool_to_soilActivePool +
        mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool + mineralize_soilMetabolicLitterPool_to_soilActivePool +
        mineralize_soilActivePool_to_soilPassivePool + mineralize_soilActivePool_to_soilSlowPool +
        mineralize_soilMicrobesPool_to_soilSlowPool + mineralize_soilSlowPool_to_soilPassivePool +
        mineralize_soilSlowPool_to_soilActivePool + mineralize_soilPassivePool_to_soilActivePool);
}

void SOIL::calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(UTILS utils, PARAMETER parameter, WEATHER weather)
{
   double nonsymbioticNitrogenFixation;
   double athomsphericDeposition;

   nonsymbioticNitrogenFixation = 0;
   athomsphericDeposition = 0.01 * (weather.potEvapoTranspiration.at(parameter.day) - (30.0 / 365.0));
   athomsphericDeposition = std::max(athomsphericDeposition, 0.0);

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += (athomsphericDeposition + nonsymbioticNitrogenFixation);
   nitrogenFixationToSoil += (athomsphericDeposition + nonsymbioticNitrogenFixation);

   if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + tolerance < 0.0)
   {
      nitrogenContent_soilMineralPoolPerSoilLayer.at(0) = 0.0;
      utils.handleError("Soil mineral nitrogen in the top soil layer is negative!");
   }
}

void SOIL::calculateNitrogenLossByVolatilization(UTILS utils)
{
   double nitrogenLossByVolatilization = 0.0;

   nitrogenLossByVolatilization = 0.0 * nitrogenGrossMineralization;

   nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -= nitrogenLossByVolatilization;
   nitrogenVolatilization += nitrogenLossByVolatilization;

   if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + tolerance < 0.0)
   {
      nitrogenContent_soilMineralPoolPerSoilLayer.at(0) = 0.0;
      utils.handleError("Soil mineral nitrogen in the top soil layer is negative!");
   }
}
