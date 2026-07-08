#include "pythoninterface.h"

PYTHONINTERFACE::PYTHONINTERFACE()
{
}

void PYTHONINTERFACE::initializeSimulation(std::string path)
{

    /**
     * @brief Starts tracking the computational runtime.
     */
    std::time_t startRunTime = std::time(nullptr);

    /**
     * @brief Reads data from input files (weather, soil, management, plant traits).
     *
     * @param path The path to the input data files.
     * @param utils Utility class for helper functions.
     * @param parameter Contains the parameters for the simulation.
     * @param weather Weather data required for the simulation.
     * @param soil Soil data for the simulation.
     * @param management Management actions to be simulated.
     */
    input.getInputData(path, utils, parameter, weather, soil, management);

    /**
     * @brief Initializes variables and sets up initial conditions for the simulation.
     */
    init.initModelSimulation(utils, parameter, community, recruitment, soil, interaction, weather);

    initializeCouplingVariables(parameter, soil);

    /**
     * @brief Prepares output files for writing simulation results.
     */
    //output.prepareModelOutput(path, utils, parameter);

    /**
     * @brief Prints a summary of the simulation settings to the console.
     */
    output.printSimulationSettingsToConsole(parameter, input);
}

void PYTHONINTERFACE::initializeCouplingVariables(PARAMETER &parameter, SOIL &soil)
{
    couplingInterface_rootBiomass.assign(parameter.numberOfSoilLayers, 0.0);
    couplingInterface_rootVolume.assign(parameter.numberOfSoilLayers, 0.0);

    soil.couplingInterface_rootExudatesCarbon.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_rootExudatesNitrogen.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_rootLitterFluxCarbon.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_rootLitterFluxNitrogen.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_surfaceLitterFluxCarbon = 0.0;
    soil.couplingInterface_surfaceLitterFluxNitrogen = 0.0;

    soil.couplingInterface_soilWaterPotentialPerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_soilWaterNh4Concentration.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_soilWaterNo3Concentration.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_diffusionFactor.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_plantNh4UptakePerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_plantNo3UptakePerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_plantSoilWaterUptakePerLayer.assign(parameter.numberOfSoilLayers, 0.0);
    soil.couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer.assign(parameter.numberOfSoilLayers, 0.0);
}

void PYTHONINTERFACE::resetCouplingVariablesFluxes(PARAMETER &parameter, SOIL &soil)
{
    std::fill(soil.couplingInterface_rootExudatesCarbon.begin(),
              soil.couplingInterface_rootExudatesCarbon.end(), 0.0);

    std::fill(soil.couplingInterface_rootExudatesNitrogen.begin(),
              soil.couplingInterface_rootExudatesNitrogen.end(), 0.0);

    std::fill(soil.couplingInterface_rootLitterFluxCarbon.begin(),
              soil.couplingInterface_rootLitterFluxCarbon.end(), 0.0);

    std::fill(soil.couplingInterface_rootLitterFluxNitrogen.begin(),
              soil.couplingInterface_rootLitterFluxNitrogen.end(), 0.0);

    soil.couplingInterface_surfaceLitterFluxCarbon = 0.0;
    soil.couplingInterface_surfaceLitterFluxNitrogen = 0.0;

    // reset uptake values
    std::fill(soil.couplingInterface_plantNh4UptakePerSoilLayer.begin(), soil.couplingInterface_plantNh4UptakePerSoilLayer.end(), 0.0);
    std::fill(soil.couplingInterface_plantNo3UptakePerSoilLayer.begin(), soil.couplingInterface_plantNo3UptakePerSoilLayer.end(), 0.0);
}

void PYTHONINTERFACE::setRandomNumberGeneratorSeed(unsigned int seed)
{
    parameter.randomNumberGeneratorSeed = seed;
    init.initRandomNumberGeneratorSeed(parameter, community);
    std::cout << "Seed of random number generator is changed to: " << std::to_string(seed) << std::endl
              << std::endl;
}

