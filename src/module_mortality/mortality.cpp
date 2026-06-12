#include "mortality.h"

MORTALITY::MORTALITY() {};
MORTALITY::~MORTALITY() {};

/**
 * @brief Orchestrates all plant mortality processes for a single simulation time step.
 *
 * Executes the following steps for all cohorts in the community:
 * 1. **Senescence and litter fall** (per cohort) — leaf browning, nitrogen
 *    relocation, brown-leaf litter transfer, and root senescence.
 * 2. **Covered-area update** — recomputes `coveredAreaOfAllPlants` and the
 *    per-height-layer version used by crowding mortality.
 * 3. **Crowding mortality** (optional, per cohort) — thins cohorts proportionally
 *    when total covered area exceeds `SIMULATION_AREA`.
 * 4. **Basic mortality** (per cohort) — applies intrinsic daily death probability.
 * 5. **Cohort cleanup** — removes cohorts with zero living individuals.
 * 6. **Cohort count update** — synchronises `totalNumberOfCohortsInCommunity`.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; provides mortality rates, flags, and PFT counts.
 * @param community   Plant community; cohort amounts, biomass, and litter pools updated.
 * @param allometry   Allometric helper used during litter-fall size updates.
 * @param growth      GROWTH object; provides temperature-response function for senescence.
 * @param interaction INTERACTION object; provides daytime temperature and water-limitation
 *                    factor used in leaf senescence.
 * @param soil        Soil state; litter pool C/N updated with all dying plant material.
 */
void MORTALITY::doPlantMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL &soil)
{

    for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
    {
        int pft = community.allPlants[cohortIndex]->pft;

        // 1. Leaf and root senescence and litter fall
        doSenescenceAndLitterFall(utils, parameter, community, allometry, growth, interaction, soil, cohortIndex, pft);
    }
    // 2. Update coveredAreaOfAllPlants to compare with simulationarea
    updateCoveredAreaOfAllPlants(parameter, community);
    for (int cohortIndex = 0; cohortIndex < community.totalNumberOfCohortsInCommunity; cohortIndex++)
    {
        int pft = community.allPlants[cohortIndex]->pft;

        // 2. Crowding mortality
        if (parameter.crowdingMortalityActivated)
        {
            if (parameter.stochasticSimulation)
            {
                community.randomNumberIndex++;
            }
            doPlantCrowding(parameter, utils, soil, community, cohortIndex, pft);
        }

        // 3. Basic mortality
        if (parameter.stochasticSimulation)
        {
            community.randomNumberIndex++;
        }
        doBasicMortality(utils, parameter, soil, community, cohortIndex, pft);
    }

    // 4. Delete cohorts if no more plants are alive
    community.checkPlantsAreAliveInCommunity(utils);

    // 5. Update number of cohorts in allPlants-vector
    community.totalNumberOfCohortsInCommunity = community.allPlants.size();
}

/**
 * @brief Applies leaf senescence, nitrogen relocation, brown-leaf litter fall,
 *        and root senescence for a single plant cohort.
 *
 * Delegates to the following sub-functions in order:
 * 1. doLeafSenescence() — moves a daily fraction of green leaf biomass to the
 *    brown leaf pool.
 * 2. doNitrogenRelocation() — recovers nitrogen released during browning back
 *    to the plant's nitrogen surplus.
 * 3. doLeafLitterFall() — transfers a fraction of brown leaves to the surface
 *    litter pool and updates plant geometry.
 * 4. doRootSenescenceAndLitterFall() — moves a daily fraction of root biomass
 *    to the soil root litter pool.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   PFT-specific parameters (leaf/root life spans, C:N ratios, etc.).
 * @param community   Plant community; cohort biomass and derived C/N fields updated.
 * @param allometry   Allometric helper for geometry updates after litter fall.
 * @param growth      Provides `calculateEffectOfAirTemperatureOnGPP()` for senescence.
 * @param interaction Provides daytime temperature and water-limitation factor.
 * @param soil        Soil state; litter pool C/N incremented with senescent material.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 */
