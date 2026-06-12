/**
 * @file step.h
 * @brief Declares the STEP class, which drives the main simulation loop and
 *        all per-day output buffering.
 *
 * STEP is the top-level orchestrator of a GRASSMIND simulation run:
 * - runModelSimulation() iterates over all simulation days.
 * - runModelSimulationStep() resets accumulators, reads weather, calls all
 *   ecological modules, and saves results for one day.
 * - doDayStepOfModelSimulation() executes the six ecological modules in the
 *   required daily order.
 * - A family of `fill*Buffer()` helpers write today's state into the
 *   corresponding OUTPUT string-stream buffers.
 * - saveSimulationResultsToBuffer() converts the current day to an ISO date,
 *   calls all fill helpers, and flushes the buffers to disk.
 */
#pragma once
#include "../module_parameter/parameter.h"
#include "../module_init/init.h"
#include "../module_plant/community.h"
#include "../module_plant/plant.h"
#include "../module_management/management.h"
#include "../module_plant/allometry.h"
#include "../module_recruitment/recruitment.h"
#include "../module_mortality/mortality.h"
#include "../module_growth/growth.h"
#include "../module_output/output.h"
#include "../module_soil/soil.h"
#include "../module_weather/weather.h"
#include "../module_interaction/interaction.h"
#include "../utils/utils.h"
#include <random>

/**
 * @class STEP
 * @brief Top-level simulation controller; drives the daily loop and output.
 *
 * STEP has no persistent state of its own — it only holds methods. The entry
 * point for a complete simulation run is runModelSimulation(); for a single
 * pre-configured day (e.g. in coupling scenarios) use runModelSimulationStep().
 */
class STEP
{
public:
    STEP();
    ~STEP();

    /**
     * @brief Runs the full simulation period by iterating over all days.
     */
    void runModelSimulation(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output);

    /**
     * @brief Executes all sub-processes for a single simulation day.
     */
    void runModelSimulationStep(UTILS utils, PARAMETER &parameter, INIT init, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, MANAGEMENT management, SOIL &soil, WEATHER weather, INTERACTION &interaction, OUTPUT &output);

    /**
     * @brief Runs all ecological process modules for a single day in the
     *        required order.
     */
    void doDayStepOfModelSimulation(UTILS utils, PARAMETER &parameter, ALLOMETRY allometry, COMMUNITY &community, RECRUITMENT &recruitment, MORTALITY mortality, GROWTH growth, INTERACTION &interaction, MANAGEMENT management, SOIL &soil, WEATHER weather);

    /**
     * @brief Converts the current day to an ISO date, fills all output buffers,
     *        and flushes them to disk.
     */
    void saveSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, INTERACTION interaction, OUTPUT &output, SOIL soil);

    /**
     * @brief Appends one row of community-level aggregates to
     *        `output.bufferCommunity`.
     */
    void fillCommunityBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row per PFT of population-level variables to
     *        `output.bufferPFTPopulation`.
     */
    void fillPFTBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row per cohort of individual-level variables to
     *        `output.bufferPlant`.
     */
    void fillPlantCohortBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row of soil C pool contents and inter-pool C fluxes
     *        to `output.bufferSoilCarbon`.
     */
    void fillSoilCarbonBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row of soil N pool contents, fluxes, mineralisation,
     *        and leaching variables to `output.bufferSoilNitrogen`.
     */
    void fillSoilNitrogenBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row of hydrological flux variables to
     *        `output.bufferSoilWater`.
     */
    void fillSoilWaterBuffer(UTILS utils, PARAMETER parameter, SOIL soil, INTERACTION interaction, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row per soil layer of per-layer water and mineral
     *        nitrogen to `output.bufferSoilResourcesPerSoilLayer`.
     */
    void fillSoilResourcePerSoilLayerBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date);

    /**
     * @brief Appends one row of detailed decomposition diagnostics to
     *        `output.bufferSoilFluxesDetails`.
     */
    void fillSoilFluxesDetailsBuffer(UTILS utils, PARAMETER parameter, SOIL soil, OUTPUT &output, std::string date);

    /**
     * @brief Delegates to all eight `fill*Buffer()` helpers for the given date.
     */
    void fillSimulationResultsToBuffer(UTILS utils, PARAMETER parameter, COMMUNITY community, INTERACTION interaction, OUTPUT &output, SOIL soil, std::string date);
};
