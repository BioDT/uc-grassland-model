#include "management.h"

MANAGEMENT::MANAGEMENT() {};
MANAGEMENT::~MANAGEMENT() {};

/**
 * @brief Applies all scheduled land-management actions for the current simulation day.
 *
 * Executes the following management steps in order:
 * 1. initializeYieldVariables() — reset per-PFT and community yield accumulators.
 * 2. checkIfTodayAndDoMowing() — cut plant cohorts if today is a mowing event.
 * 3. checkIfTodayAndDoFertilization() — add mineral nitrogen to the topsoil layer
 *    if today is a fertilisation event.
 * 4. checkIfTodayAndDoIrrigation() — add water to the topsoil layer if today is
 *    an irrigation event.
 *
 * @note Seed sowing is handled within the recruitment module
 *       (see `RECRUITMENT::getIncomingSeedsBySowing()`).
 *
 * @param utils     Utility object for error handling and allometric calculations.
 * @param community Plant community; cohort biomass, height, LAI, and community-level
 *                  yield variables are updated in place.
 * @param allometry Allometric helper object used to recompute LAI after mowing.
 * @param parameter Read-only; provides `day`, PFT count, and C:N ratios.
 * @param soil      Soil state; mineral nitrogen and water pools updated by
 *                  fertilisation and irrigation events.
 */
void MANAGEMENT::applyManagementRegime(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL &soil)
{
    /* mowing events */
    initializeYieldVariables(community, parameter);
    checkIfTodayAndDoMowing(utils, community, allometry, parameter);

    /* fertilization events */
    checkIfTodayAndDoFertilization(utils, parameter, soil);

    /* irrigation events */
    checkIfTodayAndDoIrrigation(utils, parameter, soil);

    /* seed (re-)sowing */
    // NOTE: is captured within recruitment.cpp, see getIncomingSeedsBySowing()
}

/**
 * @brief Resets all yield accumulator variables to zero at the start of a time step.
 *
 * Zeroes the per-PFT green, brown, and total biomass yield vectors as well as the
 * corresponding community-level totals. Must be called before
 * checkIfTodayAndDoMowing() so that yield is only counted for the current day's
 * cutting event.
 *
 * @param community Plant community; `greenBiomassYieldPerPFT`, `brownBiomassYieldPerPFT`,
 *                  `biomassYieldPerPFT`, `greenBiomassYield`, `brownBiomassYield`, and
 *                  `biomassYield` are set to 0.
 * @param parameter Read-only; provides `pftCount` for the loop bounds.
 */
void MANAGEMENT::initializeYieldVariables(COMMUNITY &community, PARAMETER parameter)
{
    // initialize variables to track yield
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        community.greenBiomassYieldPerPFT[pft] = 0.0;
        community.brownBiomassYieldPerPFT[pft] = 0.0;
        community.biomassYieldPerPFT[pft] = 0.0;

        community.greenBiomassYield = 0.0;
        community.brownBiomassYield = 0.0;
        community.biomassYield = 0.0;
    }
}

/**
 * @brief Checks whether today is a scheduled mowing date and, if so, cuts all
 *        plant cohorts to the specified height.
 *
 * Scans `mowingDate` for a match with `parameter.day`. On a match, iterates over
 * all cohorts and calls cutPlantsAndTrackYieldAndUpdatePlantAttributes() for each.
 * After cutting, recomputes the community-level green and total LAI accumulators
 * (`greenleafAreaIndexOfPlantsInCommunity`, `totalLeafAreaIndexOfPlantsInCommunity`)
 * to reflect the new post-mow canopy state.
 *
 * @param utils     Utility object for error handling inside cutting sub-functions.
 * @param community Plant community; cohort and community LAI variables updated in place.
 * @param allometry Allometric helper used to recompute per-cohort LAI after cutting.
 * @param parameter Read-only; provides `day` for date matching.
 */
