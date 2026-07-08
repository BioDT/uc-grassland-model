import numpy as np

from py_grassmind import GrassmindModel

lai_inputModel = np.zeros(700)
lai_outputModel = np.zeros(700)

output_model = GrassmindModel()

config_output = "../simulations/exampleCouplingProject/lat51.391900_lon11.878700__2013-01-01_2023-12-31__configuration__generic_v1_selfCoupling_setV.txt"

output_model.initialize(config_output.encode())
input_model = GrassmindModel()
config_input = "../simulations/exampleCouplingProject/lat51.391900_lon11.878700__2013-01-01_2023-12-31__configuration__generic_v1_selfCoupling_setV.txt"
input_model.initialize(config_input.encode())

for i in range(700):
    output_model.step()
    soilWaterPerLayer = output_model.get_water_content_at_plant_soil_water_uptake()
    permanentWilkingPointPerLayer = (
        output_model.get_permanent_wilting_point_at_plant_soil_water_uptake()
    )
    fieldCapacityPerLayer = output_model.get_field_capacity_at_plant_soil_water_uptake()
    soilNitrogenPerLayer = (
        output_model.get_nitrogen_content_at_plant_soil_nitrogen_uptake()
    )
    waterUptakeReductionByAvailableWater = (
        output_model.get_water_uptake_reduction_by_available_water_at_plant_soil_water_uptake()
    )
    porosityPerLayer = output_model.get_porosity_at_plant_soil_water_uptake()
    lai_outputModel[i] = output_model.get_lai()

    input_model.set_water_content_at_plant_soil_water_uptake(soilWaterPerLayer)
    input_model.set_permanent_wilting_point_at_plant_soil_water_uptake(
        permanentWilkingPointPerLayer
    )
    input_model.set_field_capacity_at_plant_soil_water_uptake(fieldCapacityPerLayer)
    input_model.set_nitrogen_content_at_plant_soil_nitrogen_uptake(soilNitrogenPerLayer)
    input_model.set_water_uptake_reduction_by_available_water_at_plant_soil_water_uptake(
        waterUptakeReductionByAvailableWater
    )
    input_model.set_porosity_at_plant_soil_water_uptake(porosityPerLayer)
    input_model.step()
    lai_inputModel[i] = input_model.get_lai()

print("max LAI: " + str(np.max(lai_outputModel)))
print("max difference: " + str(np.max(np.abs(lai_outputModel - lai_inputModel))))
