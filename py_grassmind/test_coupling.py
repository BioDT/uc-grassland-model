from datetime import datetime, timedelta
import numpy as np
from py_grassmind import GrassmindModel
from py_bodium_finam import Model


#############global input###########################################

GCEF_data_path = "prepared_field_data_gcef/"


bodium_config = "parametrizations/bodium_config.json"
grassmind_config = "/home/kantzenb/Documents/GitLabProjects/uc-grassland-model/simulations/couplingProject/lat51.391900_lon11.878700__2013-01-01_2024-12-31__configuration__ambient_intensive_v1.txt"


#############calibration ############################################


def run_one_instances():
    biomass_yield_moisture = np.zeros((5, 365 * 8))

    ##bodium
    starttime = datetime(2013, 1, 1)
    bodium_instance = Model()
    bodium_instance.start(starttime, timedelta(days=1), bodium_config.encode(), 1)

    # grassmind
    days = 356 * 2

    grassmind_instance = GrassmindModel()
    grassmind_instance.initialize(grassmind_config.encode())

    for i in range(days):
        bodium_instance.setNodalValues(
            grassmind_instance.get_root_volume_per_soil_layer(),
            "RootVolume_ManagementTime".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_root_volume_per_soil_layer(),
            "RootVolume_PlantTime".encode(),
        )
        grassmind_instance.step()
        bodium_instance.setNodalValues(
            grassmind_instance.get_plant_soil_water_uptake_per_soil_layer(),
            "WaterUptake".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_plant_no3_uptake_per_soil_layer(),
            "No3Uptake".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_plant_nh4_uptake_per_soil_layer(),
            "Nh4Uptake".encode(),
        )
        bodium_instance.setNodalValues(
            np.array(grassmind_instance.get_root_litter_per_soil_layer()) / 1000,
            "RootLitterInput".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_root_litter_cn_ratio_per_soil_layer(),
            "RootLitterInputCN".encode(),
        )
        bodium_instance.setNodalValues(
            np.array(grassmind_instance.get_root_exudates_per_soil_layer()) / 1000,
            "ExudateInput".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_root_exudates_cn_ratio_per_soil_layer(),
            "ExudateInputCN".encode(),
        )
        bodium_instance.setNodalValues(
            grassmind_instance.get_root_volume_per_soil_layer(),
            "RootVolume".encode(),
        )
        bodium_instance.setNodalValues(
            np.array(grassmind_instance.get_root_biomass_per_soil_layer()) / 1000,
            "RootMass".encode(),
        )
        bodium_instance.setValue(
            grassmind_instance.get_soil_water_surface_input(),
            "SoilWaterSurfaceInput".encode(),
        )
        bodium_instance.setValue(
            grassmind_instance.get_pet_reduced_by_interception_sublimation(),
            "ETP".encode(),
        )
        bodium_instance.setValue(
            np.array(grassmind_instance.get_surface_litter()) / 1000,
            "SurfaceLitterInput".encode(),
        )
        bodium_instance.setValue(
            grassmind_instance.get_surface_litter_cn_ratio(),
            "SurfaceLitterInputCN".encode(),
        )
        bodium_instance.setValue(
            bool(grassmind_instance.get_lai() > 0),
            "GrowthSeason".encode(),
        )
        bodium_instance.setValue(
            0.0,
            "NitrogenFixationRate".encode(),
        )
        bodium_instance.step()
        grassmind_instance.set_water_potential_per_soil_layer(
            bodium_instance.getNodalValues("WaterPotential".encode()),
        )
        grassmind_instance.set_nh4_concentration_per_soil_layer(
            bodium_instance.getNodalValues("Nh4Concentration".encode()),
        )
        grassmind_instance.set_no3_concentration_per_soil_layer(
            bodium_instance.getNodalValues("No3Concentration".encode()),
        )

        biomass_yield_moisture[0, i] = (
            grassmind_instance.get_biomass_above_clipping_height_per_squaremeter()
        )
        biomass_yield_moisture[1, i] = grassmind_instance.get_yield_per_squaremeter()

        soil_moisture = bodium_instance.getNodalValues(
            "SoilMoisturePerFreshMass".encode()
        )

        biomass_yield_moisture[2, i] = (
            (soil_moisture[0] * 10 + soil_moisture[1] * 5) / 15 * 100
        )
        biomass_yield_moisture[3, i] = (
            (soil_moisture[1] * 5 + soil_moisture[2] * 10) / 15 * 100
        )
        biomass_yield_moisture[4, i] = (
            (soil_moisture[3] * 10 + soil_moisture[4] * 10) / 20 * 100
        )

        print(grassmind_instance.get_lai())

    # return biomass_yield_moisture


run_one_instances()
