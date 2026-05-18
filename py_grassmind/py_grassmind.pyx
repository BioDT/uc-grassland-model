# distutils: language=c++
#cython: language_level=3, boundscheck=False, wraparound=False, nonecheck=False
#@PydevCodeAnalysisIgnore

import os
import warnings
import numpy as np


cdef char* to_char_string(variable):
    if not isinstance(variable, bytes):
        variable = str.encode(str(variable))
    return variable


cdef class GrassmindModel(): 
    
    cdef PYTHONINTERFACE py_grassmind


    cdef bool initialized
    cdef bool parFileHasBeenRead

    def __init__(self):
        self.initialized = False

    def __cinit__(self):
        self.parFileHasBeenRead = False


    def initialize(self, char* path):
        self.py_grassmind.initializeSimulation(path)
        self.initialized = True

    def set_random_number_generator_seed(self, unsigned int seed):
        self.py_grassmind.setRandomNumberGeneratorSeed(seed)

    def step(self):
        if self.initialized:
            self.py_grassmind.runSimulationStep()
        else:
            raise RuntimeError("Model needs to be initialized before a step can be performed!")

    def writeOutputFiles(self):
        self.py_grassmind.writeResultsToOutputFiles()
    
    def get_biomass_above_clipping_height_per_squaremeter(self):
        return self.py_grassmind.getShootBiomassAboveClippingHeightPerSquaremeter()
   
    def get_yield_per_squaremeter(self):
        return self.py_grassmind.getYieldPerSquaremeter()

    def get_lai(self):
        return self.py_grassmind.getLai()

    def get_mean_water_limitation_factor(self):
        return self.py_grassmind.getMeanWaterLimitationFactor()

    def get_mean_nitrogen_limitation_factor(self):
        return self.py_grassmind.getMeanNitrogenLimitationFactor()

    def get_soil_water_per_soil_layer(self):
        return self.py_grassmind.getSoilWaterPerLayer()

    def get_number_of_plants_per_height(self):
        return self.py_grassmind.getNumberOfPlantsPerHeight()

    ## get selfcoupling variables

    def get_permanent_wilting_point_at_plant_soil_water_uptake(self):
        return self.py_grassmind.getPermanentWiltingPoint_plantSoilWaterUptake()

    def get_field_capacity_at_plant_soil_water_uptake(self):
        return self.py_grassmind.getFieldCapacity_plantSoilWaterUptake()

    def get_porosity_at_plant_soil_water_uptake(self):
        return self.py_grassmind.getPorosity_plantSoilWaterUptake()

    def get_water_content_at_plant_soil_water_uptake(self):
        return self.py_grassmind.getWaterContent_plantSoilWaterUptake()

    def get_water_uptake_reduction_by_available_water_at_plant_soil_water_uptake(self):
        return self.py_grassmind.getWaterUptakeReductionByAvailableWater_plantSoilWaterUptake()

    def get_nitrogen_content_at_plant_soil_nitrogen_uptake(self):
        return self.py_grassmind.getNitrogenContent_plantSoilNitrogenUptake()

    ## set selfcoupling variables

    def set_permanent_wilting_point_at_plant_soil_water_uptake(self, vector[double] values):
        self.py_grassmind.setPermanentWiltingPoint_plantSoilWaterUptake(values)

    def set_field_capacity_at_plant_soil_water_uptake(self, vector[double] values):
        self.py_grassmind.setFieldCapacity_plantSoilWaterUptake(values)

    def set_porosity_at_plant_soil_water_uptake(self, vector[double] values):
        self.py_grassmind.setPorosity_plantSoilWaterUptake(values)

    def set_water_content_at_plant_soil_water_uptake(self, vector[double] values):
        self.py_grassmind.setWaterContent_plantSoilWaterUptake(values)

    def set_water_uptake_reduction_by_available_water_at_plant_soil_water_uptake(self, vector[double] values):
        self.py_grassmind.setWaterUptakeReductionByAvailableWater_plantSoilWaterUptake(values)

    def set_nitrogen_content_at_plant_soil_nitrogen_uptake(self, vector[double] values):
        self.py_grassmind.setNitrogenContent_plantSoilNitrogenUptake(values)

    ## get external soil module coupling variables
    def get_root_volume_per_soil_layer(self):
        return self.py_grassmind.getRootVolume()
    def get_root_biomass_per_soil_layer(self):
        return self.py_grassmind.getRootBiomass()
    def get_plant_soil_water_uptake_per_soil_layer(self):
        return self.py_grassmind.getPlantWaterUptake()
    def get_plant_no3_uptake_per_soil_layer(self):
        return self.py_grassmind.getPlantNo3Uptake()
    def get_plant_nh4_uptake_per_soil_layer(self):
        return self.py_grassmind.getPlantNh4Uptake()
    def get_root_litter_per_soil_layer(self):
        return self.py_grassmind.getRootLitter()
    def get_root_litter_cn_ratio_per_soil_layer(self):
        return self.py_grassmind.getRootLitterCnRatio()
    def get_root_exudates_per_soil_layer(self):
        return self.py_grassmind.getRootExudates()
    def get_root_exudates_cn_ratio_per_soil_layer(self):
        return self.py_grassmind.getRootExudatesCnRatio()
    def get_surface_litter(self):
        return self.py_grassmind.getSurfaceLitter()
    def get_surface_litter_cn_ratio(self):
        return self.py_grassmind.getSurfaceLitterCnRatio()
    def get_soil_water_surface_input(self):
        return self.py_grassmind.getSoilWaterSurfaceInput()
    def get_pet_reduced_by_interception_sublimation(self):
        return self.py_grassmind.getPetReducedByInterceptionSublimation()

    ## set extern soil module coupling variables
    def set_water_potential_per_soil_layer(self, vector[double] values):
        self.py_grassmind.setWaterPotential(values)
    def set_nh4_concentration_per_soil_layer(self, vector[double] values):
        self.py_grassmind.setNh4Concentration(values)
    def set_no3_concentration_per_soil_layer(self, vector[double] values):
        self.py_grassmind.setNo3Concentration(values)
    def set_diffusion_factor_per_soil_layer(self, vector[double] values):
        self.py_grassmind.setDiffusionFactor(values)