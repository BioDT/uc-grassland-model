# coding=utf-8
# distutils: language=c++
#@PydevCodeAnalysisIgnore
#cython: language_level=3
###, boundscheck=False, wraparound=False, nonecheck=False

from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stddef cimport size_t




cdef extern from "../src/pythoninterface/pythoninterface.h":
    cdef cppclass PYTHONINTERFACE:
        PYTHONINTERFACE() except + nogil
        void initializeSimulation(string path) except + nogil
        void setRandomNumberGeneratorSeed(unsigned int seed) except + nogil
        void runSimulationStep() except + nogil
        void writeResultsToOutputFiles() except + nogil
        double getShootBiomassAboveClippingHeightPerSquaremeter() except + nogil
        double getYieldPerSquaremeter() except + nogil
        double getLai() except + nogil
        double getMeanWaterLimitationFactor() except + nogil
        double getMeanNitrogenLimitationFactor() except + nogil
        double getNumberOfPlants() except + nogil
        double getNumberOfPlantsAboveClippingHeight() except + nogil
        double getNumberOfPlantsAbove10cm() except + nogil
        vector[double] getSoilWaterPerLayer() except + nogil
        vector[double] getNumberOfPlantsPerHeight() except + nogil

        ## get selfcoupling variables
        vector[double] getPermanentWiltingPoint_plantSoilWaterUptake() except + nogil
        vector[double] getFieldCapacity_plantSoilWaterUptake() except + nogil
        vector[double] getPorosity_plantSoilWaterUptake() except + nogil
        vector[double] getWaterContent_plantSoilWaterUptake() except + nogil
        vector[double] getWaterUptakeReductionByAvailableWater_plantSoilWaterUptake() except + nogil
        vector[double] getNitrogenContent_plantSoilNitrogenUptake() except + nogil

        ## set selfcoupling variables
        void setPermanentWiltingPoint_plantSoilWaterUptake(vector[double] values) except + nogil
        void setFieldCapacity_plantSoilWaterUptake(vector[double] values) except + nogil
        void setPorosity_plantSoilWaterUptake(vector[double] values) except + nogil
        void setWaterContent_plantSoilWaterUptake(vector[double] values) except + nogil
        void setWaterUptakeReductionByAvailableWater_plantSoilWaterUptake(vector[double] values) except + nogil
        void setNitrogenContent_plantSoilNitrogenUptake(vector[double] values) except + nogil

        ## get external soil module coupling variables
        vector[double] getRootVolume() except + nogil
        vector[double] getRootBiomass() except + nogil
        vector[double] getPlantWaterUptake() except + nogil
        vector[double] getPlantNo3Uptake() except + nogil
        vector[double] getPlantNh4Uptake() except + nogil
        vector[double] getRootLitter() except + nogil
        vector[double] getRootLitterCnRatio() except + nogil
        vector[double] getRootExudates() except + nogil
        vector[double] getRootExudatesCnRatio() except + nogil
        double getSurfaceLitter() except + nogil
        double getSurfaceLitterCnRatio() except + nogil
        double getSoilWaterSurfaceInput() except + nogil
        double getPetReducedByInterceptionSublimation() except + nogil

        ## set extern soil module coupling variables
        void setWaterPotential(vector[double] values) except + nogil
        void setNh4Concentration(vector[double] values) except + nogil
        void setNo3Concentration(vector[double] values) except + nogil
        void setDiffusionFactor(vector[double] values) except + nogil

        ## fast coupling
        void setInterfacesRunSimulationStepGetInterfaces(
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
            double& surfaceLitterInputCN) except + nogil

