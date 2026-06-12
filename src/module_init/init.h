/**
 * @file init.h
 * @brief Declares the INIT class responsible for model initialisation at simulation start.
 *
 * The INIT class sets all state variables to their start-of-simulation values and
 * prepares per-time-step accumulator variables for their first use. It covers
 * vegetation (plant cohorts, recruitment pools), interaction (LAI arrays), and
 * soil resource (C/N pools, water content, flux variables) initialisation.
 */
#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_recruitment/recruitment.h"
#include "../module_soil/soil.h"
#include "../module_interaction/interaction.h"
#include "../utils/utils.h"
#include <iostream>
#include <vector>
#include <random>
#include <limits>

/**
 * @class INIT
 * @brief Provides all initialisation routines run once at the start of a simulation.
 */
class INIT
{
public:
    INIT();
    ~INIT();

    /**
     * @brief Initialises all model state variables at the start of a simulation.
     */
    void initModelSimulation(UTILS utils, PARAMETER &parameter, COMMUNITY &community, RECRUITMENT &recruitment, SOIL &soil, INTERACTION &interaction, WEATHER weather);

    /**
     * @brief Sets the simulation day counter to 1.
     */
    void initTimeVariables(PARAMETER &parameter);

    /**
     * @brief Seeds the random number generator.
     */
    void initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community);

    /**
     * @brief Clears plant cohort lists, recruitment pools, and mortality litter pools.
     */
    void initVegetationStateVariables(COMMUNITY &community, PARAMETER parameter, RECRUITMENT &recruitment, SOIL &soil);

    /**
     * @brief Resets all per-time-step vegetation process and output accumulator variables.
     */
    void resetVegetationProcessVariables(PARAMETER parameter, RECRUITMENT &recruitment, COMMUNITY &community, INTERACTION &interaction, SOIL soil);

    /**
     * @brief Initialises soil resource state variables to their start-of-simulation values.
     */
    void initSoilResourceStateVariables(UTILS utils, SOIL &soil, WEATHER weather, PARAMETER parameter);

    /**
     * @brief Estimates the initial total carbon content of all soil pools.
     */
    double calculateInitialCarbonContentOfAllSoilPools(UTILS utils, WEATHER weather, PARAMETER parameter, SOIL soil);

    /**
     * @brief Resets all soil process and flux variables to zero.
     */
    void resetSoilResourceProcessAndFluxVariables(UTILS utils, PARAMETER parameter, SOIL &soil);
};