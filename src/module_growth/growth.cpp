#include "growth.h"

GROWTH::GROWTH() {};
GROWTH::~GROWTH() {};

/* main function of plant growth */
void GROWTH::doPlantGrowth(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction)
{
   /* Plant GPP (gross primary productivity) */
   doPlantPhotosynthesis(parameter, community, interaction);

   /* Limitation of plant GPP by unfavorable soil water conditions */
   // soil.doPlantGPPLimitationBySoilWaterConditions();

   /* Plant respiration */
   doPlantRespiration(community, parameter);

   /* Plant NPP (net primary productivity) */
   calculatePlantNPPFromGPPAndRespiration(community, parameter);

   /* Limitation of plant NPP by unfavorable soil nitrogen conditions */
   // soil.doPlantNPPLimitationBySoilNitrogenConditions();

   /* Plant allocation of NPP and according C and N parts */
   // doPlantNPPAllocation(parameter, community);

   /* Plant growth in size based on NPP allocation */
   // doPlantSizeGrowth(parameter, community);
}

void GROWTH::doPlantPhotosynthesis(PARAMETER parameter, COMMUNITY &community, INTERACTION interaction)
{
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int pft = community.allPlants.at(cohortindex)->pft;
      double plantLAI = community.allPlants.at(cohortindex)->laiGreen;
      double plantCoveredArea = community.allPlants.at(cohortindex)->coveredArea;
      double plantRadiation = (24.0 / interaction.dayLength) * community.allPlants.at(cohortindex)->availableRadiation; // correct mean daily radiation by daylength hours for photosynthesis
      community.allPlants.at(cohortindex)->gpp = calculateGPPOfPlant(parameter, pft, plantLAI, plantCoveredArea, plantRadiation, interaction.dayLength);
   }
}

double GROWTH::calculateGPPOfPlant(PARAMETER parameter, int pft, double plantLAI, double plantCoveredArea, double plantRadiation, double dayLength)
{
   if (plantRadiation == 0)
   {
      return 0;
   }
   else
   {
      double CO2UptakePerSecondAndSquareMeter = calculateCO2UptakePerSecondAndSquareMeter(parameter, pft, plantRadiation, plantLAI);
      double OdmUptakePerSecondAndSquareMeter = CO2UptakePerSecondAndSquareMeter * CO2ConversionToOdm * molarMassOfCO2; // conversion from CO2 to Odm

      double secondsPerDay = dayLength * 60 * 60;                                                             // scaling from seconds to day
      double plantPhotosynthesisPerDay = OdmUptakePerSecondAndSquareMeter * secondsPerDay * plantCoveredArea; // scaling to plant
                                                                                                              // TODO: statt odm use of carbon?
      return plantPhotosynthesisPerDay;                                                                       // g ODM per day and plant
   }
}

double GROWTH::calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI)
{
   const double alpha = parameter.initialSlopeOfLightResponseCurve.at(pft);
   const double k = parameter.lightExtinctionCoefficients.at(pft);
   const double pmax = parameter.maximumGrossLeafPhotosynthesisRate.at(pft);

   const double calcPart1 = alpha * k * plantRadiation;
   const double calcPart2 = pmax * (1 - lightTransmissionCoefficient);

   double CO2UptakePerSecondsAndSquareMeter = ((pmax / k) * log((calcPart1 + calcPart2) / (calcPart1 * exp(-k * plantLAI) + calcPart2)));
   return (CO2UptakePerSecondsAndSquareMeter);
}

double GROWTH::calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature)
{
   double reductionFactor = 0;
   int day;

   if (dayTimeAirTemperature <= -5)
   {
      reductionFactor = 0;
   }

   if (dayTimeAirTemperature > -5 && dayTimeAirTemperature <= 2)
   {
      reductionFactor = (0.02857 * dayTimeAirTemperature + 0.142);
   }

   if (dayTimeAirTemperature > 2 && dayTimeAirTemperature <= 10)
   {
      reductionFactor = (0.1 * dayTimeAirTemperature);
   }

   if (dayTimeAirTemperature > 10)
   {
      reductionFactor = 1.0;
   }

   return reductionFactor;
}

