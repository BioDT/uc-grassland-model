/**
 * @file output.h
 * @brief Declares the OUTPUT class, which manages all file-based simulation output.
 *
 * OUTPUT supports eight parallel output streams, each controlled by a boolean flag
 * in PARAMETER:
 * - **Community** — daily community-level aggregates (plant count, LAI, C/N balance).
 * - **PFT population** — per-PFT biomass, GPP, NPP, and respiration.
 * - **Plant cohort** — per-cohort geometry, biomass pools, and process rates.
 * - **Soil carbon** — daily C pool contents and inter-pool fluxes.
 * - **Soil nitrogen** — daily N pool contents, fluxes, mineralisation, and leaching.
 * - **Soil water** — daily hydrological fluxes (interception, run-off, snow, ET).
 * - **Soil resources per layer** — per-layer water content and mineral N.
 * - **Soil fluxes details** — decisive C/N ratios, decomposition respiratory losses,
 *   immobilisation, and mineralisation flux breakdown.
 *
 * Each stream follows the same lifecycle:
 * 1. prepareModelOutput() — create folder, open files, write headers, load date list.
 * 2. Per time step: fill the corresponding `buffer*` stream, then call
 *    writeSimulationResultsToOutputFiles() to flush to disk.
 * 3. closeOutputFiles() — flush and close all open streams at simulation end.
 */
#pragma once
#include "../module_parameter/parameter.h"
#include "../module_plant/community.h"
#include "../module_input/input.h"
#include "../module_soil/soil.h"
#include "../module_management/management.h"
#include "../module_recruitment/recruitment.h"
#include "../utils/utils.h"
#include <iostream>
#include <vector>
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/**
 * @class OUTPUT
 * @brief Creates, manages, and writes all simulation output files.
 *
 * The main entry point is prepareModelOutput(), which must be called once before
 * the simulation loop. Inside the loop, caller code fills the `buffer*` members
 * and calls writeSimulationResultsToOutputFiles() to flush them. At simulation
 * end, closeOutputFiles() closes all streams.
 */
class OUTPUT
{
public:
    OUTPUT();
    ~OUTPUT();

    /** @brief Resolved path to the `output/` directory where result files are written. */
    std::string outputDirectory;

    /** @brief Resolved path to the optional output-writing-dates file. */
    std::string fileDirectory;

    /** @brief File stream for the community-level output file. */
    std::ofstream outputCommunity;

    /** @brief File stream for the PFT population output file. */
    std::ofstream outputPFTPopulation;

    /** @brief File stream for the plant-cohort output file. */
    std::ofstream outputPlant;

    /** @brief File stream for the soil carbon output file. */
    std::ofstream outputSoilCarbon;

    /** @brief File stream for the soil nitrogen output file. */
    std::ofstream outputSoilNitrogen;

    /** @brief File stream for the soil water output file. */
    std::ofstream outputSoilWater;

    /** @brief File stream for the per-soil-layer resources output file. */
    std::ofstream outputSoilResourcesPerSoilLayer;

    /** @brief File stream for the detailed soil-flux output file. */
    std::ofstream outputSoilFluxesDetails;

    /**
     * @brief In-memory write buffer for community-level output.
     */
    std::stringstream bufferCommunity;

    /**
     * @brief In-memory write buffer for PFT population output.
     */
    std::stringstream bufferPFTPopulation;

    /**
     * @brief In-memory write buffer for plant-cohort output.
     */
    std::stringstream bufferPlant;

    /**
     * @brief In-memory write buffer for soil carbon output.
     */
    std::stringstream bufferSoilCarbon;

    /**
     * @brief In-memory write buffer for soil nitrogen output.
     */
    std::stringstream bufferSoilNitrogen;

    /**
     * @brief In-memory write buffer for soil water output.
     */
    std::stringstream bufferSoilWater;

    /**
     * @brief In-memory write buffer for per-soil-layer resources output.
     */
    std::stringstream bufferSoilResourcesPerSoilLayer;

    /**
     * @brief In-memory write buffer for detailed soil-flux output.
     */
    std::stringstream bufferSoilFluxesDetails;

    /**
     * @brief Simulation day indices (1-based offsets from the reference Julian start
     *        day) on which output should be written.
     */
    std::vector<int> outputWritingDates;

    /**
     * @brief `true` if the output-writing-dates file was opened successfully;
     *        `false` if the file was absent, not specified, or could not be read,
     *        in which case output falls back to daily writing.
     */
    bool outputWritingDatesFileOpened;

    /**
     * @brief Aggregates per-cohort state variables into PFT-level and community-level
     *        output accumulators, and computes ecosystem-scale C/N balances.
     */
    void updateVegetationStateVariablesForOutput(PARAMETER parameter, COMMUNITY &community, SOIL soil, MANAGEMENT management, RECRUITMENT recruitment);

    /**
     * @brief Prepares all output infrastructure before the simulation loop starts.
     */
    void prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter);

    /**
     * @brief Creates the `output/` subdirectory adjacent to the configuration file.
     */
    void createOutputFolder(std::string path, UTILS utils);

    /**
     * @brief Loads the optional list of days on which output should be written.
     */
    void openAndReadOutputWritingDates(std::string path, UTILS utils, PARAMETER &parameter);

    /**
     * @brief Prints a summary of simulation settings and input-file status to stdout.
     */
    void printSimulationSettingsToConsole(PARAMETER parameter, INPUT input);

    /**
     * @brief Constructs output file names and opens one stream per enabled output type.
     */
    void createAndOpenOutputFiles(UTILS utils, PARAMETER parameter);

    /**
     * @brief Writes tab-separated column headers to all open output files.
     */
    void writeHeaderInOutputFiles(UTILS utils, PARAMETER parameter);

    /**
     * @brief Flushes all in-memory `buffer*` streams to the corresponding output files.
     */
    void writeSimulationResultsToOutputFiles(UTILS utils, PARAMETER parameter);

    /**
     * @brief Closes all open output file streams at simulation end.
     */
    void closeOutputFiles(UTILS utils, PARAMETER parameter);
};