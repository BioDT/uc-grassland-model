#include "management.h"

MANAGEMENT::MANAGEMENT() {};
MANAGEMENT::~MANAGEMENT() {};

void MANAGEMENT::applyManagementRegime(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter)
{
   /* mowing events */
   initializeYieldVariables(community, parameter);
   checkIfTodayAndDoMowing(utils, community, allometry, parameter);
}

void MANAGEMENT::initializeYieldVariables(COMMUNITY &community, PARAMETER parameter)
{
   // initialize variables to track yield
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      community.greenBiomassYield[pft] = 0.0;
      community.brownBiomassYield[pft] = 0.0;
      community.biomassYield[pft] = 0.0;

      community.greenYield = 0.0;
      community.brownYield = 0.0;
      community.yield = 0.0;
   }
}

void MANAGEMENT::checkIfTodayAndDoMowing(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter)
{
   // scan through all mowing dates from the management file to check if today is a mowing event
   int index = 0;
   for (auto day : mowingDate)
   {
      if (parameter.day == day)
      {
         double heightToCutPlantsDownTo = 100.0 * mowingHeight.at(index); // convert m in cm
         for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
         {
            int pft = community.allPlants[cohortIndex]->pft;
            cutPlantsAndTrackYieldAndUpdatePlantAttributes(utils, community, allometry, parameter, cohortIndex, pft, heightToCutPlantsDownTo);
         }
      }
      index++;
   }
}

void MANAGEMENT::cutPlantsAndTrackYieldAndUpdatePlantAttributes(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int cohortIndex, int pft, double heightToCutPlantsDownTo)
{
   if (community.allPlants[cohortIndex]->amount > 0)
   {
      if (community.allPlants[cohortIndex]->height > heightToCutPlantsDownTo)
      {
         // cut the plants
         double heightProportionalityFactor = (community.allPlants[cohortIndex]->height - heightToCutPlantsDownTo) / community.allPlants[cohortIndex]->height;
         double cutGreenLeaves = heightProportionalityFactor * community.allPlants[cohortIndex]->shootBiomassGreenLeaves;
         double cutBrownLeaves = heightProportionalityFactor * community.allPlants[cohortIndex]->shootBiomassBrownLeaves;

         // track the yield
         community.greenBiomassYield[pft] += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
         community.brownBiomassYield[pft] += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
         community.biomassYield[pft] += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

         community.greenYield += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
         community.brownYield += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
         community.yield += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

         // update attributes of plants
         community.allPlants[cohortIndex]->shootBiomass -= (cutGreenLeaves + cutBrownLeaves);
         community.allPlants[cohortIndex]->shootBiomassGreenLeaves -= cutGreenLeaves;
         community.allPlants[cohortIndex]->shootBiomassBrownLeaves -= cutBrownLeaves;

         // community.allPlants[cohortIndex]->shootNitrogen = (community.allPlants[cohortIndex]->shootBiomassGreenLeaves * carbonContentOdm / parameter.plantCNRatioGreenLeaves[pft]) +
         //                                                   (community.allPlants[cohortIndex]->shootBiomassBrownLeaves * carbonContentOdm / parameter.plantCNRatioBrownLeaves[pft]);

         // Check if nitrogen pool of plant is consistent to CN ratios and biomass
         /*double nContentCalculatedFromBiomass =
             ((community.allPlants[cohortIndex]->shootBiomassBrownLeaves * carbonContentOdm) / parameter.plantCNRatioBrownLeaves[pft]) +
             ((community.allPlants[cohortIndex]->shootBiomassGreenLeaves * carbonContentOdm) / parameter.plantCNRatioGreenLeaves[pft]);
         if (nContentCalculatedFromBiomass > community.allPlants[cohortIndex]->shootNitrogen)
         {
            utils.handleError("Nitrogen content in plants is too low. There might be an code error!");
         }
         else if (nContentCalculatedFromBiomass < community.allPlants[cohortIndex]->shootNitrogen)
         {
            utils.handleError("Nitrogen content in plants is too high. There might be an code error!");
         }*/

         community.allPlants[cohortIndex]->height = heightToCutPlantsDownTo;
         community.allPlants[cohortIndex]->laiGreen = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassGreenLeaves,
                                                                                           community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
         community.allPlants[cohortIndex]->laiBrown = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassBrownLeaves,
                                                                                           community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
         community.allPlants[cohortIndex]->lai = community.allPlants[cohortIndex]->laiGreen + community.allPlants[cohortIndex]->laiBrown;
      }
   }
}
