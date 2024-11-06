#include "interaction.h"

INTERACTION::INTERACTION() {};
INTERACTION::~INTERACTION() {};

void INTERACTION::calculateLightAttenuationAndAvailabilityForPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community, double fullSunLight)
{
   /// 1. calculate height of largest plant in the community to discretisize the aboveground space vertically into height layers
   calculateNumberOfHeightLayersFromLargestPlant(utils, community);

   /// 2. calculate leaf area index of all plants in the community across height layers
   /// in a cumulative way from top to bottom
   calculateCumulativeLeafAreaIndexAcrossHeightLayers(utils, community, parameter);

   /// 3. calculate how much cumulative leaf area index is overtopping and shading a plant cohort
   /// 4. and calculation of available sunlight reaching this plant cohort
   /// based on Beer-Lambert law of light transmission and extinction
   calculateLightAvailabilityForPlants(utils, community, parameter, fullSunLight);

   // TODO: maybe modify calculateCrowdingConditions();
}

void INTERACTION::calculateNumberOfHeightLayersFromLargestPlant(UTILS utils, COMMUNITY &community)
{
   /// maximumHeightOfAllPlants is initialized (with 0) in every day step in initAndResetProcessVariables()
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      if (community.allPlants[cohortindex]->height > community.maximumHeightOfAllPlants)
      {
         community.maximumHeightOfAllPlants = community.allPlants[cohortindex]->height;
      }
   }
   /// predefined maximum height layer = 300 (see module_init/constants.h)
   if (community.maximumHeightOfAllPlants > maximumHeightLayer)
   {
      utils.handleError("Maximum plant height is exceeding the constant parameter maximumHeightLayer. The parameter should be set up in module_init/constants.h!");
   }
   else
   {
      /// reduce for-loops by calculating maximum height layer reached by currently largest plant in community
      maximumHeightLayerIndexReachedByPlants = (int)std::ceil(community.maximumHeightOfAllPlants / heightLayerWidth);
   }
}

void INTERACTION::calculateCumulativeLeafAreaIndexAcrossHeightLayers(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{
   /// go through all living plants in the community and add their leaf area to the respective height layers
   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int plantAmount = community.allPlants.at(cohortindex)->amount;
      int pft = community.allPlants.at(cohortindex)->pft;
      double plantHeight = community.allPlants.at(cohortindex)->height;
      double plantArea = community.allPlants.at(cohortindex)->coveredArea;
      double plantLeafAreaIndex = community.allPlants.at(cohortindex)->lai;
      double plantLightExtinctionCoefficient = parameter.lightExtinctionCoefficients.at(pft);
      double leafAreaOfPlantCohort = plantAmount * plantArea * plantLeafAreaIndex;

      if (plantAmount > 0)
      {
         /// search for height layer up to which the plant cohort is reaching to
         /// Note: floor is used because first height layer 0-1 cm has index 0
         int topHeightLayerIndexOfPlant = (int)std::floor((plantHeight / heightLayerWidth));

         /// calculate weights for the plants' contribution to height layers based on plant height
         calculateWeightsOfPlantHeightContributionToHeightLayers(topHeightLayerIndexOfPlant, plantHeight);

         /// add plant cohort's leaf area to height layers according to the respective weights
         addPlantLeafAreaToHeightLayers(topHeightLayerIndexOfPlant, leafAreaOfPlantCohort, plantLightExtinctionCoefficient);
      }
   }

   /// accumulate the leaf area index across height layers from top to bottom
   accumulateLeafAreaFromTopToBottomHeightLayers(maximumHeightLayerIndexReachedByPlants);
}

void INTERACTION::calculateWeightsOfPlantHeightContributionToHeightLayers(int topHeightLayerIndexOfPlant, double plantHeight)
{
   double plantPartInHeightLayer;
   weightsForPlantContributionToHeightLayer.clear();

   for (int layerindex = 0; layerindex < topHeightLayerIndexOfPlant; layerindex++)
   {
      /// plant parts fully cover the respective height layer
      plantPartInHeightLayer = heightLayerWidth / plantHeight;
      weightsForPlantContributionToHeightLayer.push_back(plantPartInHeightLayer);
   }

   /// plant parts at top layer (topHeightLayerIndexOfPlant) may not fully cover the entire height layer
   /// downward correction is required
   plantPartInHeightLayer = ((plantHeight / heightLayerWidth) - std::floor(plantHeight / heightLayerWidth));
   weightsForPlantContributionToHeightLayer.push_back((plantPartInHeightLayer / plantHeight));
}