void MANAGEMENT::checkIfTodayAndDoMowing(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter)
{
    // scan through all mowing dates from the management file to check if today is a mowing event
    int index = 0;
    for (auto day : mowingDate)
    {
        if (parameter.day == day)
        {
            double heightToCutPlantsDownTo = 100.0 * mowingHeight.at(index); // convert m in cm
            for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
            {
                int pft = community.allPlants[cohortIndex]->pft;
                cutPlantsAndTrackYieldAndUpdatePlantAttributes(utils, community, allometry, parameter, cohortIndex, pft, heightToCutPlantsDownTo);
            }
            // update community variables in case of cutting
            community.greenleafAreaIndexOfPlantsInCommunity = 0;
            community.totalLeafAreaIndexOfPlantsInCommunity = 0;
            for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
            {
                // state variable updates for soil evaporation
                community.greenleafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->amount * community.allPlants[cohortindex]->laiGreen * community.allPlants[cohortindex]->coveredArea / SIMULATION_AREA;
                community.totalLeafAreaIndexOfPlantsInCommunity += community.allPlants[cohortindex]->lai * community.allPlants[cohortindex]->coveredArea * community.allPlants[cohortindex]->amount / SIMULATION_AREA;
            }
        }
        index++;
    }
}

/**
 * @brief Cuts a single plant cohort to the target height, records the harvested
 *        biomass as yield, and updates all affected plant attributes.
 *
 * Only acts if the cohort has living individuals (`amount > 0`) and its current
 * height exceeds `heightToCutPlantsDownTo`. The removed shoot biomass is
 * proportional to `(height - cutHeight) / height` and is split between green
 * and brown leaf fractions.
 *
 * The following cohort fields are updated:
 * - Shoot biomass pools (total, green, brown) and their derived carbon and
 *   nitrogen quantities.
 * - `height` is set to `heightToCutPlantsDownTo`.
 * - `laiGreen`, `laiBrown`, and `lai` are recomputed via allometry.
 *
 * Community-level yield accumulators (per-PFT and total) are incremented by the
 * removed biomass weighted by cohort `amount`.
 *
 * @param utils               Utility object for error handling in allometric calls.
 * @param community           Plant community; cohort biomass/height/LAI and community
 *                            yield variables updated in place.
 * @param allometry           Provides `laiFromShootBiomassAreaSla()` for LAI update.
 * @param parameter           Read-only; provides C/N ratios and specific leaf area.
 * @param cohortIndex         Index of the cohort to be cut.
 * @param pft                 Plant functional type index of the cohort.
 * @param heightToCutPlantsDownTo Target cutting height (cm).
 */
void MANAGEMENT::cutPlantsAndTrackYieldAndUpdatePlantAttributes(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int cohortIndex, int pft, double heightToCutPlantsDownTo)
{
    if (community.allPlants[cohortIndex]->amount > 0)
    {
        if (community.allPlants[cohortIndex]->height > heightToCutPlantsDownTo)
        {
            // cut the plants
            double heightProportionalityFactor = (community.allPlants[cohortIndex]->height - heightToCutPlantsDownTo) / community.allPlants[cohortIndex]->height;
            double cutGreenLeaves = heightProportionalityFactor * community.allPlants[cohortIndex]->shootBiomassGreenLeaves;
            double cutBrownLeaves = heightProportionalityFactor * community.allPlants[cohortIndex]->shootBiomassBrownLeaves;

            // track the yield
            community.greenBiomassYieldPerPFT[pft] += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
            community.brownBiomassYieldPerPFT[pft] += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
            community.biomassYieldPerPFT[pft] += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

            community.greenBiomassYield += (cutGreenLeaves * community.allPlants[cohortIndex]->amount);
            community.brownBiomassYield += (cutBrownLeaves * community.allPlants[cohortIndex]->amount);
            community.biomassYield += ((cutBrownLeaves + cutGreenLeaves) * community.allPlants[cohortIndex]->amount);

            // update attributes of plants
            community.allPlants[cohortIndex]->shootBiomass -= (cutGreenLeaves + cutBrownLeaves);
            community.allPlants[cohortIndex]->shootBiomassGreenLeaves -= cutGreenLeaves;
            community.allPlants[cohortIndex]->shootBiomassBrownLeaves -= cutBrownLeaves;

            community.allPlants[cohortIndex]->shootCarbonGreenLeaves = community.allPlants[cohortIndex]->shootBiomassGreenLeaves * CARBON_CONTENT_ODM;
            community.allPlants[cohortIndex]->shootCarbonBrownLeaves = community.allPlants[cohortIndex]->shootBiomassBrownLeaves * CARBON_CONTENT_ODM;
            community.allPlants[cohortIndex]->shootCarbon = community.allPlants[cohortIndex]->shootCarbonGreenLeaves + community.allPlants[cohortIndex]->shootCarbonBrownLeaves;

            community.allPlants[cohortIndex]->shootNitrogenGreenLeaves = community.allPlants[cohortIndex]->shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];
            community.allPlants[cohortIndex]->shootNitrogenBrownLeaves = community.allPlants[cohortIndex]->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
            community.allPlants[cohortIndex]->shootNitrogen = community.allPlants[cohortIndex]->shootNitrogenGreenLeaves + community.allPlants[cohortIndex]->shootNitrogenBrownLeaves;

            community.allPlants[cohortIndex]->height = heightToCutPlantsDownTo;
            community.allPlants[cohortIndex]->laiGreen = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassGreenLeaves,
                                                                                              community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
            community.allPlants[cohortIndex]->laiBrown = allometry.laiFromShootBiomassAreaSla(utils, community.allPlants[cohortIndex]->shootBiomassBrownLeaves,
                                                                                              community.allPlants[cohortIndex]->coveredArea, parameter.plantSpecificLeafArea[pft]);
            community.allPlants[cohortIndex]->lai = community.allPlants[cohortIndex]->laiGreen + community.allPlants[cohortIndex]->laiBrown;
        }
    }
}