void PYTHONINTERFACE::setInterfacesRunSimulationStepGetInterfaces(
    const double* waterPotential,
    const double* nh4Concentration,
    const double* no3Concentration,
    const double* diffusionFactor,
    size_t numberOfSoilLayers,
    double* rootVolume_managementTime,
    double* rootVolume_plantTime,
    double* waterUptake,
    double* no3Uptake,
    double* nh4Uptake,
    double* rootLitterInput,
    double* rootLitterInputCN,
    double* exudateInput,
    double* exudateInputCN,
    double* rootVolume,
    double* rootMass,
    double& soilWaterSurfaceInput,
    double& ETP,
    double& surfaceLitterInput,
    double& surfaceLitterInputCN)
{
    soil.couplingInterface_soilWaterPotentialPerSoilLayer.assign(waterPotential, waterPotential + numberOfSoilLayers);
    soil.couplingInterface_soilWaterNh4Concentration.assign(nh4Concentration, nh4Concentration + numberOfSoilLayers);
    soil.couplingInterface_soilWaterNo3Concentration.assign(no3Concentration, no3Concentration + numberOfSoilLayers);
    soil.couplingInterface_diffusionFactor.assign(diffusionFactor, diffusionFactor + numberOfSoilLayers);

    std::copy(couplingInterface_rootVolume.begin(),
              couplingInterface_rootVolume.end(),
              rootVolume_managementTime);

    std::copy(couplingInterface_rootVolume.begin(),
              couplingInterface_rootVolume.end(),
              rootVolume_plantTime);

    runSimulationStep();

    std::copy(soil.couplingInterface_plantSoilWaterUptakePerLayer.begin(),
              soil.couplingInterface_plantSoilWaterUptakePerLayer.end(),
              waterUptake);

    std::copy(soil.couplingInterface_plantNo3UptakePerSoilLayer.begin(),
              soil.couplingInterface_plantNo3UptakePerSoilLayer.end(),
              no3Uptake);

    std::copy(soil.couplingInterface_plantNh4UptakePerSoilLayer.begin(),
              soil.couplingInterface_plantNh4UptakePerSoilLayer.end(),
              nh4Uptake);

    auto rootLitter = getRootLitter();
    std::copy(rootLitter.begin(), rootLitter.end(), rootLitterInput);

    auto rootLitterCN = getRootLitterCnRatio();
    std::copy(rootLitterCN.begin(), rootLitterCN.end(), rootLitterInputCN);

    auto exudates = getRootExudates();
    std::copy(exudates.begin(), exudates.end(), exudateInput);

    auto exudateCN = getRootExudatesCnRatio();
    std::copy(exudateCN.begin(), exudateCN.end(), exudateInputCN);

    std::copy(couplingInterface_rootVolume.begin(), couplingInterface_rootVolume.end(), rootVolume);
    std::copy(couplingInterface_rootBiomass.begin(), couplingInterface_rootBiomass.end(), rootMass);

    soilWaterSurfaceInput = soil.couplingInterface_soilWaterSurfaceInput;
    ETP = soil.couplingInterface_potentialEvapotranspirationReducedByInterceptionSublimation;
    surfaceLitterInput = soil.couplingInterface_surfaceLitterFluxCarbon / CARBON_CONTENT_ODM;
    surfaceLitterInputCN = soil.couplingInterface_surfaceLitterFluxCarbon / soil.couplingInterface_surfaceLitterFluxNitrogen;
}

void PYTHONINTERFACE::runSimulationStep()
{
    resetCouplingVariablesFluxes(parameter, soil);
    if (parameter.day > parameter.simulationTimeInDays)
    {
        utils.handleError("Running more timesteps than configured!");
    }
    step.runModelSimulationStep(utils, parameter, init, allometry, community, recruitment, mortality, growth, management, soil, weather, interaction, output);
    parameter.day++; // increase day by one (initialized 1)
    couplingInterface_rootBiomass = calculateRootBiomassPerLayer(); 
    couplingInterface_rootVolume = calculateRootVolumeFromRootBiomass(couplingInterface_rootBiomass);
}