void INTERACTION::addPlantLeafAreaToHeightLayers(int topHeightLayerIndexOfPlant, double leafAreaOfPlantCohort, double plantLightExtinctionCoefficient)
{
   for (int layerindex = 0; layerindex <= topHeightLayerIndexOfPlant; layerindex++)
   {
      LAI.at(layerindex) += (leafAreaOfPlantCohort * weightsForPlantContributionToHeightLayer.at(layerindex));
      LAIwithLightExtinction.at(layerindex) += (leafAreaOfPlantCohort * plantLightExtinctionCoefficient * weightsForPlantContributionToHeightLayer.at(layerindex));
   }
}

void INTERACTION::accumulateLeafAreaFromTopToBottomHeightLayers(int maximumHeightLayerReachedByPlants)
{
   for (int layerindex = maximumHeightLayerReachedByPlants - 1; layerindex >= 0; layerindex--)
   {
      LAI.at(layerindex) += LAI.at(layerindex + 1);
      LAIwithLightExtinction.at(layerindex) += LAIwithLightExtinction.at(layerindex + 1);
   }
}

void INTERACTION::calculateLightAvailabilityForPlants(UTILS utils, COMMUNITY &community, PARAMETER parameter, double fullSunLight)
{
   int lowestOvertoppingHeightLayerIndexOfPlant;

   for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
   {
      int plantAmount = community.allPlants.at(cohortindex)->amount;
      int pft = community.allPlants.at(cohortindex)->pft;
      double plantHeight = community.allPlants.at(cohortindex)->height;

      if (plantAmount > 0)
      {
         lowestOvertoppingHeightLayerIndexOfPlant = (int)std::ceil(plantHeight / heightLayerWidth);

         getOvertoppingCumulativeLeafAreaIndexOfPlant(community, cohortindex, lowestOvertoppingHeightLayerIndexOfPlant);
         calculateAvailableLightReachingAPlant(parameter, community, cohortindex, fullSunLight);
         calculateShadingIndicatorOfPlantForOutput(utils, parameter, community, cohortindex, fullSunLight);
      }
   }

   // TODO: not knowing if we really need this for grasslands
   // fractionOfLightAtFloor = getRadiationAtLowerBoundaryOfHeightLayer(LAIwithLightExtinction.at(0), 1);
}

void INTERACTION::getOvertoppingCumulativeLeafAreaIndexOfPlant(COMMUNITY &community, int cohortindex, int layerindex)
{
   community.allPlants.at(cohortindex)->cumulativeOvertoppingCommunityLAI = LAIwithLightExtinction.at(layerindex);
   //  / Light_Comp_Factor -> rename to plantInteractionWeightingFactors?
}

void INTERACTION::calculateAvailableLightReachingAPlant(PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight)
{
   double shadingCommunityLeafAreaIndex = community.allPlants.at(cohortindex)->cumulativeOvertoppingCommunityLAI;
   community.allPlants.at(cohortindex)->availableRadiation = getRadiationByLightExtinctionLaw(shadingCommunityLeafAreaIndex, fullSunLight);
}

void INTERACTION::calculateShadingIndicatorOfPlantForOutput(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double fullSunLight)
{
   double sunLightReachingPlant = community.allPlants.at(cohortindex)->availableRadiation;
   if (fullSunLight > 0)
   {
      community.allPlants.at(cohortindex)->shadingIndicator = sunLightReachingPlant / fullSunLight;
   }
   else
   {
      community.allPlants.at(cohortindex)->shadingIndicator = -1.0;
      utils.handleWarning("There is no sunlight today. The shading factor is set to default value of -1.");
   }
}

double INTERACTION::getRadiationByLightExtinctionLaw(double cumulativeLAIAboveAndAtHeightLayer, double fullSunLight)
{
   /// the resulting radiationByExtinction is the remaining sun light that reaches the lower boundary of a height layer
   /// whose cumulativeLAI (i.e. LAIwithLightExtinction) equals the leaf area at this layer and of all layers above
   double radiationByExtinction = exp(-cumulativeLAIAboveAndAtHeightLayer) * fullSunLight;
   return radiationByExtinction;
}

void INTERACTION::getEnvironmentalConditionsOfDay(WEATHER weather, SOIL soil, MANAGEMENT management, int day)
{
   fullSunLight = weather.photosyntheticPhotonFluxDensity.at(day - 1); // parameter.day starts at 1, but vectors start with index 0
   dayLength = weather.dayLength.at(day - 1);
   dayTimeAirTemperature = weather.dayTimeAirTemperature.at(day - 1);
   fullDayAirTemperature = weather.fullDayAirTemperature.at(day - 1);
}