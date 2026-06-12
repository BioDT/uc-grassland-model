/**
 * @file management.h
 * @brief Declares the MANAGEMENT class, which applies scheduled land-management
 *        actions (mowing, fertilisation, irrigation, and sowing) to the model.
 *
 * Management event schedules are read from the management input file by INPUT and
 * stored as parallel date/value vectors in this class. Each day, the model calls
 * applyManagementRegime() which scans those vectors and executes any action whose
 * date matches the current simulation day.
 *
 * @note Seed sowing is handled in the recruitment module
 *       (`RECRUITMENT::getIncomingSeedsBySowing()`); the `sowingDate` and
 *       `amountOfSownSeeds` members are populated here but consumed there.
 */
#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_plant/allometry.h"
#include "../module_soil/soil.h"
#include "../utils/utils.h"
#include <vector>
#include <iostream>

/**
 * @class MANAGEMENT
 * @brief Stores scheduled management event data and applies management actions
 *        for the current simulation day.
 */
class MANAGEMENT
{
public:
    MANAGEMENT();
    ~MANAGEMENT();

    /**
     * @brief Simulation day indices of scheduled mowing events (1-based, offset
     *        from the reference Julian start day).
     */
    std::vector<int> mowingDate;

    /**
     * @brief Target cutting heights for each mowing event (m), parallel to
     *        `mowingDate`. Converted to cm before use.
     */
    std::vector<double> mowingHeight;

    /**
     * @brief Simulation day indices of scheduled fertilisation events (1-based).
     */
    std::vector<int> fertilizationDate;

    /**
     * @brief Amount of mineral nitrogen fertiliser applied at each fertilisation
     *        event (g N m⁻²), parallel to `fertilizationDate`.
     */
    std::vector<double> fertilizerAmount;

    /**
     * @brief Simulation day indices of scheduled irrigation events (1-based).
     */
    std::vector<int> irrigationDate;

    /**
     * @brief Amount of water applied at each irrigation event (mm), parallel to
     *        `irrigationDate`.
     */
    std::vector<double> irrigationAmount;

    /**
     * @brief Simulation day indices of scheduled seed-sowing events (1-based).
     * @note Consumed by `RECRUITMENT::getIncomingSeedsBySowing()`.
     */
    std::vector<int> sowingDate;

    /**
     * @brief Number of seeds sown per PFT per sowing event. Consumed by `RECRUITMENT::getIncomingSeedsBySowing()`.
     */
    std::vector<std::vector<int>> amountOfSownSeeds;

    /**
     * @brief Applies all scheduled management actions for the current simulation day.
     */
    void applyManagementRegime(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL &soil);

    /**
     * @brief Resets per-PFT and community yield accumulators to zero.
     */
    void initializeYieldVariables(COMMUNITY &community, PARAMETER parameter);

    /**
     * @brief Checks whether today matches a scheduled mowing date and, if so,
     *        cuts all plant cohorts to the specified height.
     */
    void checkIfTodayAndDoMowing(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter);

    /**
     * @brief Cuts a single cohort to the target height, tracks harvested biomass
     *        as yield, and updates all affected plant attributes.
     */
    void cutPlantsAndTrackYieldAndUpdatePlantAttributes(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int cohortIndex, int pft, double heightToCutPlantsDownTo);

    /**
     * @brief Checks whether today matches a scheduled fertilisation date and,
     *        if so, adds mineral nitrogen to the topmost soil layer.
     */
    void checkIfTodayAndDoFertilization(UTILS utils, PARAMETER parameter, SOIL &soil);

    /**
     * @brief Checks whether today matches a scheduled irrigation date and,
     *        if so, adds water to the topmost soil layer.
     */
    void checkIfTodayAndDoIrrigation(UTILS utils, PARAMETER parameter, SOIL &soil);
};