#include <iostream>
#include <ctime>
#include <vector>

#include "../module_input/input.h"
#include "../module_output/output.h"
#include "../module_parameter/parameter.h"
#include "../module_weather/weather.h"
#include "../module_soil/soil.h"
#include "../module_management/management.h"
#include "../module_init/init.h"
#include "../module_step/step.h"
#include "../module_plant/community.h"
#include "../module_interaction/interaction.h"
#include "../utils/utils.h"

#include "../module_recruitment/recruitment.h"
#include "../module_mortality/mortality.h"
#include "../module_plant/allometry.h"
#include "../module_growth/growth.h"

/**
 * @brief Class to execute the model simulation from python binding.
 *
 */

class PYTHONINTERFACE
{
public:
    PYTHONINTERFACE();

    /**
     * @brief Create instances of the required classes.
     */
    OUTPUT output;
    INPUT input;
    UTILS utils;
    PARAMETER parameter;
    WEATHER weather;
    SOIL soil;
    MANAGEMENT management;
    ALLOMETRY allometry;
    INIT init;
    STEP step;
    COMMUNITY community;
    RECRUITMENT recruitment;
    MORTALITY mortality;
    GROWTH growth;
    INTERACTION interaction;

    // coupling interface output variables, cacluated end of timestep
    std::vector<double> couplingInterface_rootBiomass;
    std::vector<double> couplingInterface_rootVolume;

    // base functions
    void initializeSimulation(std::string path);
    void initializeCouplingVariables(PARAMETER &parameter, SOIL &soil);
    void resetCouplingVariablesFluxes(PARAMETER &parameter, SOIL &soil);
    void runSimulationStep();
    void writeResultsToOutputFiles();

    // get output variables
    double getShootBiomassAboveClippingHeightPerSquaremeter();
    double getYieldPerSquaremeter();
    double getLai();
    std::vector<double> getSoilWaterPerLayer();

    // get selfcoupling variables
    std::vector<double> getPermanentWiltingPoint_plantSoilWaterUptake();
    std::vector<double> getFieldCapacity_plantSoilWaterUptake();
    std::vector<double> getPorosity_plantSoilWaterUptake();
    std::vector<double> getWaterContent_plantSoilWaterUptake();
    std::vector<double> getWaterUptakeReductionByAvailableWater_plantSoilWaterUptake();
    std::vector<double> getNitrogenContent_plantSoilNitrogenUptake();

    // set selfcoupling variables
    void setPermanentWiltingPoint_plantSoilWaterUptake(std::vector<double> values);
    void setFieldCapacity_plantSoilWaterUptake(std::vector<double> values);
    void setPorosity_plantSoilWaterUptake(std::vector<double> values);
    void setWaterContent_plantSoilWaterUptake(std::vector<double> values);
    void setWaterUptakeReductionByAvailableWater_plantSoilWaterUptake(std::vector<double> values);
    void setNitrogenContent_plantSoilNitrogenUptake(std::vector<double> values);

    // get coupling interface variables for external soil module
    std::vector<double> getRootVolume();
    std::vector<double> getRootBiomass();
    std::vector<double> getPlantWaterUptake();
    std::vector<double> getPlantNo3Uptake();
    std::vector<double> getPlantNh4Uptake();
    std::vector<double> getRootLitter();
    std::vector<double> getRootLitterCnRatio();
    std::vector<double> getRootExudates();
    std::vector<double> getRootExudatesCnRatio();
    double getSurfaceLitter();
    double getSurfaceLitterCnRatio();
    double getSoilWaterSurfaceInput();
    double getPetReducedByInterceptionSublimation();

    // set coupling interface variables from external soil module
    void setWaterPotential(std::vector<double> values);
    void setNh4Concentration(std::vector<double> values);
    void setNo3Concentration(std::vector<double> values);

    // helper functions
    std::vector<double> calculateRootVolumeFromRootBiomass(std::vector<double> rootMassPerLayer);
    std::vector<double> calculateRootBiomassPerLayer();
};