void MORTALITY::doSenescenceAndLitterFall(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL &soil, int cohortIndex, int pft)
{
    /// Leaf senescence
    double browningLeafBiomass = doLeafSenescence(community, parameter, growth, interaction, cohortIndex, pft);
    doNitrogenRelocation(utils, parameter, community, browningLeafBiomass, cohortIndex, pft);

    // Litter fall of senescent leaves & transfer to surface litter pool
    doLeafLitterFall(utils, community, allometry, parameter, soil, cohortIndex, pft);

    // Root senescence
    doRootSenescenceAndLitterFall(utils, community, parameter, soil, cohortIndex, pft);
}

/**
 * @brief Computes the daily leaf browning rate and transfers biomass from the
 *        green to the brown leaf pool.
 *
 * The daily browning biomass is:
 * @f[
 *   \Delta B_{\text{brown}} = f_T \cdot f_W \cdot \frac{B_{\text{green}}}{\tau_{\text{leaf}}}
 * @f]
 * where @f$f_T@f$ is the daytime-temperature reduction factor (from
 * `GROWTH::calculateEffectOfAirTemperatureOnGPP()`), @f$f_W = 1 - \text{limitingFactorGppWater}@f$
 * is a water-stress factor, @f$B_{\text{green}}@f$ is current green shoot biomass, and
 * @f$\tau_{\text{leaf}}@f$ is the PFT-specific leaf life span (days).
 *
 * Updates `shootBiomassGreenLeaves`, `shootBiomassBrownLeaves`, and their
 * derived carbon and nitrogen quantities. Total shoot biomass is unchanged.
 *
 * @param community   Plant community; cohort leaf biomass and C/N fields updated.
 * @param parameter   PFT-specific parameters; provides `leafLifeSpan` and C/N ratios.
 * @param growth      Provides the temperature-response function for GPP / senescence.
 * @param interaction Provides `dayTimeAirTemperature` and `limitingFactorGppWater`.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 * @return Daily browning leaf biomass (g ODM plant⁻¹).
 */
double MORTALITY::doLeafSenescence(COMMUNITY &community, PARAMETER parameter, GROWTH growth, INTERACTION interaction, int cohortIndex, int pft)
{
    double effectOfDayTimeTemperature = growth.calculateEffectOfAirTemperatureOnGPP(interaction.dayTimeAirTemperature);
    double effectOfWaterLimitation = 1 - community.allPlants.at(cohortIndex)->limitingFactorGppWater;
    double browningLeafBiomass = effectOfDayTimeTemperature * effectOfWaterLimitation * (community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves / parameter.leafLifeSpan[pft]);

    community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves += browningLeafBiomass;
    community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves -= browningLeafBiomass;
    // community.allPlants.at(cohortIndex)->shootBiomass remains unchanged here

    community.allPlants.at(cohortIndex)->shootCarbonBrownLeaves = community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves * CARBON_CONTENT_ODM;
    community.allPlants.at(cohortIndex)->shootCarbonGreenLeaves = community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves * CARBON_CONTENT_ODM;

    community.allPlants.at(cohortIndex)->shootNitrogenBrownLeaves = community.allPlants.at(cohortIndex)->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
    community.allPlants.at(cohortIndex)->shootNitrogenGreenLeaves = community.allPlants.at(cohortIndex)->shootCarbonGreenLeaves / parameter.plantCNRatioGreenLeaves[pft];

    return (browningLeafBiomass);
}

