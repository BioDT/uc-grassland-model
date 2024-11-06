#version 3.11 or later of CMake needed
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)

project(
    grassDTmodel
    VERSION 1.0
    DESCRIPTION "Model component of the Digital Twin pipeline for grassland biodiversity")

#Copy header files from src to include directory
#default DESTINATION is the created build folder
set(DESTINATION build/)
file(COPY src/utils/utils.h DESTINATION include/utils)
file(COPY src/module_output/output.h DESTINATION include/module_output)
file(COPY src/module_input/input.h DESTINATION include/module_input)
file(COPY src/module_parameter/parameter.h DESTINATION include/module_parameter)
file(COPY src/module_weather/weather.h DESTINATION include/module_weather)
file(COPY src/module_soil/soil.h DESTINATION include/module_soil)
file(COPY src/module_management/management.h DESTINATION include/module_management)
file(COPY src/module_plant/community.h DESTINATION include/module_plant)
file(COPY src/module_plant/plant.h DESTINATION include/module_plant)
file(COPY src/module_plant/allometry.h DESTINATION include/module_plant)
file(COPY src/module_init/init.h DESTINATION include/module_init)
file(COPY src/module_step/step.h DESTINATION include/module_step)
file(COPY src/module_growth/growth.h DESTINATION include/module_growth)
file(COPY src/module_mortality/mortality.h DESTINATION include/module_mortality)
file(COPY src/module_recruitment/recruitment.h DESTINATION include/module_recruitment)
file(COPY src/module_interaction/interaction.h DESTINATION include/module_interaction)

#Define source files
set(SOURCES
    src/main.cpp
    src/utils/utils.cpp
    src/module_output/output.cpp
    src/module_input/input.cpp
    src/module_parameter/parameter.cpp
    src/module_weather/weather.cpp
    src/module_soil/soil.cpp
    src/module_management/management.cpp
    src/module_plant/community.cpp
    src/module_plant/plant.cpp
    src/module_plant/allometry.cpp
    src/module_init/init.cpp
    src/module_step/step.cpp
    src/module_growth/growth.cpp
    src/module_mortality/mortality.cpp
    src/module_recruitment/recruitment.cpp
    src/module_interaction/interaction.cpp
)

#Define header files
set(HEADERS
    include/utils/utils.h
    include/module_output/output.h
    include/module_input/input.h
    include/module_parameter/parameter.h
    include/module_weather/weather.h
    include/module_soil/soil.h
    include/module_management/management.h
    include/module_plant/community.h
    include/module_plant/plant.h
    include/module_plant/allometry.h
    include/module_init/init.h
    include/module_step/step.h
    include/module_growth/growth.h
    include/module_mortality/mortality.h
    include/module_recruitment/recruitment.h
    include/module_interaction/interaction.h
)

#Create executable target
add_executable(grassDTmodel ${SOURCES} ${HEADERS})

#Set include directories for the target
target_include_directories(grassDTmodel PUBLIC include)