void GROWTH::doPlantRespiration(COMMUNITY &community, PARAMETER parameter)
{
   // TODO: add here temperature effect
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int pft = community.allPlants.at(cohortindex)->pft;
      double greenShootBiomass = community.allPlants.at(cohortindex)->shootBiomassGreenLeaves;
      double rootBiomass = community.allPlants.at(cohortindex)->rootBiomass;
      community.allPlants.at(cohortindex)->maintenanceRespiration = parameter.maintenanceRespirationRate * (greenShootBiomass + rootBiomass);
   }
}

double GROWTH::calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature)
{
   double reductionFactor = 0;

   if (airTemperature > 15)
   {
      double tExponent = (airTemperature - parameter.plantResponseToTemperatureQ10Reference) / 10.0;
      reductionFactor = std::pow(parameter.plantResponseToTemperatureQ10Base, tExponent);
   }
   else if (airTemperature <= 0)
      reductionFactor = 0;
   else
      reductionFactor = 0.03333 * airTemperature;

   return reductionFactor;
}

void GROWTH::calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter)
{
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int pft = community.allPlants.at(cohortindex)->pft;
      double plantGPP = community.allPlants.at(cohortindex)->gpp;
      double maintenanceRespiration = community.allPlants.at(cohortindex)->maintenanceRespiration;
      double growthRespiration = 0;

      if (plantGPP > maintenanceRespiration)
      {
         growthRespiration = parameter.growthRespirationFraction * (plantGPP - maintenanceRespiration);
         community.allPlants.at(cohortindex)->growthRespiration = growthRespiration;
      }
      else
      {
         growthRespiration = 0;
         community.allPlants.at(cohortindex)->growthRespiration = 0;
      }

      community.allPlants.at(cohortindex)->totalRespiration = maintenanceRespiration + growthRespiration;

      // calculate biomass increment binc = (1-ra)*(plant->gpp - plant->maintenanceRespiration);
      double biomassIncrement = plantGPP - community.allPlants.at(cohortindex)->totalRespiration; //+ community.allPlants.at(cohortindex)->Buffer;

      // ==== reset buffer for next year ====
      // plant->photoproductionBuffer is negative if last year's respiration exceeded photosynthesis
      // community.allPlants.at(cohortindex)->Buffer = 0.0; // reset for current year
      if (biomassIncrement < 0)
      {
         // community.allPlants.at(cohortindex)->Buffer = biomassIncrement;         // buffer  of photosynthesis
         community.allPlants.at(cohortindex)->maintenanceRespiration = plantGPP; // all photosynthesis is used for respiration
         community.allPlants.at(cohortindex)->growthRespiration = 0;
         community.allPlants.at(cohortindex)->totalRespiration = maintenanceRespiration;
         biomassIncrement = 0;
      }
   }
}

/*
void GROWTH::doPlantNPPAllocation(PARAMETER parameter, COMMUNITY &community, WEATHER weather)
{

   /// Plant maintenance respiration
   double biomassIncrement;
   double maxHeightFromWidth;

   double nitrogenBalance;
   double multiplier = 1; // 000;

   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int pft = community.allPlants.at(cohortindex)->pft;
      double biomassIncrementForAllocation = community.allPlants.at(cohortindex)->npp;
      // adjustAllocationRates(&plant);

      // *******Do this already in N limitation function!!!
      // biomassIncrement = community.allPlants.at(cohortindex)->potentialBiomassIncrementForNitrogenDemands;
      // biomassIncrement *= plant->limitingFactorGppNitrogen; // reduction by nitrogen limitation
      // store total biomass increment: to be used for shoot, root, reproduction, exudation
      // plant->biomassIncrementForAllocation = biomassIncrement;

      // error check
      /*nitrogenBalance = plant->nitrogenContentShoot -
                        plant->biomassGreen * ODM_TO_C / par.cnRatioGreenLeaves[pft] -
                        plant->biomassBrown * ODM_TO_C / par.cnRatioBrownLeaves[pft];
      if (abs(nitrogenBalance) > multiplier * EPSILON)
      {
         std::cout << "grass_grow.cpp: Wrong nitrogen amount in plant shoot during uptake!" << std::endl;
         std::cout << "N difference 'shoot' - 'biomass': " << nitrogenBalance << std::endl;
         std::cout << "N uptake to 'shoot': " << plant->nitrogenUptakeShoot << std::endl;
         std::cout << "pft: " << pft << std::endl;
         std::cout << "timestep: " << parameter.day << std::endl;
      }*/
