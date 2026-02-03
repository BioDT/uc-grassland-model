#pragma once
#include "plant.h"
#include "allometry.h"
#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <algorithm>

/**
 * @brief Represents a community of plants in a grassland ecosystem.
 *
 * The `COMMUNITY` class maintains a collection of plant cohorts, tracks their state,
 * and provides methods for managing and updating the community dynamics. It includes
 * functionalities to check for plant viability and calculate aggregated state variables
 * based on the current plant composition.
 */
class COMMUNITY
{
public:
    COMMUNITY();
    ~COMMUNITY();

    int randomNumberIndex;

    std::vector<std::shared_ptr<PLANT>> allPlants;

    //*********************************************************/
    // community / ecosystem specific state variables calculated in community.cpp for process calculations (and partly output)
    int totalNumberOfCohortsInCommunity; // required for loops through community vector, updated in mortality.cpp and recruitment.cpp
    int totalNumberOfPlantsInCommunity;  // required for output (calculated in output.cpp)

    // required for light attenuation and shading calculations, and for output
    double maximumHeightOfAllPlants; // cm (calculated in interaction.cpp)

    // required for soil evaporation, calculated in soil.cpp
    double greenleafAreaIndexOfPlantsInCommunity;
    double totalLeafAreaIndexOfPlantsInCommunity; // cm per cm (calculated in output.cpp) --> CHANGE

    // required for crowding, calculated in
    double coveredAreaOfAllPlants;

    // required for carbon balance, calculated in
    double carbonRespirationOfAllPlants;
    double carbonNPPOfAllPlants;
    double carbonSeedlingIngrowthOfAllPlants;

    double ecosystemNitrogenBalance;
    double ecosystemCarbonBalance;
    double ecosystemCarbonRespiration;

    // required for soil water competition
    double totalSoilWaterDemand;
    std::vector<double> totalSoilWaterDemandPerSoilLayer;
    double totalSoilWaterUptake;
    std::vector<double> totalSoilWaterUptakePerSoilLayer;

    // required for soil nitrogen competition
    double totalSoilNitrogenDemand;
    std::vector<double> totalSoilNitrogenDemandPerSoilLayer;
    std::vector<int> numberOfPlantsCompetingForSoilNitrogenPerSoilLayer;
    double totalSoilNitrogenUptake;
    std::vector<double> totalSoilNitrogenUptakePerSoilLayer;

    // state variables calculated in management.cpp to track yield after mowing
    double greenBiomassYield;
    double brownBiomassYield;
    double biomassYield;

    //*********************************************************/
    // PFT specific state variables calculated in community.cpp for output
    std::vector<double> pftComposition;                    //*
    std::vector<double> numberOfPlantsPerPFT;              //*
    std::vector<double> coveredAreaOfPlantsPerPFT;         //*
    std::vector<double> shootBiomassOfPlantsPerPFT;        //*
    std::vector<double> greenShootBiomassOfPlantsPerPFT;   //*
    std::vector<double> brownShootBiomassOfPlantsPerPFT;   //*
    std::vector<double> clippedShootBiomassOfPlantsPerPFT; //*
    std::vector<double> rootBiomassOfPlantsPerPFT;         //*
    std::vector<double> recruitmentBiomassOfPlantsPerPFT;  //*
    std::vector<double> exudationBiomassOfPlantsPerPFT;    //*

    std::vector<double> gppOfPlantsPerPFT;
    std::vector<double> nppOfPlantsPerPFT;
    std::vector<double> carbonRespirationOfPlantsPerPFT;

    // state variables calculated in management.cpp for output
    std::vector<double> greenBiomassYieldPerPFT;
    std::vector<double> brownBiomassYieldPerPFT;
    std::vector<double> biomassYieldPerPFT;

    void checkPlantsAreAliveInCommunity(UTILS utils);
};
