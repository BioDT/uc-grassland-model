#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"
#include "../module_soil/soil.h"
#include "../module_plant/allometry.h"
#include "../module_init/constants.h"
#include "../utils/utils.h"

class GROWTH
{
public:
    GROWTH();
    ~GROWTH();

    void doPlantGrowth(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community, INTERACTION interaction, ALLOMETRY allometry, SOIL &soil);

    void doPlantPhotosynthesis(UTILS utils, PARAMETER parameter, COMMUNITY &community, INTERACTION interaction);
    double calculateCO2UptakePerSecondAndSquareMeter(PARAMETER parameter, int pft, double plantRadiation, double plantLAI);
    double calculateGPPOfPlantWithCommunityShading(UTILS utils, INTERACTION interaction, PARAMETER parameter, int pft, double plantHeight, double plantLAI);
    double calculateCO2UptakePerSecondAndSquareMeterWithCommunityShading(PARAMETER parameter, int pft, double lightExtinction, double photoactiveLai, double plantRadiation);

    void doPlantRespiration(COMMUNITY &community, PARAMETER parameter, INTERACTION interaction);
    void calculatePlantNPPFromGPPAndRespiration(COMMUNITY &community, PARAMETER parameter);
    double calculateEffectOfAirTemperatureOnGPP(double dayTimeAirTemperature);
    double calculateEffectOfAirTemperatureOnRespiration(PARAMETER parameter, double airTemperature);
    void adjustAllocationRates(UTILS utils, PARAMETER parameter, COMMUNITY &community);
    void doPlantNPPAllocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, SOIL &soil);
    double calculateProportionalityFactorForAllocationDistributionToPlantParts(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft);
    void adjustAllocationRatesForMaturePlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);
    void adjustAllocationRatesForRegrowingPlants(PARAMETER parameter, COMMUNITY &community, int cohortindex, int pft, double proportionOfNppAllocationToPlantGrowthToShoot);
    void doPlantGrowthInSizeAndAging(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry);
};