// *******Do this above already in N limitation function!!!
/*
      if (biomassIncrementForAllocation > 0)
      {
         /// aboveground allocation and growth
         plant->biomassGreen += biomassIncrement * plant->nppAllocationToShoot;
         plant->biomassShoot = plant->biomassGreen + plant->biomassBrown;
         plant->biomassFractionGreen = plant->biomassGreen / plant->biomassShoot;
         plant->biomassFractionBrown = plant->biomassBrown / plant->biomassShoot;

         if (abs(plant->biomassFractionBrown + plant->biomassFractionGreen - 1.0) > tolerance)
         {
            utils.handleError("Green and brown biomass fractions do not sum to 1!");
         }

         /// belowground allocation and growth
         plant->rootBiomass += biomassIncrement * plant->nppAllocationToRoot;
         plant->biomassReproduction += biomassIncrement * plant->nppAllocationToReproduction;
         plant->biomassExudation = biomassIncrement * plant->nppAllocationToExudation;

         patch->CPool_Soil_active +=
             plant->biomassExudation; // QQM: times plant->N ??? -> then also replace 1 in function by plant->N
         plant->biomassExudation = 0;
      }

      /// modularisieren
      if (plant->nitrogenUptakeShoot >= 0.0)
      {
         plant->nitrogenContentShoot += plant->nitrogenUptakeShoot;
      }
      else
      {
         forError("grass_grow.cpp: plant->nitrogenUptakeShoot is negative!");
      }
      // error check
      nitrogenBalance = plant->nitrogenContentShoot -
                        plant->biomassGreen * ODM_TO_C / par.cnRatioGreenLeaves[pft] -
                        plant->biomassBrown * ODM_TO_C / par.cnRatioBrownLeaves[pft];
      if (abs(nitrogenBalance) > multiplier * EPSILON)
      {
         std::cout << "grass_grow.cpp: Wrong nitrogen amount in plant shoot after uptake!" << std::endl;
         std::cout << "N difference 'shoot' - 'biomass': " << nitrogenBalance << std::endl;
         std::cout << "N uptake to 'shoot': " << plant->nitrogenUptakeShoot << std::endl;
         std::cout << "pft: " << pft << std::endl;
         std::cout << "timestep: " << timer.getCurrentTimeStep() << std::endl;
      }

      if (plant->nitrogenUptakeRoot >= 0.0)
      {
         plant->nitrogenContentRoot += plant->nitrogenUptakeRoot;
      }
      else
      {
         forError("grass_grow.cpp: plant->nitrogenUptakeRoot is negative!");
      }

      if (plant->nitrogenUptakeReproduction >= 0.0)
      {
         plant->nitrogenContentReproduction += plant->nitrogenUptakeReproduction;
      }
      else
      {
         forError("grass_grow.cpp: plant->nitrogenUptakeReproduction is negative!");
      }

      if (plant->nitrogenUptakeExudation >= 0.0)
      {
         plant->nitrogenContentExudation += plant->nitrogenUptakeExudation;
      }
      else
      {
         forError("grass_grow.cpp: plant->nitrogenUptakeExudation is negative!");
      }

      /// Update plant geometry according to allocated biomass increment
      if (plant->isRegrowing)
      {
         // Concept: when regrowing (after mowing) all biomass increment is put into height growth
         //          not in width or roots, until height-width-ratio is reached again
         // TODO: discuss if we should also change allocation rate?
         // TODO: or even in future change parameter height-width-ratio
         plant->height =
             heightFromBiomassWidthForm(plant->biomassShoot, plant->dbh, par.formFactorFromDbhParams[0][pft]);
         maxHeightFromWidth = heightFromWidthHwr(plant->dbh, par.heightFromDbhParams[0][pft]);

         if (plant->height > maxHeightFromWidth)
         {
            plant->isRegrowing = false;
            // new height is above height-width-ratio, normal geometry calculation can be used again
            // comes in '!isRegrowing' case below
         }
      }

      if (!plant->isRegrowing)
      {
         plant->dbh = widthFromBiomassHwrForm(plant->biomassShoot, par.heightFromDbhParams[0][pft],
                                              par.formFactorFromDbhParams[0][pft]);
         plant->height = heightFromWidthHwr(plant->dbh, par.heightFromDbhParams[0][pft]);
         plant->crownArea = areaFromWidth(plant->dbh);

         // cut height if above maximum
         if (plant->height > maxTreeHeight)
         {
            plant->height = maxTreeHeight;
            plant->dbh = widthFromHeightHwr(plant->height, par.heightFromDbhParams[0][pft]);
            plant->crownArea = areaFromWidth(plant->dbh);

            // QQT:  what to do with the surplus in carbon and nitrogen?
            // TODO: check if this case can happen at all or there is a limit
            //       by saturation functions --> only develop further if needed
            plant->biomassShoot =
                biomassFromHeightWidthForm(plant->height, plant->dbh, par.formFactorFromDbhParams[0][pft]);

            // QQT: not sure what this does and why it is in the > maxTreeHeight case?
            plant->totalRespiration = plant->gpp;
            plant->growthRespiration = 0.0;
            plant->maintenanceRespiration = plant->gpp;
         }
      }

      patch->maxTreeHeight = std::max(plant->height, patch->maxTreeHeight);

      plant->rootHeight = par.rootDepthParams[0][pft] *
                          std::pow((par.shootRootRatio[0][pft] / par.formFactorFromDbhParams[0][pft]),
                                   par.rootDepthParams[1][pft]) *
                          std::pow((double)plant->rootBiomass, par.rootDepthParams[1][pft]);
      plant->totalBranchLengthOfRoot = plant->rootBiomass * par.specificRootLength[pft];
      plant->laiGreen =
          laiFromBiomassAreaSla(plant->biomassGreen, plant->crownArea, par.laiFromDbhParams[0][pft]);
      plant->laiBrown =
          laiFromBiomassAreaSla(plant->biomassBrown, plant->crownArea, par.laiFromDbhParams[0][pft]);
      plant->lai = plant->laiBrown + plant->laiGreen;

      // Remove? Not needed in GRASSMIND
      // plant->stemVolume = plant->height * plant->crownArea;
      // plant->stemVolume_logging = plant->height * plant->crownArea;
      if (plant->height > par.calibBiomassClipHeight)
      {
         if (par.crownDensityCurveType == 1)
         {
            plant->biomassAboveClipHeight =
                ((plant->height - par.calibBiomassClipHeight) / plant->height) * plant->biomassShoot;
         }
         else
         {
            plant->biomassAboveClipHeight = (1 - plant->biomassAboveClipHeight_weighted) * plant->biomassShoot;
         }
      }

      plant->dbhIncrement = plant->dbh - previousDiameter;
      plant->heightIncrement = plant->height - previousHeight;
      plant->biomassIncrementShoot = plant->biomassShoot - previousShootBiomass;
      plant->biomassIncrementRoot = plant->rootBiomass - previousRootBiomass;
   }
}
*/