void PYTHONINTERFACE::writeResultsToOutputFiles()
{
    /**
     * @brief Writes the daily simulation results to output files.
     */
    output.writeSimulationResultsToOutputFiles(utils, parameter);

    /**
     * @brief Closes the output files after writing the results.
     */
    output.closeOutputFiles(utils, parameter);
}

// get output variables

double PYTHONINTERFACE::getShootBiomassAboveClippingHeightPerSquaremeter()
{
    return std::accumulate(community.clippedShootBiomassOfPlantsPerPFT.begin(), community.clippedShootBiomassOfPlantsPerPFT.end(), 0.0);
}

double PYTHONINTERFACE::getYieldPerSquaremeter()
{
    return community.biomassYield;
}

double PYTHONINTERFACE::getLai()
{
    return community.totalLeafAreaIndexOfPlantsInCommunity;
}

std::vector<double> PYTHONINTERFACE::getSoilWaterPerLayer()
{
    return soil.waterContent_soilWaterPoolPerSoilLayer;
}

double PYTHONINTERFACE::getMeanWaterLimitationFactor()
{
    double summedWaterLimitationFactor = 0;
    double numberOfPlants = 0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        summedWaterLimitationFactor += community.allPlants.at(cohortindex)->limitingFactorGppWater * community.allPlants.at(cohortindex)->amount;
        numberOfPlants += community.allPlants.at(cohortindex)->amount;
    }
    return summedWaterLimitationFactor/numberOfPlants;
}

double PYTHONINTERFACE::getMeanNitrogenLimitationFactor()
{
    double summedNitrogenLimitationFactor = 0;
    double numberOfPlants = 0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        summedNitrogenLimitationFactor += community.allPlants.at(cohortindex)->limitingFactorNppNitrogen * community.allPlants.at(cohortindex)->amount;
        numberOfPlants += community.allPlants.at(cohortindex)->amount;
    }
    return summedNitrogenLimitationFactor/numberOfPlants;
}

double PYTHONINTERFACE::getNumberOfPlants()
{
    return community.totalNumberOfPlantsInCommunity;
}

double PYTHONINTERFACE::getNumberOfPlantsAboveClippingHeight()
{
    double numberOfPlants = 0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->height > parameter.clippingHeightOfBiomassMeasurement)
        {
            numberOfPlants += community.allPlants.at(cohortindex)->amount;
        }
    }
    return numberOfPlants;
}

double PYTHONINTERFACE::getNumberOfPlantsAbove10cm()
{
    double numberOfPlants = 0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->height > 10.0) // 10cm is the mowing height
        {
            numberOfPlants += community.allPlants.at(cohortindex)->amount;
        }
    }
    return numberOfPlants;}

std::vector<double> PYTHONINTERFACE::getNumberOfPlantsPerHeight()
{
    // return number of plants per height in cm up to 1m and number of plants above 1m
    // for graphic runtimeoutput
    std::vector<double> numberOfPlantsPerHeight(101, 0.0);

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        int heightLayer = std::min(int(floor(community.allPlants.at(cohortindex)->height)), 100);

        numberOfPlantsPerHeight.at(heightLayer) += community.allPlants.at(cohortindex)->amount;
    }
    return numberOfPlantsPerHeight;
}

