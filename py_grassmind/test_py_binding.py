from py_grassmind import GrassmindModel

grassmind_config = "/home/kantzenb/Documents/GitLabProjects/uc-grassland-model/simulations/couplingProject/lat51.391900_lon11.878700__2013-01-01_2024-12-31__configuration__ambient_intensive_v1.txt"


if __name__ == "__main__":
    model1 = GrassmindModel()
    # config = "../simulations/exampleProject/lat51.391900_lon11.878700__2013-01-01_2023-12-31__configuration__generic_v1.txt"
    # config = "../simulations/couplingProject/lat51.391900_lon11.878700__2013-01-01_2024-12-31__configuration__ambient_intensive_v1.txt"
    model1.initialize(grassmind_config.encode())
    days = 356 * 2

    for day in range(days):
        model1.step()

        print("lai " + str(model1.get_lai()))

    # model1.writeOutputFiles()