/**
 * @brief Transfers a daily fraction of brown leaf biomass to the surface litter
 *        pool and updates plant geometry.
 *
 * On all days except the last day of the year (`parameter.day % 365 == 0`),
 * the fraction is `parameter.brownBiomassFractionFalling`. On the last day of
 * the year the fraction is forced to 1 (complete leaf drop) to reset the brown
 * leaf pool.
 *
 * The fallen biomass is deducted from `shootBiomassBrownLeaves` and transferred
 * to the surface brown litter pool via `SOIL::transferDyingPlantPartsToLitterPools()`.
 * Shoot biomass, carbon, and nitrogen fields are updated accordingly. Plant
 * height, width, covered area, and LAI are recomputed via updatePlantSize().
 *
 * @param utils       Utility object for allometric error handling.
 * @param community   Plant community; shoot biomass, C/N, and geometry fields updated.
 * @param allometry   Allometric helper for post-litter-fall size recalculation.
 * @param parameter   PFT-specific parameters; provides `brownBiomassFractionFalling`,
 *                    C/N ratios, `day`, and specific leaf area.
 * @param soil        Soil state; surface brown litter C/N pool incremented.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 */
void MORTALITY::doLeafLitterFall(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL &soil, int cohortIndex, int pft)
{
    if (community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves > 0.0)
    {
        double fractionLeavesFalling = parameter.brownBiomassFractionFalling;
        (parameter.day % 365 == 0) ? (fractionLeavesFalling = 1) : (fractionLeavesFalling = fractionLeavesFalling);

        if (fractionLeavesFalling > 0)
        {
            double fallingLeafBiomass = fractionLeavesFalling * community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;

            community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves -= fallingLeafBiomass;
            community.allPlants.at(cohortIndex)->shootBiomass = community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves + community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves;

            community.allPlants[cohortIndex]->shootCarbonBrownLeaves = community.allPlants[cohortIndex]->shootBiomassBrownLeaves * CARBON_CONTENT_ODM;
            community.allPlants[cohortIndex]->shootCarbon = community.allPlants[cohortIndex]->shootCarbonGreenLeaves + community.allPlants[cohortIndex]->shootCarbonBrownLeaves;

            community.allPlants[cohortIndex]->shootNitrogenBrownLeaves = community.allPlants[cohortIndex]->shootCarbonBrownLeaves / parameter.plantCNRatioBrownLeaves[pft];
            community.allPlants[cohortIndex]->shootNitrogen = community.allPlants[cohortIndex]->shootNitrogenGreenLeaves + community.allPlants[cohortIndex]->shootNitrogenBrownLeaves;

            // community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves remains unchanged here

            soil.transferDyingPlantPartsToLitterPools(utils, parameter, community.allPlants.at(cohortIndex)->amount, fallingLeafBiomass, "surface_brown", pft);
            // TODO: avoid change of form after cutting
            updatePlantSize(utils, community, allometry, parameter, fractionLeavesFalling, cohortIndex, pft);
        }
    }
}

/**
 * @brief Recomputes plant height, width, covered area, and LAI after litter fall.
 *
 * Two regimes are applied based on `fractionLeavesFalling`:
 * - **Full drop** (`fractionLeavesFalling == 1`): only height is recalculated
 *   from shoot biomass and current width; width and covered area are unchanged.
 * - **Partial drop** (`fractionLeavesFalling < 1`): width, height, and covered
 *   area are all recomputed from shoot biomass using allometric relations.
 *
 * In both cases, `laiGreen`, `laiBrown`, and `lai` are updated via
 * `ALLOMETRY::laiFromShootBiomassAreaSla()`.
 *
 * @param utils               Utility object for allometric error handling.
 * @param community           Plant community; geometry and LAI fields updated.
 * @param allometry           Provides allometric height, width, area, and LAI functions.
 * @param parameter           PFT-specific parameters (height-to-width ratio, shoot
 *                            correction factor, specific leaf area).
 * @param fractionLeavesFalling Fraction of brown leaves that fell (0–1); 1 triggers
 *                            height-only recalculation.
 * @param cohortIndex         Index of the target cohort.
 * @param pft                 Plant functional type index.
 */