// get selfcoupling variables
std::vector<double> PYTHONINTERFACE::getPermanentWiltingPoint_plantSoilWaterUptake()
{
    return soil.couplingInterface_permanentWiltingPointPerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getFieldCapacity_plantSoilWaterUptake()
{
    return soil.couplingInterface_fieldCapacityPerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getPorosity_plantSoilWaterUptake()
{
    return soil.couplingInterface_porosityPerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getWaterContent_plantSoilWaterUptake()
{
    return soil.couplingInterface_waterContentPerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getWaterUptakeReductionByAvailableWater_plantSoilWaterUptake()
{
    return soil.couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getNitrogenContent_plantSoilNitrogenUptake()
{
    return soil.couplingInterface_nitrogenContentPerSoilLayer;
}

// set selfcoupling variables
void PYTHONINTERFACE::setPermanentWiltingPoint_plantSoilWaterUptake(std::vector<double> values)
{
    soil.couplingInterface_permanentWiltingPointPerSoilLayer = values;
}
void PYTHONINTERFACE::setFieldCapacity_plantSoilWaterUptake(std::vector<double> values)
{
    soil.couplingInterface_fieldCapacityPerSoilLayer = values;
}
void PYTHONINTERFACE::setPorosity_plantSoilWaterUptake(std::vector<double> values)
{
    soil.couplingInterface_porosityPerSoilLayer = values;
}
void PYTHONINTERFACE::setWaterContent_plantSoilWaterUptake(std::vector<double> values)
{
    soil.couplingInterface_waterContentPerSoilLayer = values;
}
void PYTHONINTERFACE::setWaterUptakeReductionByAvailableWater_plantSoilWaterUptake(std::vector<double> values)
{
    soil.couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer = values;
}
void PYTHONINTERFACE::setNitrogenContent_plantSoilNitrogenUptake(std::vector<double> values)
{
    soil.couplingInterface_nitrogenContentPerSoilLayer = values;
}

/// get coupling interface variables for external soil module
std::vector<double> PYTHONINTERFACE::getRootVolume()
{
    return couplingInterface_rootVolume;
}
std::vector<double> PYTHONINTERFACE::getRootBiomass()
{
    return couplingInterface_rootBiomass;
}
std::vector<double> PYTHONINTERFACE::getPlantWaterUptake()
{
    return soil.couplingInterface_plantSoilWaterUptakePerLayer;
}
std::vector<double> PYTHONINTERFACE::getPlantNo3Uptake()
{
    return soil.couplingInterface_plantNo3UptakePerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getPlantNh4Uptake()
{
    return soil.couplingInterface_plantNh4UptakePerSoilLayer;
}
std::vector<double> PYTHONINTERFACE::getRootLitter()
{
    std::vector<double> fluxInOdm(soil.couplingInterface_rootLitterFluxCarbon.size());
    std::transform(soil.couplingInterface_rootLitterFluxCarbon.begin(), soil.couplingInterface_rootLitterFluxCarbon.end(), fluxInOdm.begin(),
                   [](double x)
                   { return x / CARBON_CONTENT_ODM; });
    return fluxInOdm;
}
std::vector<double> PYTHONINTERFACE::getRootLitterCnRatio()
{
    std::vector<double> cnRatios(parameter.numberOfSoilLayers);
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        if (soil.couplingInterface_rootLitterFluxNitrogen.at(soilLayer) > 0)
        {
            cnRatios[soilLayer] = soil.couplingInterface_rootLitterFluxCarbon.at(soilLayer) / soil.couplingInterface_rootLitterFluxNitrogen.at(soilLayer);
        }
        else
        {
            cnRatios[soilLayer] =  std::numeric_limits<double>::quiet_NaN();
        }
    }
    return cnRatios;
}

std::vector<double> PYTHONINTERFACE::getRootExudates()
{
    std::vector<double> fluxInOdm(soil.couplingInterface_rootExudatesCarbon.size());
    std::transform(soil.couplingInterface_rootExudatesCarbon.begin(), soil.couplingInterface_rootExudatesCarbon.end(), fluxInOdm.begin(),
                   [](double x)
                   { return x / CARBON_CONTENT_ODM; });
    return fluxInOdm;
}
std::vector<double> PYTHONINTERFACE::getRootExudatesCnRatio()
{
    std::vector<double> cnRatios(parameter.numberOfSoilLayers);
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        if (soil.couplingInterface_rootExudatesNitrogen.at(soilLayer) > 0)
        {
            cnRatios[soilLayer] = soil.couplingInterface_rootExudatesCarbon.at(soilLayer) / soil.couplingInterface_rootExudatesNitrogen.at(soilLayer);
        }
        else
        {
            cnRatios[soilLayer] = std::numeric_limits<double>::quiet_NaN();
        }
    }
    return cnRatios;
}
double PYTHONINTERFACE::getSurfaceLitter()
{
    return soil.couplingInterface_surfaceLitterFluxCarbon / CARBON_CONTENT_ODM;
}
double PYTHONINTERFACE::getSurfaceLitterCnRatio()
{
    return soil.couplingInterface_surfaceLitterFluxCarbon / soil.couplingInterface_surfaceLitterFluxNitrogen;
}

double PYTHONINTERFACE::getSoilWaterSurfaceInput()
{
    return soil.couplingInterface_soilWaterSurfaceInput;
}
double PYTHONINTERFACE::getPetReducedByInterceptionSublimation()
{
    return soil.couplingInterface_potentialEvapotranspirationReducedByInterceptionSublimation;
}

// set coupling interface variables from external soil module
void PYTHONINTERFACE::setWaterPotential(std::vector<double> values)
{
    soil.couplingInterface_soilWaterPotentialPerSoilLayer = values;
}
void PYTHONINTERFACE::setNh4Concentration(std::vector<double> values)
{
    soil.couplingInterface_soilWaterNh4Concentration = values;
}
void PYTHONINTERFACE::setNo3Concentration(std::vector<double> values)
{
    soil.couplingInterface_soilWaterNo3Concentration = values;
}
void PYTHONINTERFACE::setDiffusionFactor(std::vector<double> values)
{
    soil.couplingInterface_diffusionFactor = values;
}

// helper functions

std::vector<double> PYTHONINTERFACE::calculateRootBiomassPerLayer()
{
    std::vector<double> rootMassPerLayer(parameter.numberOfSoilLayers);
    std::fill(rootMassPerLayer.begin(), rootMassPerLayer.end(), 0.0);

    for (int cohortindex = 0; cohortindex < community.allPlants.size(); cohortindex++)
    {

        for (int lay = 0; lay < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; lay++)
        {
            double depth = 0.0;
            if (lay < (community.allPlants.at(cohortindex)->numberOfSoilLayersRooting - 1))
            {
                rootMassPerLayer[lay] +=
                    community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->rootBiomass * parameter.soilLayerWidth[lay] / community.allPlants.at(cohortindex)->rootingDepth;
                depth += parameter.soilLayerWidth[lay];
            }
            else
            {
                rootMassPerLayer[lay] += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->rootBiomass *
                                         (1 - (depth / community.allPlants.at(cohortindex)->rootingDepth));
            }
        }
    }
    return rootMassPerLayer;
}

std::vector<double> PYTHONINTERFACE::calculateRootVolumeFromRootBiomass(
    std::vector<double> rootMassPerLayer)
{
    // calculation of volume from mass as in bodium, such that relation of volume to mass is the same as in
    // bodium for summeroats
    // TODO: think of changing this calculation to pft specific
    // values as in bodium, such that
    double meanRootDiameter = 7e-4;              // m
    double meanSpecificRootLength = 120000 * 1/1000; // m per kg * g/kg
    std::vector<double> rootVolumePerLayer(parameter.numberOfSoilLayers);
    std::fill(rootVolumePerLayer.begin(), rootVolumePerLayer.end(), 0.0);

    for (int lay = 0; lay < parameter.numberOfSoilLayers; lay++)
    {
        rootVolumePerLayer[lay] =
            PI * pow(meanRootDiameter / 2, 2) * rootMassPerLayer[lay] * meanSpecificRootLength;
    }

    return rootVolumePerLayer;
}