/**
 * @brief Checks whether today is a scheduled fertilisation date and, if so,
 *        adds mineral nitrogen to the top soil layer.
 *
 * Resets `soil.addedMineralNitrogenToSoilByFertilization` to zero at the start
 * of each call so that the field always reflects only today's input. On a date
 * match, the specified amount of mineral fertiliser is added to
 * `nitrogenContent_soilMineralPoolPerSoilLayer[0]` (topmost soil layer).
 *
 * @note Organic fertiliser support is planned but currently not implemented
 *       (amount is hard-coded to 0).
 *
 * @param utils     Utility object (reserved for future error handling).
 * @param parameter Read-only; provides `day` for date matching.
 * @param soil      Soil state; `nitrogenContent_soilMineralPoolPerSoilLayer[0]` and
 *                  `addedMineralNitrogenToSoilByFertilization` updated in place.
 */
void MANAGEMENT::checkIfTodayAndDoFertilization(UTILS utils, PARAMETER parameter, SOIL &soil)
{
    soil.addedMineralNitrogenToSoilByFertilization = 0.0;

    // scan through all fertilization dates from the management file to check if today is an event
    int index = 0;
    for (auto day : fertilizationDate)
    {
        if (parameter.day == day)
        {
            // input in management file is in g N m⁻²; no conversion needed as simulation area is 100 x 100 cm²
            double amountOfMineralFertilizer = fertilizerAmount.at(index);
            soil.nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += amountOfMineralFertilizer;
            soil.addedMineralNitrogenToSoilByFertilization += amountOfMineralFertilizer;

            double amountOfOrganicFertilizer = 0.0; // TODO: add organic fertilizer option to management file
            soil.nitrogenContent_surfaceMetabolicLitterPool += amountOfOrganicFertilizer;
        }
        index++;
    }
}

/**
 * @brief Checks whether today is a scheduled irrigation date and, if so, adds
 *        water to the top soil layer.
 *
 * Resets `soil.addedWaterToSoilByIrrigation` to zero at the start of each call
 * so that the field always reflects only today's input. On a date match, the
 * specified water amount is added to
 * `waterContent_soilWaterPoolPerSoilLayer[0]` (topmost soil layer).
 *
 * @param utils     Utility object (reserved for future error handling).
 * @param parameter Read-only; provides `day` for date matching.
 * @param soil      Soil state; `waterContent_soilWaterPoolPerSoilLayer[0]` and
 *                  `addedWaterToSoilByIrrigation` updated in place.
 */
void MANAGEMENT::checkIfTodayAndDoIrrigation(UTILS utils, PARAMETER parameter, SOIL &soil)
{
    soil.addedWaterToSoilByIrrigation = 0.0;

    // scan through all irrigation dates from the management file to check if today is an event
    int index = 0;
    for (auto day : irrigationDate)
    {
        if (parameter.day == day)
        {
            double amountOfWater = irrigationAmount.at(index);
            soil.waterContent_soilWaterPoolPerSoilLayer.at(0) += amountOfWater;
            soil.addedWaterToSoilByIrrigation += amountOfWater;
        }
        index++;
    }
}