void MORTALITY::updatePlantSize(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int fractionLeavesFalling, int cohortIndex, int pft)
{
    // calculation of width & coveredArea only if fractionFalling < 1
    // width shall not be updated when all brown biomass falls off at once, but only height
    if (fractionLeavesFalling == 1)
    {
        community.allPlants.at(cohortIndex)->height = allometry.heightFromShootBiomassWidthShootCorrection(utils, community.allPlants.at(cohortIndex)->shootBiomass, community.allPlants.at(cohortIndex)->width,
                                                                                                           parameter.plantShootCorrectionFactor[pft]);
    }
    else
    {
        community.allPlants.at(cohortIndex)->width = allometry.widthFromShootBiomassByRatioAndShootCorrection(utils, community.allPlants.at(cohortIndex)->shootBiomass, parameter.plantHeightToWidthRatio[pft],
                                                                                                              parameter.plantShootCorrectionFactor[pft]);

        community.allPlants.at(cohortIndex)->height = allometry.heightFromWidthByRatio(community.allPlants.at(cohortIndex)->width, parameter.plantHeightToWidthRatio[pft]);
        community.allPlants.at(cohortIndex)->coveredArea = allometry.areaFromWidth(community.allPlants.at(cohortIndex)->width);
    }

    community.allPlants.at(cohortIndex)->laiGreen =
        allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, community.allPlants.at(cohortIndex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
    community.allPlants.at(cohortIndex)->laiBrown =
        allometry.laiFromShootBiomassAreaSla(utils, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, community.allPlants.at(cohortIndex)->coveredArea, parameter.plantSpecificLeafArea[pft]);
    community.allPlants.at(cohortIndex)->lai = community.allPlants.at(cohortIndex)->laiBrown + community.allPlants.at(cohortIndex)->laiGreen;
}

/**
 * @brief Applies daily root senescence and transfers the dying root biomass to
 *        the soil root litter pool.
 *
 * The fraction of root biomass senescing each day equals `1 / rootLifeSpan[pft]`.
 * The dying biomass is passed to `SOIL::transferDyingPlantPartsToLitterPools()`
 * distributed across the cohort's rooting soil layers. Root biomass, carbon, and
 * nitrogen fields are updated accordingly.
 *
 * @param utils       Utility object for error handling.
 * @param community   Plant community; `rootBiomass`, `rootCarbon`, and `rootNitrogen`
 *                    updated for the target cohort.
 * @param parameter   PFT-specific parameters; provides `rootLifeSpan` and
 *                    `plantCNRatioRoots`.
 * @param soil        Soil state; soil root litter C/N pool incremented.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 */
void MORTALITY::doRootSenescenceAndLitterFall(UTILS utils, COMMUNITY &community, PARAMETER parameter, SOIL &soil, int cohortIndex, int pft)
{
    double dyingRootBiomass = community.allPlants.at(cohortIndex)->rootBiomass * (1.0 / parameter.rootLifeSpan[pft]);

    soil.transferDyingPlantPartsToLitterPools(utils, parameter, community.allPlants.at(cohortIndex)->amount, dyingRootBiomass, "soil_root", pft, community.allPlants.at(cohortIndex)->numberOfSoilLayersRooting);
    community.allPlants.at(cohortIndex)->rootBiomass -= dyingRootBiomass;
    community.allPlants.at(cohortIndex)->rootCarbon = community.allPlants.at(cohortIndex)->rootBiomass * CARBON_CONTENT_ODM;
    community.allPlants.at(cohortIndex)->rootNitrogen = community.allPlants.at(cohortIndex)->rootCarbon / parameter.plantCNRatioRoots[pft];
}

/**
 * @brief Recovers nitrogen from browning leaves and adds it to the plant's
 *        internal nitrogen surplus.
 *
 * When green leaves brown, their C/N ratio changes from `plantCNRatioGreenLeaves`
 * to `plantCNRatioBrownLeaves`. The difference represents nitrogen that is
 * retranslocated to the plant rather than lost to litter. This recovered nitrogen
 * is added to `nitrogenSurplus` and subtracted from `shootNitrogen`.
 *
 * @param utils              Utility object (reserved for future error handling).
 * @param parameter          PFT-specific parameters; provides green and brown leaf
 *                           C/N ratios.
 * @param community          Plant community; `nitrogenSurplus` and `shootNitrogen`
 *                           updated for the target cohort.
 * @param browningLeafBiomass Daily green-to-brown leaf biomass transfer (g ODM);
 *                           returned by doLeafSenescence().
 * @param cohortIndex        Index of the target cohort.
 * @param pft                Plant functional type index.
 */
void MORTALITY::doNitrogenRelocation(UTILS utils, PARAMETER parameter, COMMUNITY &community, double browningLeafBiomass, int cohortIndex, int pft)
{
    double carbonContentBrowningLeaves = browningLeafBiomass * CARBON_CONTENT_ODM;
    double previousNitrogenContentBrowningLeaves = carbonContentBrowningLeaves / parameter.plantCNRatioGreenLeaves[pft];
    double currentNitrogenContentBrowningLeaves = carbonContentBrowningLeaves / parameter.plantCNRatioBrownLeaves[pft];

    double relocatedNitrogen = previousNitrogenContentBrowningLeaves - currentNitrogenContentBrowningLeaves;
    community.allPlants[cohortIndex]->nitrogenSurplus += relocatedNitrogen;
    community.allPlants[cohortIndex]->shootNitrogen -= relocatedNitrogen;
}

/**
 * @brief Accumulates the total covered area of all living plant cohorts,
 *        weighted by the PFT-specific shoot overlap factor.
 *
 * Updates `community.coveredAreaOfAllPlants` (used as a simple community-wide
 * area sum) and, when `parameter.crowdingCalculationFromPlantTopLayer` is set,
 * also updates `community.coveredAreaOfAllPlantsPerHeightLayer` (a per-layer
 * version used by the layer-aware crowding mortality calculation).
 *
 * @param parameter Read-only; provides `crowdingCalculationFromPlantTopLayer` flag
 *                  and `plantShootOverlapFactors` per PFT.
 * @param community Plant community; `coveredAreaOfAllPlants` and
 *                  `coveredAreaOfAllPlantsPerHeightLayer` accumulated in place.
 */
void MORTALITY::updateCoveredAreaOfAllPlants(PARAMETER parameter, COMMUNITY &community)
{
    if (community.allPlants.size() > 0)
    {
        for (int cohortIndex = 0; cohortIndex < community.allPlants.size(); cohortIndex++)
        {
            int pft = community.allPlants[cohortIndex]->pft;
            community.coveredAreaOfAllPlants += community.allPlants[cohortIndex]->coveredArea * parameter.plantShootOverlapFactors[community.allPlants[cohortIndex]->pft] * community.allPlants[cohortIndex]->amount;
            if (parameter.crowdingCalculationFromPlantTopLayer)
            {
                /// search for height layer up to which the plant cohort is reaching to
                /// Note: floor is used because first height layer 0-1 cm has index 0
                int topHeightLayerIndexOfPlant = (int)std::floor((community.allPlants.at(cohortIndex)->height / HEIGHT_LAYER_WIDTH) + NUMERIC_TOLERANCE);
                for (int layerindex = 0; layerindex <= topHeightLayerIndexOfPlant; layerindex++)
                {
                    community.coveredAreaOfAllPlantsPerHeightLayer.at(layerindex) += community.allPlants[cohortIndex]->coveredArea * parameter.plantShootOverlapFactors[community.allPlants[cohortIndex]->pft] * community.allPlants[cohortIndex]->amount;
                }
            }
        }
    }
}

/**
 * @brief Applies crowding mortality to a single cohort when total covered area
 *        exceeds the simulation area.
 *
 * The number of plants to remove is proportional to the excess area:
 * @f[
 *   N_{\text{die}} = N \cdot \left(1 - \frac{A_{\text{sim}}}{A_{\text{total}}}
 *   \right)
 * @f]
 * When `parameter.crowdingCalculationFromPlantTopLayer` is set, the per-layer
 * covered area at the plant's topmost height layer is used instead of the
 * community-wide total.
 *
 * In stochastic mode, the fractional part of @f$N_{\text{die}}@f$ is resolved
 * probabilistically via a uniform random draw. In deterministic mode, the
 * fractional number is applied directly.
 *
 * Biomass of dying plants (shoot green, shoot brown, roots, recruitment) is
 * transferred to the corresponding litter pools before decrementing `amount`.
 *
 * @param parameter   Model parameters; provides simulation area, crowding flag,
 *                    and stochastic-mode flag.
 * @param utils       Utility object for error handling.
 * @param soil        Soil state; litter pools incremented with dying plant biomass.
 * @param community   Plant community; `amount` of the target cohort decremented.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 *
 * @cite Concept of crowding mortality derived from the forest model FORMIND
 *       (www.formind.org).
 */
void MORTALITY::doPlantCrowding(PARAMETER parameter, UTILS utils, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft)
{
    if (community.allPlants[cohortIndex]->amount > 0)
    {
        if (community.coveredAreaOfAllPlants > SIMULATION_AREA)
        {
            /* amount of plants that shall die due to crowding */
            double amountOfTooManyPlants;
            if (parameter.crowdingCalculationFromPlantTopLayer)
            {
                /// search for height layer up to which the plant cohort is reaching to
                /// Note: floor is used because first height layer 0-1 cm has index 0
                int topHeightLayerIndexOfPlant = (int)std::floor((community.allPlants.at(cohortIndex)->height / HEIGHT_LAYER_WIDTH) + NUMERIC_TOLERANCE);
                if (community.coveredAreaOfAllPlantsPerHeightLayer.at(topHeightLayerIndexOfPlant) > SIMULATION_AREA)
                {
                    amountOfTooManyPlants = community.allPlants[cohortIndex]->amount * (1.0 - (SIMULATION_AREA / community.coveredAreaOfAllPlantsPerHeightLayer.at(topHeightLayerIndexOfPlant)));
                }
                else
                {
                    amountOfTooManyPlants = 0.0;
                }
            }
            else
            {
                amountOfTooManyPlants = community.allPlants[cohortIndex]->amount * (1.0 - (SIMULATION_AREA / community.coveredAreaOfAllPlants));
            }
            /* avoid negative values from compuational precision */
            amountOfTooManyPlants = std::max(0.0, amountOfTooManyPlants);

            /* decide if either stochastic or deterministic mortality */
            if (parameter.stochasticSimulation)
            {
                std::uniform_real_distribution<> dis(0.0, 1.0);
                std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
                double randomNumber = dis(gen);

                double letAnotherPlantDy = amountOfTooManyPlants - int(amountOfTooManyPlants);
                amountOfTooManyPlants = int(amountOfTooManyPlants);
                if (randomNumber <= letAnotherPlantDy)
                {
                    amountOfTooManyPlants += 1.0;
                }
            }

            /* let plants die due to crowding */
            if (community.allPlants[cohortIndex]->amount - amountOfTooManyPlants >= 0)
            {
                soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, "surface_green", pft);
                soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, "surface_brown", pft);
                soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->rootBiomass, "soil_root", pft, community.allPlants.at(cohortIndex)->numberOfSoilLayersRooting);
                soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfTooManyPlants, community.allPlants.at(cohortIndex)->recruitmentBiomass, "soil_seed", pft);
                community.allPlants[cohortIndex]->amount -= amountOfTooManyPlants;
            }
            else
            {
                utils.handleError("Error (mortality): more plants shall die than are available in the cohort.");
            }
        }
    }
    else
    {
        utils.handleError("Error (mortality): no more plants available in the cohort to die.");
    }
}

