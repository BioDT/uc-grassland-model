#include "management.h"

MANAGEMENT::MANAGEMENT() {};
MANAGEMENT::~MANAGEMENT() {};

void MANAGEMENT::applyManagementRegime(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL &soil)
{
    /* mowing events */
    initializeYieldVariables(community, parameter);
    checkIfTodayAndDoMowing(utils, community, allometry, parameter);

    /* fertilization events */
    checkIfTodayAndDoFertilization(utils, parameter, soil);

    /* irrigation events */
    checkIfTodayAndDoIrrigation(utils, parameter, soil);

    /* seed (re-)sowing */
    // is captured within recruitment.cpp, see getIncomingSeedsBySowing()
}

void MANAGEMENT::initializeYieldVariables(COMMUNITY &community, PARAMETER parameter)
{
    // initialize variables to track yield
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        community.greenBiomassYieldPerPFT[pft] = 0.0;
        community.brownBiomassYieldPerPFT[pft] = 0.0;
        community.biomassYieldPerPFT[pft] = 0.0;

        community.greenBiomassYield = 0.0;
        community.brownBiomassYield = 0.0;
        community.biomassYield = 0.0;
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
            // update community variables in case of cutting
            community.greenleafAreaIndexOfPlantsInCommunity = 0;
            community.totalLeafAreaIndexOfPlantsInCommunity = 0;
            for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
            {
                // state variable updates for soil evaporation
                community.greenleafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->amount * community.allPlants[cohortindex]->laiGreen * community.allPlants[cohortindex]->coveredArea/SIMULATION_AREA;
                community.totalLeafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->lai * community.allPlants[cohortindex]->coveredArea * community.allPlants[cohortindex]->amount/SIMULATION_AREA;
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
            community.greenBiomassYieldPerPFT[pft] += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
            community.brownBiomassYieldPerPFT[pft] += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
            community.biomassYieldPerPFT[pft] += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

            community.greenBiomassYield += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
            community.brownBiomassYield += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
            community.biomassYield += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

            // update attributes of plants
            community.allPlants[cohortIndex]->shootBiomass -= (cutGreenLeaves + cutBrownLeaves);
            community.allPlants[cohortIndex]->shootBiomassGreenLeaves -= cutGreenLeaves;
            community.allPlants[cohortIndex]->shootBiomassBrownLeaves -= cutBrownLeaves;

            community.allPlants[cohortIndex]->shootCarbonGreenLeaves = community.allPlants[cohortIndex]->shootBiomassGreenLeaves * CARBON_CONTENT_ODM;
            community.allPlants[cohortIndex]->shootCarbonBrownLeaves = community.allPlants[cohortIndex]->shootBiomassBrownLeaves * CARBON_CONTENT_ODM;
            community.allPlants[cohortIndex]->shootCarbon = community.allPlants[cohortIndex]->shootCarbonGreenLeaves + community.allPlants[cohortIndex]->shootCarbonBrownLeaves;

            community.allPlants[cohortIndex]->shootNitrogenGreenLeaves = community.allPlants[cohortIndex]->shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
            community.allPlants[cohortIndex]->shootNitrogenBrownLeaves = community.allPlants[cohortIndex]->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
            community.allPlants[cohortIndex]->shootNitrogen = community.allPlants[cohortIndex]->shootNitrogenGreenLeaves + community.allPlants[cohortIndex]->shootNitrogenBrownLeaves;

            community.allPlants[cohortIndex]->height = heightToCutPlantsDownTo;
            community.allPlants[cohortIndex]->laiGreen = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassGreenLeaves,
                                                                                              community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
            community.allPlants[cohortIndex]->laiBrown = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassBrownLeaves,
                                                                                              community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
            community.allPlants[cohortIndex]->lai = community.allPlants[cohortIndex]->laiGreen + community.allPlants[cohortIndex]->laiBrown;
        }
    }
}

void MANAGEMENT::checkIfTodayAndDoFertilization(UTILS utils, PARAMETER parameter, SOIL &soil)
{
    soil.addedMineralNitrogenToSoilByFertilization = 0.0;

    // scan through all fertilization dates from the management file to check if today is an event
    int index = 0;
    for (auto day : fertilizationDate)
    {
        if (parameter.day == day)
        {
            double amountOfMineralFertilizer = fertilizerAmount.at(index);
            soil.nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += amountOfMineralFertilizer;
            soil.addedMineralNitrogenToSoilByFertilization += amountOfMineralFertilizer;

            double amountOfOrganicFertilizer = 0.0; // TODO: add organic fertilizer option to management file
            soil.nitrogenContent_surfaceMetabolicLitterPool += amountOfOrganicFertilizer;
        }
        index++;
    }
}

void MANAGEMENT::checkIfTodayAndDoIrrigation(UTILS utils, PARAMETER parameter, SOIL &soil)
{
    soil.addedWaterToSoilByIrrigation = 0.0;

    // scan through all irrigation dates from the management file to check if today is an event
    int index = 0;
    for (auto day : irrigationDate)
    {
        if (parameter.day == day)
        {
            double amountOfWater = irrigationAmount.at(index);
            soil.waterContent_soilWaterPoolPerSoilLayer.at(0) += amountOfWater;
            soil.addedWaterToSoilByIrrigation += amountOfWater;
        }
        index++;
    }
}