/**
 * @brief Applies intrinsic (background) mortality to a single plant cohort.
 *
 * Retrieves the daily mortality probability from getPlantMortalityProbability()
 * and computes the number of plants to remove:
 * - **Stochastic mode**: draws from a binomial distribution
 *   `Binomial(amount, mortalityProbability)`.
 * - **Deterministic mode**: multiplies amount by probability; if the surviving
 *   count falls below `parameter.tresholdCohortDeathDeterministic`, the entire
 *   cohort dies.
 *
 * Biomass of dying plants is transferred to the corresponding litter pools before
 * decrementing `amount`.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; provides mortality probabilities, deterministic
 *                    threshold, and stochastic-mode flag.
 * @param soil        Soil state; litter pools incremented with dying plant biomass.
 * @param community   Plant community; `amount` of the target cohort decremented.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 */
void MORTALITY::doBasicMortality(UTILS utils, PARAMETER parameter, SOIL &soil, COMMUNITY &community, int cohortIndex, int pft)
{
    double mortalityProbability = getPlantMortalityProbability(parameter, community, cohortIndex, pft);
    double amountOfPlantsToDie;

    if (community.allPlants[cohortIndex]->amount > 0)
    {
        if (parameter.stochasticSimulation)
        {
            /* stochastic basic mortality */
            /* draw number of dying plants from binomial distribution */
            std::binomial_distribution<> d(static_cast<int>(community.allPlants[cohortIndex]->amount), mortalityProbability);
            std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
            amountOfPlantsToDie = d(gen);
        }
        else
        {
            /* deterministic basic mortality */
            amountOfPlantsToDie = community.allPlants[cohortIndex]->amount * mortalityProbability;
            /* if cohort falls below treshold, dieout of all plants */
            if ((community.allPlants[cohortIndex]->amount - amountOfPlantsToDie) < parameter.tresholdCohortDeathDeterministic)
            {
                amountOfPlantsToDie = community.allPlants[cohortIndex]->amount;
            }
        }
        soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfPlantsToDie, community.allPlants.at(cohortIndex)->shootBiomassGreenLeaves, "surface_green", pft);
        soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfPlantsToDie, community.allPlants.at(cohortIndex)->shootBiomassBrownLeaves, "surface_brown", pft);
        soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfPlantsToDie, community.allPlants.at(cohortIndex)->rootBiomass, "soil_root", pft, community.allPlants.at(cohortIndex)->numberOfSoilLayersRooting);
        soil.transferDyingPlantPartsToLitterPools(utils, parameter, amountOfPlantsToDie, community.allPlants.at(cohortIndex)->recruitmentBiomass, "soil_seed", pft);
        community.allPlants[cohortIndex]->amount -= amountOfPlantsToDie;
    }
}

/**
 * @brief Returns the daily mortality probability for a given plant cohort.
 *
 * Selects the appropriate rate based on cohort age relative to `maturityAges[pft]`:
 * - **Adult** (`age >= maturityAges[pft]`):
 *   - For annual PFTs (`plantLifeSpan == "annual"`), returns 1.0 (certain death)
 *     once the plant has lived more than 365 days.
 *   - Otherwise returns `plantMortalityProbability[pft]`.
 * - **Seedling** (`age < maturityAges[pft]`): returns
 *   `seedlingMortalityProbability[pft]`.
 *
 * @param parameter   PFT-specific parameters; provides maturity age, life-span
 *                    string, and mortality probability values.
 * @param community   Read-only; provides cohort `age` via `allPlants`.
 * @param cohortIndex Index of the target cohort.
 * @param pft         Plant functional type index.
 * @return Daily mortality probability in [0, 1].
 */
double MORTALITY::getPlantMortalityProbability(PARAMETER parameter, COMMUNITY community, int cohortIndex, int pft)
{
    if (community.allPlants[cohortIndex]->age >= parameter.maturityAges[pft])
    {
        if (parameter.plantLifeSpan[pft] == "annual" && community.allPlants[cohortIndex]->age > 365)
        {
            return (1.0);
        }
        else
        {
            return (parameter.plantMortalityProbability[pft]);
        }
    }
    else
    {
        return (parameter.seedlingMortalityProbability[pft]);
    }
}
