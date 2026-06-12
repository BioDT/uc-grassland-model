#include "soil.h"

SOIL::SOIL() {};
SOIL::~SOIL() {};

/**
 * @brief Converts dying plant biomass to C/N fluxes and adds them to the
 *        corresponding litter pool.
 *
 * For the **internal soil module** and self-coupling set mode, increments
 * the relevant internal pool (surface green/brown litter, soil root litter,
 * or soil seed litter). For the **external BODIUM module**, routes surface
 * material to `couplingInterface_surfaceLitterFlux*` and root material to
 * `couplingInterface_rootLitterFlux*` distributed equally over the rooting
 * layers.
 *
 * @param utils                  Utility object for error handling.
 * @param parameter              PFT-specific C/N ratios and soil-module flags.
 * @param number                 Number of dying individuals (or seed count).
 * @param biomass                Per-individual biomass (g ODM).
 * @param typeOfMaterial         Litter type: `"surface_green"`, `"surface_brown"`,
 *                               `"soil_root"`, or `"soil_seed"`.
 * @param pft                    Plant functional type index.
 * @param numberOfTargetSoilLayers Number of rooting soil layers for root litter
 *                               distribution (required only when
 *                               `useExternalSoilModule_BODIUM` is active;
 *                               default = 0 for surface material).
 */
void SOIL::transferDyingPlantPartsToLitterPools(UTILS utils, PARAMETER parameter, double number, double biomass, std::string typeOfMaterial, int pft, int numberOfTargetSoilLayers)
{

    double carbonFlux = number * (biomass * CARBON_CONTENT_ODM);

    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        if (typeOfMaterial == "surface_green")
        {
            carbonContent_surfaceGreenLitterPool += carbonFlux;
            nitrogenContent_surfaceGreenLitterPool += (carbonFlux / parameter.plantCNRatioGreenLeaves[pft]);
        }
        else if (typeOfMaterial == "surface_brown")
        {
            carbonContent_surfaceBrownLitterPool += carbonFlux;
            nitrogenContent_surfaceBrownLitterPool += (carbonFlux / parameter.plantCNRatioBrownLeaves[pft]);
        }
        else if (typeOfMaterial == "soil_root")
        {
            carbonContent_soilRootLitterPool += carbonFlux;
            nitrogenContent_soilRootLitterPool += (carbonFlux / parameter.plantCNRatioRoots[pft]);
        }
        else if (typeOfMaterial == "soil_seed")
        {
            carbonContent_soilSeedLitterPool += carbonFlux;
            nitrogenContent_soilSeedLitterPool += (carbonFlux / parameter.plantCNRatioSeeds[pft]);
        }
        else
        {
            utils.handleError("Wrong type of material of litter.");
        }
    }
    else if (parameter.useExternalSoilModule_BODIUM)
    {
        if (typeOfMaterial == "surface_green")
        {
            couplingInterface_surfaceLitterFluxCarbon += carbonFlux;
            couplingInterface_surfaceLitterFluxNitrogen += (carbonFlux / parameter.plantCNRatioGreenLeaves[pft]);
        }
        else if (typeOfMaterial == "surface_brown")
        {
            couplingInterface_surfaceLitterFluxCarbon += carbonFlux;
            couplingInterface_surfaceLitterFluxNitrogen += (carbonFlux / parameter.plantCNRatioBrownLeaves[pft]);
        }
        else if (typeOfMaterial == "soil_seed")
        {
            couplingInterface_surfaceLitterFluxCarbon += carbonFlux;
            couplingInterface_surfaceLitterFluxNitrogen += (carbonFlux / parameter.plantCNRatioSeeds[pft]);
        }
        else if (typeOfMaterial == "soil_root")
        {
            if (numberOfTargetSoilLayers == 0)
            {
                utils.handleError("External soil module require number of target layers for root litter.");
            }
            // distribute dying root flux equally among rooting layers
            for (int soilLayer = 0; soilLayer < numberOfTargetSoilLayers; soilLayer++)
            {
                couplingInterface_rootLitterFluxCarbon[soilLayer] +=
                    carbonFlux / numberOfTargetSoilLayers;
                couplingInterface_rootLitterFluxNitrogen[soilLayer] += (carbonFlux / parameter.plantCNRatioRoots[pft]) / numberOfTargetSoilLayers;
            }
        }
        else
        {
            utils.handleError("Wrong type of material of litter.");
        }
    }
}

/**
 * @brief Orchestrates all soil resource dynamics for a single simulation time step.
 *
 * Always runs the soil water pipeline (calculateSoilWaterDynamics()).
 * Additionally runs the soil C/N decomposition pipeline
 * (calculateSoilCarbonNitrogenDynamics()) when the internal soil module or
 * the self-coupling set mode is active.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; soil-module flags control which sub-modules run.
 * @param weather     Daily weather data (precipitation, temperature, PET).
 * @param community   Plant community; LAI accumulators read for soil evaporation.
 * @param interaction Interaction state; soil temperature and LAI read.
 *
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::calculateSoilResourceDynamics(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community, INTERACTION interaction)
{
    /* soil water dynamics */
    calculateSoilWaterDynamics(utils, parameter, weather, interaction, community);

    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        /* soil carbon & nitrogen dynamics */
        calculateSoilCarbonNitrogenDynamics(utils, parameter, interaction, weather);
    }
}

// ##################################################################################################
// soil water dynamics
// ##################################################################################################
/**
 * @brief Executes the full daily soil water pipeline.
 *
 * Steps applied in order (some conditional on snow/module flags):
 * 1. Add precipitation and irrigation to daily water input.
 * 2. accumulateSnowWhenFreezing() — intercept input as snow at T ≤ 0 °C.
 * 3. meltingOfSnow() — melt a fraction of the snowpack at T > 0 °C.
 * 4. evaporationOfSnow() — sublimate snow and reduce remaining PET.
 * 5. When no solid snow cover:
 *    - interceptionByVegetation() — canopy interception.
 *    - runOffAtSurface() — surface run-off.
 * 6. If BODIUM coupling: export surface water input and PET to coupling interface.
 * 7. If internal module / self-coupling set:
 *    - soilWaterPercolation() — layer-by-layer downward water flow.
 *    - leachingOfNitrogenCoupledToWaterPercolation() — N leaching with drainage.
 *    - evaporationFromTopSoilLayer() — bare-soil evaporation from top layers.
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Model parameters; soil-module flags, layer properties.
 * @param weather     Provides precipitation, PET, and air temperature.
 * @param interaction Provides community LAI for interception calculation.
 * @param community   Provides LAI accumulators for soil evaporation fraction.
 * @cite Function and code adapted from the CENTURY 4.0 soil model, evaporation function adapted from BOWET model.
 */
void SOIL::calculateSoilWaterDynamics(UTILS utils, PARAMETER parameter, WEATHER weather, INTERACTION interaction, COMMUNITY &community)
{
    // water input to soil [mm/day]
    double dailyWaterInputToSoil = weather.precipitation.at(parameter.day - 1) + addedWaterToSoilByIrrigation;

    // snow accumulation [mm/day]
    dailyWaterInputToSoil = accumulateSnowWhenFreezing(utils, parameter, weather, dailyWaterInputToSoil);

    // melting of solid snow and adding rain to liquid snowpack
    dailyWaterInputToSoil = meltingOfSnow(utils, parameter, weather, dailyWaterInputToSoil);

    // sublimation of solidSnowContent and liquidSnowContent
    double remainingDailyPET = evaporationOfSnow(utils, weather, parameter);

    // as long as there is a solid snow pack, vegetation and soil is covered
    // preventing interception by vegetation, surface runoff and bare soil evaporation
    if (solidSnowContent == 0.0)
    {
        // interception [mm/day]
        dailyWaterInputToSoil = interceptionByVegetation(utils, interaction, dailyWaterInputToSoil, remainingDailyPET);

        // surface runoff [mm/day]
        dailyWaterInputToSoil = runOffAtSurface(utils, parameter, dailyWaterInputToSoil);
    }

    if (parameter.useExternalSoilModule_BODIUM)
    {
        couplingInterface_soilWaterSurfaceInput = dailyWaterInputToSoil;
        couplingInterface_potentialEvapotranspirationReducedByInterceptionSublimation = remainingDailyPET - interception;
    }

    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        // vertical stream of dailyWaterInputToSoil through soil layers
        soilWaterPercolation(utils, parameter, dailyWaterInputToSoil);

        // leaching of nitrogen downwards through the soil layers coupled to water fluxes
        leachingOfNitrogenCoupledToWaterPercolation(utils, parameter);

        //  evaporation from top soil layer [mm/day]
        evaporationFromTopSoilLayer(utils, parameter, community, remainingDailyPET);
    }
}

/**
 * @brief Accumulates precipitation as solid snow when air temperature is at
 *        or below freezing.
 *
 * If the full-day mean air temperature is ≤ 0 °C, all `waterInputToSoil`
 * is added to `solidSnowContent` and the function returns 0 (no liquid water
 * percolates into the soil). Otherwise the input is returned unchanged.
 *
 * @param utils            Utility object (reserved for future error handling).
 * @param parameter        Provides `day` for indexing weather vectors.
 * @param weather          Daily full-day air temperature vector.
 * @param waterInputToSoil Liquid water available before snow accumulation (mm).
 * @return Liquid water remaining after potential snow accumulation (mm);
 *         0 when all input is frozen.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
double SOIL::accumulateSnowWhenFreezing(UTILS utils, PARAMETER parameter, WEATHER weather, double waterInputToSoil)
{
    //// adding rain to solid snowpack if air temperature is below 0 degrees
    // function returns then a value of 0 (with no further water able to percolate into soil)

    double fullDayAverageAirTempertaure = weather.fullDayAirTemperature.at(parameter.day - 1);

    if (fullDayAverageAirTempertaure <= 0.0)
    {
        solidSnowContent += waterInputToSoil;
        return (0.0); // water input is accumulated as snow and no further water can percolate into soil
    }
    else
    {
        return waterInputToSoil;
    }
}

/**
 * @brief Melts a fraction of the solid snowpack at above-freezing temperatures
 *        and manages liquid snow storage.
 *
 * At T ≥ 0 °C
 * - Melts 0.2 % of `solidSnowContent` per degree above 0 °C per day.
 * - While a solid snowpack remains and T > 0 °C, incoming liquid water is
 *   absorbed by the snowpack (stored as `liquidSnowContent`) rather than
 *   percolating into the soil.
 * - The snowpack can retain at most 5 % of `solidSnowContent` as liquid;
 *   excess liquid is released as additional soil-water input.
 *
 * @param utils            Utility object for error handling.
 * @param parameter        Provides `day` for indexing weather vectors.
 * @param weather          Daily full-day air temperature vector.
 * @param waterInputToSoil Liquid water entering the top of the snowpack (mm).
 * @return Total liquid water available for soil percolation after snowmelt (mm).
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
double SOIL::meltingOfSnow(UTILS utils, PARAMETER parameter, WEATHER weather, double waterInputToSoil)
{
    // if air temperature is <= 0, precipitation has been accumulated as solid snow (see accumulateSnowWhenFreezing with the result of waterInputToSoil = 0)
    // if air temperature >= 0, then a fraction of the solid snow can melt (see accumulateSnowWhenFreezing with the result of waterInputToSoil >= 0)
    // the snowpack is only able to store waterInputToSoil at air temperature > 0
    // at air temperatures = 0, waterInputToSoil would be accumulated as snow (while fraction of solid snow can melt to liquid snow parts)
    // if there is no snowpack, waterInputToSoil is returned unchanged (and able to percolate into soil)
    // if all of the snowpack has melted, solidSnowContent = 0 while liquidSnowContent > 0 percolating together with waterInputToSoil into soil

    double fullDayAverageAirTemperature = weather.fullDayAirTemperature.at(parameter.day - 1);
    double meltedSnowAsAddedWaterInputToSoil = 0.0;

    if (fullDayAverageAirTemperature >= 0.0)
    {
        if (solidSnowContent > 0.0)
        {
            // 0.2% of snow (in mm) per temperature degree increase above 0 is able to melt per day
            double meltingSnow = 0.002 * fullDayAverageAirTemperature;
            meltingSnow = std::min(meltingSnow, solidSnowContent);
            solidSnowContent -= meltingSnow;
            liquidSnowContent += meltingSnow;
        }

        // if air temperature is above 0 (not at 0 degrees) and not all the solid snow has been melted
        // then the snowpack is able to store the waterInputToSoil (preventing its percolation into soil)
        if (fullDayAverageAirTemperature > 0.0 && solidSnowContent > 0.0)
        {
            liquidSnowContent += waterInputToSoil;
            waterInputToSoil = 0;
        }

        // the snowpack is only able to store 5% as liquid
        // the remaining melted snow is added to waterInputToSoil percolating into soil
        double maximumLiquidSnowContent = 0.05 * solidSnowContent;
        if (liquidSnowContent > maximumLiquidSnowContent)
        {
            meltedSnowAsAddedWaterInputToSoil = liquidSnowContent - maximumLiquidSnowContent;
            if (meltedSnowAsAddedWaterInputToSoil <= liquidSnowContent)
            {
                liquidSnowContent -= meltedSnowAsAddedWaterInputToSoil;
            }
            else
            {
                utils.handleError("Liquid snow content is below 0 through melting.");
                liquidSnowContent = 0.0;
            }
        }
    }

    return (waterInputToSoil + meltedSnowAsAddedWaterInputToSoil);
}

/**
 * @brief Sublimates snow from the solid and liquid snowpack using potential
 *        evapotranspiration (PET) and returns the remaining PET.
 *
 * When a solid snowpack is present, 87 % of daily PET is used for snow
 * sublimation (proportionally from solid and liquid fractions). The
 * sublimated amount is added to the `evaporation` accumulator and deducted
 * from the daily PET. If there is no solid snowpack, daily PET is returned
 * unchanged for subsequent canopy and soil evaporation use.
 *
 * @param utils     Utility object for error handling.
 * @param weather   Provides daily potential evapotranspiration.
 * @param parameter Provides `day` for indexing weather vectors.
 * @return Remaining PET after snow sublimation (mm d⁻¹).
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
double SOIL::evaporationOfSnow(UTILS utils, WEATHER weather, PARAMETER parameter)
{
    // sublimation: reduces solidSnowContent and liquidSnowContent and increases evaporation
    // usually 13% of dailyPET remain
    // solid and liquid snow pack could be reduced to 0 in case of high evaporation
    // if there is no solid snow pack, remainingDailyPET = dailyPET

    // potential evapotranspiration [mm/day]
    double dailyPET = weather.potEvapoTranspiration.at(parameter.day - 1);

    if (solidSnowContent > 0.0)
    {
        double sumOfSnowPack = solidSnowContent + liquidSnowContent;

        // if there is a snowpack, this fraction of PET is reserved for snow evaporation, leaving 13% of remaining PET for plant transpiration and soil evaporation
        double snowEvaporation = dailyPET * 0.87;
        snowEvaporation = std::min(snowEvaporation, sumOfSnowPack);

        // proportional reduction of solid and liquid snow pack for evaporation
        double reductionSolidSnow = solidSnowContent / sumOfSnowPack;
        double reductionLiquidSnow = liquidSnowContent / sumOfSnowPack;

        solidSnowContent -= (snowEvaporation * reductionSolidSnow);
        liquidSnowContent -= (snowEvaporation * reductionLiquidSnow);

        if (solidSnowContent < 0.0)
        {
            utils.handleError("Solid snow pack is below 0 through evaporation / sublimation.");
            solidSnowContent = 0.0;
        }
        if (liquidSnowContent < 0.0)
        {
            utils.handleError("Liquid snow pack is below 0 through evaporation / sublimation.");
            liquidSnowContent = 0.0;
        }

        // adding snow evaporation to total evaporation flux and reducing dailyPET
        evaporation += snowEvaporation;
        dailyPET -= snowEvaporation;
        dailyPET = std::max(dailyPET, 0.0);
    }

    return dailyPET;
}

/**
 * @brief Calculates canopy interception of precipitation by the plant community.
 *
 * Maximum interception is the lesser of incoming water and
 * 0.2 × community LAI. The actual interception is scaled by canopy closure
 * (1 − e^⁻^LAI_ext) and capped at `remainingDailyPET`. Reduces
 * `dailyWaterInputToSoil` accordingly and updates `interception`.
 *
 * @param utils                 Utility object (reserved for future error handling).
 * @param interaction           Community LAI (index 0) and light-extinction-weighted
 *                              LAI used for canopy closure.
 * @param dailyWaterInputToSoil Water input before interception (mm).
 * @param remainingDailyPET     Remaining PET available for interception (mm).
 * @return Water input after subtracting interception (mm).
 *
 * @cite Approach adapted from the FORMIND forest model (www.formind.org)
 */
double SOIL::interceptionByVegetation(UTILS utils, INTERACTION interaction, double dailyWaterInputToSoil, double remainingDailyPET)
{
    double maximumInterception = std::min(dailyWaterInputToSoil, 0.2 * interaction.LAI.at(0)); // TODO: 0.2 is a interception constant
    double canopyClosure = 1.0 - exp(-interaction.LAIwithLightExtinction.at(0));
    interception = maximumInterception * canopyClosure;

    interception = std::min(remainingDailyPET, interception);
    dailyWaterInputToSoil -= interception;
    return (dailyWaterInputToSoil);
}

/**
 * @brief Calculates surface run-off from incoming water using a linear threshold model.
 *
 * Run-off is computed as: surfaceRunOff = max(0, 0.41 × input − 28.7/30).
 * Skipped entirely when `parameter.disableRunoff` is set. Updates `surfaceRunOff`
 * and returns the remaining water available for soil infiltration.
 *
 * @param utils                 Utility object (reserved for future error handling).
 * @param parameter             Provides the `disableRunoff` flag.
 * @param dailyWaterInputToSoil Water input after interception (mm).
 * @return Water remaining for soil infiltration after run-off (mm).
 *
 * @cite Approach adapted from the CENTURY 4.0 model.
 */
double SOIL::runOffAtSurface(UTILS utils, PARAMETER parameter, double dailyWaterInputToSoil)
{
    if (!parameter.disableRunoff)
    {
        surfaceRunOff = 0.41 * dailyWaterInputToSoil - (28.7 / 30.0);
        surfaceRunOff = std::max(surfaceRunOff, 0.0);
        dailyWaterInputToSoil -= surfaceRunOff;
    }

    return (dailyWaterInputToSoil);
}

/**
 * @brief Simulates vertical water percolation through soil layers and an
 *        underground groundwater storage layer.
 *
 * Starting at the surface, adds the incoming water to each layer in turn.
 * If the layer water content exceeds field capacity, excess water percolates
 * to the next layer using a non-linear conductance equation:
 * @f[
 *   q = \frac{\lambda (\theta - \text{FC})^2}{1 + \lambda (\theta - \text{FC})}
 * @f]
 * where @f$\lambda = 0.01 \cdot K_{\text{sat}} / (\phi - \text{FC})@f$.
 * Below the lowest soil layer, excess water enters a groundwater storage pool.
 * Downward fluxes per layer are stored in `soilWaterFluxDownwardsOutOfSoilLayer`.
 *
 * @param utils            Utility object for error handling.
 * @param parameter        Provides layer count, field capacity, porosity, and
 *                         saturated hydraulic conductivity per layer.
 * @param waterInputToSoil Total liquid water entering the top layer (mm).
 *
 * @cite Approach adapted from the CENTURY 4.0 model.
 */
void SOIL::soilWaterPercolation(UTILS utils, PARAMETER parameter, double waterInputToSoil)
{
    double addWaterExcessToNextSoilLayer = waterInputToSoil;
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        double percolationToNextSoilLayer = 0.0;

        waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) += addWaterExcessToNextSoilLayer;
        double waterContentBeyondSaturatedCapacity = waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) - fieldCapacity.at(soilLayer);

        if (waterContentBeyondSaturatedCapacity > 0.0)
        {
            double lambda = 0.01 * saturatedHydraulicConductivity.at(soilLayer) / (porosity.at(soilLayer) - fieldCapacity.at(soilLayer));
            percolationToNextSoilLayer = (lambda * std::pow(waterContentBeyondSaturatedCapacity, 2.0)) / (1.0 + lambda * waterContentBeyondSaturatedCapacity);
        }

        waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) -= percolationToNextSoilLayer;
        soilWaterFluxDownwardsOutOfSoilLayer.at(soilLayer) = percolationToNextSoilLayer;
        addWaterExcessToNextSoilLayer = percolationToNextSoilLayer;
    }

    // underground storage of water
    double stormflow = 0, baseflow = 0;                                                                                   // TODO: add values as parameter
    waterContent_soilWaterPoolPerSoilLayer.at(parameter.numberOfSoilLayers) += addWaterExcessToNextSoilLayer - stormflow; // runoff into underground storage
    baseflow = waterContent_soilWaterPoolPerSoilLayer.at(parameter.numberOfSoilLayers) * 0.0;                             // TODO: add 0 as parameter
    waterContent_soilWaterPoolPerSoilLayer.at(parameter.numberOfSoilLayers) -= baseflow;
    soilRunOff = (stormflow + baseflow);
}

/**
 * @brief Leaches mineral nitrogen downward through soil layers coupled to
 *        the previous day's downward water fluxes.
 *
 * For each layer with a positive downward water flux and positive mineral
 * nitrogen, computes the leaching fraction using a soil-texture factor
 * (0.6 + 0.4 × sand fraction) and a non-linear coupling to water flux
 * (clamped to [0, 1]). Leached N is deducted from the source layer and
 * either added to the next layer or accumulated in `nitrogenContent_leachedFromSoil`
 * when it exits the bottom of the soil column.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Provides layer count, sand content, and soil layer widths.
 *
 * @cite Approach adapted from the CENTURY 4.0 model.
 */
void SOIL::leachingOfNitrogenCoupledToWaterPercolation(UTILS utils, PARAMETER parameter)
{
    double streamOfNitrogen = 0.0; // TODO: no calculation here!

    double amountOfLeachingNitrogen = 0.0;
    int indexOfNextSoilLayer;

    double soilTypeFactor = (0.6 + 0.4 * sandContent) * 0.95;

    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        amountOfLeachingNitrogen = 0.0;
        indexOfNextSoilLayer = soilLayer + 1;

        // if there was a saturated water flow out of soil layer i and the mineral nitrogen content > 0
        if ((soilWaterFluxDownwardsOutOfSoilLayer.at(soilLayer) > 0.0) && (nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) > 0.0))
        {
            double coupledSoilWaterFlux = std::min(1.0 - (2.5 - (soilWaterFluxDownwardsOutOfSoilLayer.at(soilLayer) / 10.0)) / 2.5, 1.0);
            coupledSoilWaterFlux = std::max(coupledSoilWaterFlux, 0.0);

            amountOfLeachingNitrogen = soilTypeFactor * nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) * coupledSoilWaterFlux;
            amountOfLeachingNitrogen = std::min(amountOfLeachingNitrogen, nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer));

            // subtract from soil layer
            nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) -= amountOfLeachingNitrogen;
            if (nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) + TOLERANCE < 0.0)
            {
                nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) = 0.0;
                // utils.handleError("Plants do not have access to soil nitrogen or soil nitrogen pool is negative!");
            }

            // add to soil layer 'indexOfNextSoilLayer'
            if (indexOfNextSoilLayer == parameter.numberOfSoilLayers)
            {
                nitrogenContent_leachedFromSoil += amountOfLeachingNitrogen;
            }
            else if (indexOfNextSoilLayer < parameter.numberOfSoilLayers)
            {
                nitrogenContent_soilMineralPoolPerSoilLayer.at(indexOfNextSoilLayer) += amountOfLeachingNitrogen;
                if (nitrogenContent_soilMineralPoolPerSoilLayer.at(indexOfNextSoilLayer) + TOLERANCE < 0.0)
                {
                    // utils.handleError("Plants do not have access to soil nitrogen or soil nitrogen pool is negative!");
                    nitrogenContent_soilMineralPoolPerSoilLayer.at(indexOfNextSoilLayer) = 0.0;
                }
            }
        }
    }

    nitrogenVolatilization += streamOfNitrogen;
}

/**
 * @brief Estimates the fraction of the soil surface exposed to direct evaporation.
 *
 * Returns `1 − canopyClosure` where `canopyClosure` is the ratio of green
 * to total community LAI (clamped to [0, 1]). A fully closed green canopy
 * yields 0 (no bare-soil evaporation); a leafless community yields 1.
 *
 * @param utils     Utility object; issues a warning if canopy closure exceeds 1.
 * @param community Read-only; provides `greenleafAreaIndexOfPlantsInCommunity`
 *                  and `totalLeafAreaIndexOfPlantsInCommunity`.
 * @return Open-canopy fraction (dimensionless, 0–1).
 * @cite Approach adapted from BOWET model.
 */
double SOIL::estimateSoilSurfaceAffectedBySoilEvaporation(UTILS utils, COMMUNITY community)
{
    double canopyClosure = 0.0;
    if (community.totalLeafAreaIndexOfPlantsInCommunity > 0)
    {
        canopyClosure = community.greenleafAreaIndexOfPlantsInCommunity / community.totalLeafAreaIndexOfPlantsInCommunity;
    }

    if (canopyClosure > 1.0)
    {
        canopyClosure = 1.0;
        utils.handleWarning("Canopy closure is above 1.0, setting to 1.0");
    }

    return (1.0 - canopyClosure);
}

/**
 * @brief Computes the cumulative soil depth affected by evaporation.
 *
 * Sums the widths of the top `numberOfTopSoilLayersAffectedByEvaporation`
 * soil layers from `parameter.soilLayerWidth`.
 *
 * @param utils                                  Utility object (reserved).
 * @param parameter                              Provides `soilLayerWidth`.
 * @param numberOfTopSoilLayersAffectedByEvaporation Number of topmost layers
 *                                               to include.
 * @return Cumulative depth of the evaporation-affected soil horizon (cm).
 */
double SOIL::calculateSoilHorizonAffectedBySoilEvaporation(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation)
{
    double soilDepthAffectedByEvaporation = 0; // cm
    for (int soilLayer = 0; soilLayer < numberOfTopSoilLayersAffectedByEvaporation; soilLayer++)
    {
        soilDepthAffectedByEvaporation += parameter.soilLayerWidth.at(soilLayer);
    }
    return soilDepthAffectedByEvaporation;
}

/**
 * @brief Calculates the per-layer distribution of soil evaporation using an
 *        exponential depth-weighting function.
 *
 * Evaporation decreases exponentially with depth. A drainage-proportion
 * parameter (currently hard-coded at 20) controls the rate of decrease.
 * The per-layer fractions are normalised so they sum to 1.
 *
 * @param utils                                  Utility object (reserved).
 * @param parameter                              Provides `soilLayerWidth`.
 * @param numberOfTopSoilLayersAffectedByEvaporation Number of affected layers.
 * @param soilDepthAffectedByEvaporation         Total depth of the evaporation
 *                                               horizon (cm).
 * @param remainingDailyPET                      Remaining PET for soil
 *                                               evaporation (mm; not used
 *                                               directly here but passed through).
 * @return Vector of dimensionless per-layer evaporation fractions summing to 1.
 * @cite Function adapted from BOWET model.
 */
std::vector<double> SOIL::calculateDistributionOfEvaporationAcrossAffectedSoilLayers(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, double soilDepthAffectedByEvaporation, double remainingDailyPET)
{
    std::vector<double> proportionOfEvaporationPerSoilLayer(numberOfTopSoilLayersAffectedByEvaporation, 0.0);

    double layer0 = 0;
    double layer1 = 0;
    double normalizationFactor = 0.0;

    for (int soilLayer = 0; soilLayer < numberOfTopSoilLayersAffectedByEvaporation; soilLayer++)
    {
        double drainageProportion = 20.0; // TODO: add parameter.drainageProportion;
        (soilLayer > 0) ? layer0 += parameter.soilLayerWidth.at(soilLayer - 1) : layer0 = 0;
        layer1 += parameter.soilLayerWidth.at(soilLayer);

        double part1 = -(1.0 / drainageProportion) * (layer1 - layer0);
        double part2 = ((1.0 + drainageProportion) / (std::pow(drainageProportion, 2.0)));
        double part3 = log(1.0 + drainageProportion * (layer1 / soilDepthAffectedByEvaporation)) - log(1.0 + drainageProportion * (layer0 / soilDepthAffectedByEvaporation));
        proportionOfEvaporationPerSoilLayer.at(soilLayer) = part1 + (part2 * part3 * soilDepthAffectedByEvaporation);
        normalizationFactor += proportionOfEvaporationPerSoilLayer.at(soilLayer);
    }

    for (int soilLayer = 0; soilLayer < numberOfTopSoilLayersAffectedByEvaporation; soilLayer++)
    {
        proportionOfEvaporationPerSoilLayer.at(soilLayer) = proportionOfEvaporationPerSoilLayer.at(soilLayer) / normalizationFactor;
    }

    return proportionOfEvaporationPerSoilLayer;
}

/**
 * @brief Calculates actual soil evaporation per layer, limited by soil water
 *        availability above permanent wilting point.
 *
 * For each affected layer with water content > PWP, actual evaporation is:
 * `openCanopy × reductionFactor × proportionOfEvaporation × remainingPET`
 * where `reductionFactor` scales linearly from 0 (at PWP) to 1 (at
 * 70 % of field capacity) and is clamped to 1 above that threshold.
 *
 * @param utils                                  Utility object (reserved).
 * @param parameter                              Provides PWP and field capacity
 *                                               per layer.
 * @param numberOfTopSoilLayersAffectedByEvaporation Number of affected layers.
 * @param proportionOfEvaporationPerSoilLayer    Normalised per-layer fractions
 *                                               from calculateDistributionOf…().
 * @param openCanopy                             Open-canopy fraction (0–1).
 * @param remainingDailyPET                      Available PET for soil
 *                                               evaporation (mm).
 * @return Vector of actual soil evaporation per layer (mm).
 * @cite Function adapted from BOWET model.
 */
std::vector<double> SOIL::calculateSoilEvaporationPerSoilLayer(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, std::vector<double> proportionOfEvaporationPerSoilLayer, double openCanopy, double remainingDailyPET)
{
    std::vector<double> soilEvaporation(numberOfTopSoilLayersAffectedByEvaporation, 0.0);
    for (int soilLayer = 0; soilLayer < numberOfTopSoilLayersAffectedByEvaporation; soilLayer++)
    {
        /* soil water content per soil layer is not allowed to fall below permanent wilting point */
        if (waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) > permanentWiltingPoint.at(soilLayer))
        {
            double criticalPoint = 0.7; // TODO: add parameter for critical point of soil water content for evaporation
            double reductionFactor = (waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) - permanentWiltingPoint.at(soilLayer)) / (criticalPoint * fieldCapacity.at(soilLayer) - permanentWiltingPoint.at(soilLayer));
            reductionFactor = std::min(reductionFactor, 1.0);

            soilEvaporation.at(soilLayer) = openCanopy * reductionFactor * proportionOfEvaporationPerSoilLayer.at(soilLayer) * remainingDailyPET;
        }
    }
    return soilEvaporation;
}

/**
 * @brief Subtracts per-layer soil evaporation from the soil water pool.
 *
 * Deducts `soilEvaporation[i]` from `waterContent_soilWaterPoolPerSoilLayer[i]`
 * and adds it to the `evaporation` accumulator. Enforces a lower bound of
 * permanent wilting point for each layer.
 *
 * @param utils                                  Utility object (reserved).
 * @param parameter                              Provides `permanentWiltingPoint`
 *                                               per layer.
 * @param numberOfTopSoilLayersAffectedByEvaporation Number of affected layers.
 * @param soilEvaporation                        Per-layer evaporation amounts
 *                                               (mm) from calculateSoilEvaporation…().
 */
void SOIL::subtractSoilEvaporationFromSoilWaterPool(UTILS utils, PARAMETER parameter, int numberOfTopSoilLayersAffectedByEvaporation, std::vector<double> soilEvaporation)
{
    for (int soilLayer = 0; soilLayer < numberOfTopSoilLayersAffectedByEvaporation; soilLayer++)
    {
        /* soil water content per soil layer is not allowed to fall below permanent wilting point */
        waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) -= soilEvaporation.at(soilLayer);
        evaporation += soilEvaporation.at(soilLayer);

        /* ensure soil water content does not fall below permanent wilting point */
        waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) = std::max(waterContent_soilWaterPoolPerSoilLayer.at(soilLayer), permanentWiltingPoint.at(soilLayer));
    }
}
/**
 * @brief Orchestrates bare-soil evaporation from the top soil layers.
 *
 * 1. estimateSoilSurfaceAffectedBySoilEvaporation() - fraction of open canopy.
 * 2. calculateSoilHorizonAffectedBySoilEvaporation() - depth of top 4 layers.
 * 3. calculateDistributionOfEvaporationAcrossAffectedSoilLayers() - per-layer fractions.
 * 4. calculateSoilEvaporationPerSoilLayer() - actual mm per layer.
 * 5. subtractSoilEvaporationFromSoilWaterPool() - update pools.
 *
 * @param utils            Utility object for error handling.
 * @param parameter        Provides layer widths, PWP, and field capacity.
 * @param community        Provides LAI accumulators for open-canopy estimate.
 * @param remainingDailyPET PET remaining after snow and interception (mm).
 *
 * @cite Approach adapted from the BOWET model.
 */
void SOIL::evaporationFromTopSoilLayer(UTILS utils, PARAMETER parameter, COMMUNITY &community, double remainingDailyPET)
{
    /* estimate fraction of soil surface affected by evaporation */
    double openCanopy = estimateSoilSurfaceAffectedBySoilEvaporation(utils, community);

    /* calculate soil depth affected by evaporation */
    int numberOfTopSoilLayersAffectedByEvaporation = 4; // TODO: add as parameter
    double soilDepthAffectedByEvaporation = calculateSoilHorizonAffectedBySoilEvaporation(utils, parameter, numberOfTopSoilLayersAffectedByEvaporation);

    /* calculate distribution of evaporation across affected soil layers */
    std::vector<double> proportionOfEvaporationPerSoilLayer = calculateDistributionOfEvaporationAcrossAffectedSoilLayers(utils, parameter, numberOfTopSoilLayersAffectedByEvaporation, soilDepthAffectedByEvaporation, remainingDailyPET);

    /* calculate soil evaporation per soil layer */
    std::vector<double> soilEvaporation = calculateSoilEvaporationPerSoilLayer(utils, parameter, numberOfTopSoilLayersAffectedByEvaporation, proportionOfEvaporationPerSoilLayer, openCanopy, remainingDailyPET);

    subtractSoilEvaporationFromSoilWaterPool(utils, parameter, numberOfTopSoilLayersAffectedByEvaporation, soilEvaporation);
}

/**
 * @brief Orchestrates soil water demand, uptake, and GPP limitation by soil
 *        water availability for all plant cohorts.
 *
 * Steps in order:
 * 1. calculateSoilWaterDemandPerPlant() — compute per-plant and per-layer demand.
 * 2. calculateSoilWaterUptakeByAvailableSoilWaterContentAndLimitPlantGpp() —
 *    compute uptake and reduce GPP by soil-water limitation factor.
 * 3. limitPlantGppAndSoilWaterUptakeByPotentialEvapotranspiration() —
 *    apply PET upper bound on transpiration.
 * 4. limitPlantGppAndSoilWaterUptakeByPermanentWiltingPoint() —
 *    prevent soil water from falling below PWP.
 * 5. subtractPlantWaterUptakeFromSoilWaterPool() — update soil pools.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Model parameters; WUE, module flags, layer count.
 * @param weather   Daily PET for PET-limitation step.
 * @param community Plant community; GPP and water demand/uptake updated.
 * @cite Approach adapted from FORMIND model (www.formind.org).
 */
void SOIL::doPlantSoilWaterUptakeAndGppLimitationBySoilWaterConditions(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community)
{
    /* calculate water demand for each plant in total and per soil layer */
    calculateSoilWaterDemandPerPlant(utils, parameter, community);

    /* reduce plant GPP and soil water uptake due to limitations by available soil water content */
    calculateSoilWaterUptakeByAvailableSoilWaterContentAndLimitPlantGpp(utils, parameter, community);

    /* reduce plant GPP and soil water uptake due to limitations by potential evapotranspiration */
    limitPlantGppAndSoilWaterUptakeByPotentialEvapotranspiration(utils, parameter, weather, community);

    /* reduce plant GPP and soil water uptake due to limitations by soil layer-specific permanent wilting point and soil water content */
    limitPlantGppAndSoilWaterUptakeByPermanentWiltingPoint(utils, parameter, community);

    /* subtract soil layer-specific soil water uptake by all plants from the soil water content per soil layer */
    subtractPlantWaterUptakeFromSoilWaterPool(utils, community, parameter);
}

/**
 * @brief Calculates daily soil water demand for each plant cohort.
 *
 * Demand per plant = GPP / water-use-efficiency. Distributed uniformly over
 * the rooting soil layers. Community-level totals
 * (`totalSoilWaterDemand`, `totalSoilWaterDemandPerSoilLayer`) are accumulated.
 *
 * @param utils     Utility object; raises errors for zero WUE or zero rooting depth.
 * @param parameter Provides `plantWaterUseEfficiency` and `numberOfSoilLayers`.
 * @param community Plant community; per-cohort and community-level demand fields updated.
 */
void SOIL::calculateSoilWaterDemandPerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            int pft = community.allPlants.at(cohortindex)->pft;
            double wue = parameter.plantWaterUseEfficiency[pft];
            double rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;

            // demand per plant and community
            if (wue > 0)
            {
                community.allPlants.at(cohortindex)->soilWaterDemand = community.allPlants.at(cohortindex)->gpp / wue;
            }
            else
            {
                utils.handleError("Water-use-efficiency is zero. Please check the plant traits file!");
            }
            community.totalSoilWaterDemand += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilWaterDemand;

            // demand per plant and community distributed uniformly among rooting layers
            for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
            {
                if (rootingSoilLayers > 0)
                {
                    community.allPlants.at(cohortindex)->soilWaterDemandPerSoilLayer.at(soilLayer) = community.allPlants.at(cohortindex)->soilWaterDemand / rootingSoilLayers;
                }
                else
                {
                    utils.handleError("The rooting depth of the plant is zero. Please check the plant traits file!");
                }
                community.totalSoilWaterDemandPerSoilLayer.at(soilLayer) += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilWaterDemandPerSoilLayer.at(soilLayer);
            }
        }
    }
}

/**
 * @brief Selects and runs the soil-water limitation approach for GPP and
 *        applies the resulting limitation factor to plant GPP.
 *
 * Routes to `calculateBodiumSoilWaterLimitationAndWaterUptake()` for BODIUM
 * coupling, or the internal/self-coupling path which calls
 * `calculateSoilWaterLimitationFactorPerPlant()` and
 * `calculateSoilWaterUptakePerPlant()`. In self-coupling set mode, transfers
 * soil parameters to the interface before the calculation and resets them
 * to NaN after.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Provides module-mode flags and GPP-reduction approach name.
 * @param community Plant community; `limitingFactorGppWater` and GPP updated.
 */
void SOIL::calculateSoilWaterUptakeByAvailableSoilWaterContentAndLimitPlantGpp(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{

    if (parameter.useExternalSoilModule_BODIUM)
    {
        calculateBodiumSoilWaterLimitationAndWaterUptake(utils, parameter, community);
    }
    else
    {
        if (parameter.useInternalSoilModule_selfCoupled_setVariables)
        {
            transferSoilParametersAndVariablesToInterface(utils);
        }
        else if (parameter.useExternalSoilModule_selfCoupled_getVariables)
        {
            transferSoilParametersAndVariablesFromInterface(utils);
        }

        //  Demand per plant and patch for soil water
        calculateSoilWaterLimitationFactorPerPlant(utils, parameter, community);

        //  Uptake per plant and patch for soil water
        calculateSoilWaterUptakePerPlant(utils, community);

        if (parameter.useExternalSoilModule_selfCoupled_getVariables)
        {
            setSoilParametersAndVariablesToNaN(utils, parameter);
        }
    }

    /* reduce GPP by the previously calculated limitation factor */
    reducePlantGppBySoilWaterLimitationFactor(utils, community);
}

/**
 * @brief Copies internal soil parameters and water-content variables to the
 *        self-coupling interface (set mode).
 *
 * Transfers `fieldCapacity`, `permanentWiltingPoint`, `porosity`, and
 * `waterContent_soilWaterPoolPerSoilLayer` to the corresponding
 * `couplingInterface_*` vectors so that a second model instance running in
 * get mode can read them.
 *
 * @param utils Utility object (reserved for future error handling).
 */
void SOIL::transferSoilParametersAndVariablesToInterface(UTILS utils)
{
    couplingInterface_fieldCapacityPerSoilLayer = fieldCapacity;
    couplingInterface_permanentWiltingPointPerSoilLayer = permanentWiltingPoint;
    couplingInterface_porosityPerSoilLayer = porosity;
    couplingInterface_waterContentPerSoilLayer = waterContent_soilWaterPoolPerSoilLayer;
}

/**
 * @brief Loads soil parameters and water-content variables from the self-coupling
 *        interface into the internal soil state (get mode).
 *
 * The reverse of `transferSoilParametersAndVariablesToInterface()`: reads
 * field capacity, PWP, porosity, and water content from the coupling interface
 * vectors into the internal soil state variables.
 *
 * @param utils Utility object (reserved for future error handling).
 */
void SOIL::transferSoilParametersAndVariablesFromInterface(UTILS utils)
{
    fieldCapacity = couplingInterface_fieldCapacityPerSoilLayer;
    permanentWiltingPoint = couplingInterface_permanentWiltingPointPerSoilLayer;
    waterContent_soilWaterPoolPerSoilLayer = couplingInterface_waterContentPerSoilLayer;
    porosity = couplingInterface_porosityPerSoilLayer;
}

/**
 * @brief Sets all internal soil water parameter and state variables to NaN
 *        after they have been copied from the coupling interface (get mode).
 *
 * Prevents accidental use of the coupling-interface values after they have
 * already been applied. Inserts NaN at the front of each vector for
 * `numberOfSoilLayers` positions.
 *
 * @param utils     Utility object (reserved for future error handling).
 * @param parameter Provides `numberOfSoilLayers`.
 */
void SOIL::setSoilParametersAndVariablesToNaN(UTILS utils, PARAMETER parameter)
{
    fieldCapacity.insert(fieldCapacity.begin(), parameter.numberOfSoilLayers, NAN);
    permanentWiltingPoint.insert(permanentWiltingPoint.begin(), parameter.numberOfSoilLayers, NAN);
    waterContent_soilWaterPoolPerSoilLayer.insert(waterContent_soilWaterPoolPerSoilLayer.begin(), parameter.numberOfSoilLayers, NAN);
    porosity.insert(porosity.begin(), parameter.numberOfSoilLayers, NAN);
}

/**
 * @brief Calculates the soil-water GPP limitation factor for each plant cohort.
 *
 * Sums water content, PWP, field capacity, and porosity over the rooting zone,
 * then applies either a `"onesided"` (drought only) or `"twosided"` (drought +
 * waterlogging) reduction model from `parameter.plantGppReductionBySoilWaterApproach`.
 * Result clamped to [0, 1] and stored in `limitingFactorGppWater`.
 *
 * @param utils     Utility object for warnings (threshold violations) and errors.
 * @param parameter Provides approach name, water content thresholds, and layer count.
 * @param community Plant community; `limitingFactorGppWater` updated per cohort.
 */
void SOIL::calculateSoilWaterLimitationFactorPerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            int pft = community.allPlants.at(cohortindex)->pft;
            double rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;

            /* sum up soil parameters and variables in rooting zone */
            double soilWaterInRootingZone = 0, pwpInRootingZone = 0, fcInRootingZone = 0, porInRootingZone = 0;
            for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
            {
                soilWaterInRootingZone += waterContent_soilWaterPoolPerSoilLayer.at(soilLayer);
                pwpInRootingZone += permanentWiltingPoint.at(soilLayer);
                fcInRootingZone += fieldCapacity.at(soilLayer);
                porInRootingZone += porosity.at(soilLayer);
            }

            /* calculate soil water limitation factor for plant to reduce GPP */
            double soilWaterLimitationFactor = 0;
            if (parameter.plantGppReductionBySoilWaterApproach == "onesided")
            {
                double minimalTolerableSoilWaterContentWithoutLimitation = parameter.lowerSoilWaterFractionForPlantGppReduction[pft] * (fcInRootingZone - pwpInRootingZone) + pwpInRootingZone;
                if (soilWaterInRootingZone > pwpInRootingZone && minimalTolerableSoilWaterContentWithoutLimitation > pwpInRootingZone)
                {
                    soilWaterLimitationFactor = (soilWaterInRootingZone - pwpInRootingZone) / (minimalTolerableSoilWaterContentWithoutLimitation - pwpInRootingZone);
                    soilWaterLimitationFactor = std::min(soilWaterLimitationFactor, 1.0);
                }
            }
            else if (parameter.plantGppReductionBySoilWaterApproach == "twosided")
            {
                double minTolerableSoilWaterContentWithoutLimitation = parameter.lowerSoilWaterContentForPlantGppReduction[pft];
                double maxTolerableSoilWaterContentWithoutLimitation = parameter.upperSoilWaterContentForPlantGppReduction[pft];

                if (minTolerableSoilWaterContentWithoutLimitation < pwpInRootingZone)
                {
                    utils.handleWarning("The minimum tolerable soil water content without limitation is below permanent wilting point for a plant. Adjusting to PWP.");
                    minTolerableSoilWaterContentWithoutLimitation = pwpInRootingZone;
                }

                if (maxTolerableSoilWaterContentWithoutLimitation > porInRootingZone)
                {
                    utils.handleWarning("The maximum tolerable soil water content without limitation is above soil porosity for a plant. Adjusting to porosity.");
                    maxTolerableSoilWaterContentWithoutLimitation = porInRootingZone;
                }

                if (maxTolerableSoilWaterContentWithoutLimitation < minTolerableSoilWaterContentWithoutLimitation)
                {
                    utils.handleError("The maximum tolerable soil water content without limitation is below the minimum tolerable soil water content without limitation for a plant. Adjusting maximum to porosity.");
                    maxTolerableSoilWaterContentWithoutLimitation = porInRootingZone;
                }

                if (soilWaterInRootingZone < minTolerableSoilWaterContentWithoutLimitation)
                {
                    soilWaterLimitationFactor = (soilWaterInRootingZone - pwpInRootingZone) / (minTolerableSoilWaterContentWithoutLimitation - pwpInRootingZone);
                    soilWaterLimitationFactor = std::min(soilWaterLimitationFactor, 1.0);
                }
                else if (soilWaterInRootingZone > maxTolerableSoilWaterContentWithoutLimitation)
                {
                    soilWaterLimitationFactor = (porInRootingZone - soilWaterInRootingZone) / (porInRootingZone - maxTolerableSoilWaterContentWithoutLimitation);
                    soilWaterLimitationFactor = std::min(soilWaterLimitationFactor, 1.0);
                }
            }
            else
            {
                utils.handleError("Unknown approach for plant GPP reduction by soil water content. Please check the plant traits file!");
            }

            /* save limitation factor in plant-specific variable */
            community.allPlants.at(cohortindex)->limitingFactorGppWater = soilWaterLimitationFactor;
        }
    }
}

/**
 * @brief Calculates actual soil water uptake per cohort from demand and
 *        limitation factor, and accumulates community totals.
 *
 * Uptake = demand × `limitingFactorGppWater`, distributed uniformly over
 * rooting layers. Updates `soilWaterUptake`, `soilWaterUptakePerSoilLayer`,
 * `totalSoilWaterUptake`, and `totalSoilWaterUptakePerSoilLayer`.
 *
 * @param utils     Utility object (reserved for error handling).
 * @param community Plant community; uptake fields updated per cohort.
 */
void SOIL::calculateSoilWaterUptakePerPlant(UTILS utils, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            community.allPlants.at(cohortindex)->soilWaterUptake = community.allPlants.at(cohortindex)->soilWaterDemand * community.allPlants.at(cohortindex)->limitingFactorGppWater;
            community.totalSoilWaterUptake += community.allPlants.at(cohortindex)->soilWaterUptake * community.allPlants.at(cohortindex)->amount;

            int rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;
            for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
            {
                community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer) = community.allPlants.at(cohortindex)->soilWaterUptake / rootingSoilLayers;
                community.totalSoilWaterUptakePerSoilLayer.at(soilLayer) += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer);
            }
        }
    }
}

/**
 * @brief Calculates soil water limitation factor and plant water uptake using
 *        the BODIUM pressure-head model.
 *
 * Derives a plant water-stress factor from the average root-zone water
 * potential using a piecewise-linear Feddes function with thresholds
 * (h3, h2, h1, h0). h2 is interpolated between h2L and h2H based on
 * potential transpiration rate. Water uptake is distributed per layer
 * weighted by the relative water potential contribution.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides BODIUM coupling constants (h2L, h2H) and soil-layer
 *                  widths for root-surface calculation.
 * @param community Plant community; `limitingFactorGppWater`, `soilWaterUptake`,
 *                  and per-layer uptake fields updated per cohort.
 * @cite Function and code adapted from the BODIUM soil model.
 */
void SOIL::calculateBodiumSoilWaterLimitationAndWaterUptake(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    /* !
    \brief		Calculates water reduction factor plantwise and water uptake (transpiration) per layer
    */
    // const taken from Bodium:
    double h3 = -1500000;       // values from Wesseling and Brandyk, 1985, Pa, except h3, Hydrus Doku S. 109 //
                                // as PWP definition in chernozem
    double h2L = parameter.h2L; //-300000; //-80000;
    double h2H = parameter.h2H; //-50000; //-20000;
    double h1 = -2500;
    double h0 = -1000;
    double transp_low = 1;  // lower potential transpiration rate
    double transp_high = 5; // higher potential transpiration rate

    // ##### Calculations of plants reduction factors #####
    double h2 = (community.totalSoilWaterDemand < transp_low) ? h2L
                : ((community.totalSoilWaterDemand >= transp_low) && (community.totalSoilWaterDemand <= transp_high))
                    ? (h2L + ((community.totalSoilWaterDemand - transp_low) / (transp_high - transp_low)) * (h2H - h2L))
                    : h2H;
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            // iterating through the layers calculating waterPotentialPlantRootAverage (bodiums "psi_t")
            // from landtrans_waterPotentialPerLayer (bodiums "psiloc"),
            // assuming root equally distributed between rooting layers
            double waterPotentialPlantRootAverage = 0.;
            double waterUptakeWeightingFactorSum = 0.;

            for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
            {
                waterPotentialPlantRootAverage +=
                    couplingInterface_soilWaterPotentialPerSoilLayer[soilLayer] / community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;
                waterUptakeWeightingFactorSum += couplingInterface_soilWaterPotentialPerSoilLayer[soilLayer] < h3
                                                     ? 0
                                                     : (couplingInterface_soilWaterPotentialPerSoilLayer[soilLayer] - h3);
            }
            community.allPlants.at(cohortindex)->limitingFactorGppWater =
                (waterPotentialPlantRootAverage < h3)   ? 0
                : (waterPotentialPlantRootAverage < h2) ? ((waterPotentialPlantRootAverage - h3) / (h2 - h3))
                : (waterPotentialPlantRootAverage < h1) ? 1
                : (waterPotentialPlantRootAverage < h0) ? ((waterPotentialPlantRootAverage - h0) / (h1 - h0))
                                                        : 0;

            // water uptake
            community.allPlants.at(cohortindex)->soilWaterUptake = community.allPlants.at(cohortindex)->soilWaterDemand * community.allPlants.at(cohortindex)->limitingFactorGppWater;
            community.totalSoilWaterUptake += community.allPlants.at(cohortindex)->soilWaterUptake * community.allPlants.at(cohortindex)->amount;

            if (community.allPlants.at(cohortindex)->soilWaterUptake > 0)
            {
                for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
                {
                    community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer) =
                        community.allPlants.at(cohortindex)->soilWaterUptake *
                        (couplingInterface_soilWaterPotentialPerSoilLayer[soilLayer] < h3
                             ? 0
                             : (couplingInterface_soilWaterPotentialPerSoilLayer[soilLayer] - h3)) /
                        waterUptakeWeightingFactorSum;
                    community.totalSoilWaterUptakePerSoilLayer.at(soilLayer) += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer);
                }
            }
        }
    }
}

/**
 * @brief Scales GPP down by the previously computed soil-water limitation factor.
 *
 * Multiplies each cohort's `gpp` by `limitingFactorGppWater` (0–1).
 *
 * @param utils     Utility object (reserved for error handling).
 * @param community Plant community; `gpp` updated per cohort.
 */
void SOIL::reducePlantGppBySoilWaterLimitationFactor(UTILS utils, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            community.allPlants.at(cohortindex)->gpp *= community.allPlants.at(cohortindex)->limitingFactorGppWater;
        }
    }
}

/**
 * @brief Applies a PET-based upper bound on community water uptake and GPP.
 *
 * If total uptake exceeds (PET − interception), all uptake and GPP values
 * are scaled by `(PET - interception) / totalUptake` (clamped to [0, 1]).
 * Affects both community totals and per-cohort `gpp` and `soilWaterUptake`.
 *
 * @param utils     Utility object (reserved for error handling).
 * @param parameter Provides `day` and `numberOfSoilLayers`.
 * @param weather   Provides daily PET.
 * @param community Plant community; GPP and water uptake fields scaled.
 * @cite Approach adapted from FORMIND model (www.formind.org).
 */
void SOIL::limitPlantGppAndSoilWaterUptakeByPotentialEvapotranspiration(UTILS utils, PARAMETER parameter, WEATHER weather, COMMUNITY &community)
{
    double totalSoilWaterUptake = community.totalSoilWaterUptake;
    double limitationFactorGppPET = 0;
    double pet = weather.potEvapoTranspiration.at(parameter.day - 1);

    // TODO: use remainingPET here as evaporation from soil already happened
    /* calculate limitation factor */
    if ((totalSoilWaterUptake > 0))
    {
        limitationFactorGppPET = ((pet - interception) / totalSoilWaterUptake);
        limitationFactorGppPET = std::min(limitationFactorGppPET, 1.0);
    }

    /* apply limitation factor to community variables */
    community.totalSoilWaterUptake *= limitationFactorGppPET;
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        community.totalSoilWaterUptakePerSoilLayer.at(soilLayer) *= limitationFactorGppPET;
    }

    /* apply limitation factor to plant-specific variables */
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            community.allPlants.at(cohortindex)->gpp *= limitationFactorGppPET;
            community.allPlants.at(cohortindex)->soilWaterUptake *= limitationFactorGppPET;
            for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
            {
                community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer) *= limitationFactorGppPET;
            }
        }
    }
}

/**
 * @brief Prevents soil water content from falling below permanent wilting point
 *        by reducing plant uptake (and associated GPP) accordingly.
 *
 * For the internal/self-coupling path, calls `limitSoilWaterUptakeByPermanentWiltingPoint()`
 * to compute per-layer reduction factors, then passes them to
 * `reducePlantGppBasedOnLimitationByPermanentWiltingPoint()`. For BODIUM/get
 * coupling, reads the reduction factors from the interface.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Provides module-mode flags and `numberOfSoilLayers`.
 * @param community Plant community; uptake and GPP fields updated.
 * @cite Approach adapted from FORMIND model (www.formind.org).
 */
void SOIL::limitPlantGppAndSoilWaterUptakeByPermanentWiltingPoint(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    std::vector<double> waterUptakeReductionByAvailableWaterPerLayer(parameter.numberOfSoilLayers, 0);

    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        // current PWP limitation works based on water amount
        // in BODIUM coupling water limitation is based on water potential
        // TODO: new PWP limitation criterion?
        waterUptakeReductionByAvailableWaterPerLayer = limitSoilWaterUptakeByPermanentWiltingPoint(utils, parameter, community);
        if (parameter.useInternalSoilModule_selfCoupled_setVariables)
        {
            couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer = waterUptakeReductionByAvailableWaterPerLayer;
        }
    }
    else if (parameter.useExternalSoilModule_BODIUM || parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        waterUptakeReductionByAvailableWaterPerLayer = couplingInterface_waterUptakeReductionByAvailableWaterPerSoilLayer;
        double newWaterUptake = 0;
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            community.totalSoilWaterUptakePerSoilLayer.at(soilLayer) *= (1 - waterUptakeReductionByAvailableWaterPerLayer.at(soilLayer));
            newWaterUptake += community.totalSoilWaterUptakePerSoilLayer.at(soilLayer);
        }
        community.totalSoilWaterUptake = newWaterUptake;
    }

    reducePlantGppBasedOnLimitationByPermanentWiltingPoint(utils, community, waterUptakeReductionByAvailableWaterPerLayer);
    // plant->limitingFactorGppWater is not changed here, but gpp, transpiration and water uptake
    // limitingFactorGppWater only describes the reduction due to competition
    // instead limitingFactorGppWaterPwpPet includes reduction due to comeptition and PET/pwp limitation
    // together
}

/**
 * @brief Computes per-layer maximum-allowed uptake to prevent soil water from
 *        falling below PWP, and updates community-level uptake accordingly.
 *
 * For each layer where (`soilWater - totalUptake`) < PWP, clamps uptake to
 * `max(0, soilWater - PWP)` and records a reduction factor
 * (1 - allowedUptake/demandedUptake). Returns the vector of per-layer
 * reduction factors for use by `reducePlantGppBasedOnLimitationByPermanentWiltingPoint()`.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides PWP, field capacity, and `numberOfSoilLayers`.
 * @param community Plant community; community-level uptake totals updated.
 * @return Vector of per-layer reduction factors (0 = no reduction, 1 = full reduction).
 * @cite Approach adapted from FORMIND model (www.formind.org).
 */
std::vector<double> SOIL::limitSoilWaterUptakeByPermanentWiltingPoint(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    double limitedTotalSoilWaterUptake = 0;
    std::vector<double> limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer(parameter.numberOfSoilLayers, 0); // 1 for full reduction to zero uptake, 0 for no reduction // TODO: remove 1.0 here and in next function?

    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        double pwp = permanentWiltingPoint.at(soilLayer);
        double soilWaterContent = waterContent_soilWaterPoolPerSoilLayer.at(soilLayer);
        double soilWaterUptake = community.totalSoilWaterUptakePerSoilLayer.at(soilLayer);

        /* soil water content shall not fall below PWP after plant water uptake */
        if ((soilWaterContent - soilWaterUptake) < pwp)
        {
            /* calculate maximum soil water that is allowed to take up due to permanent wilting point */
            double maximumAllowedSoilWaterUptakeAtSoilLayer = std::max(0.0, soilWaterContent - pwp);

            /* calculate limitation factor */
            if (soilWaterUptake > 0)
            {
                limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer.at(soilLayer) = 1.0 - (maximumAllowedSoilWaterUptakeAtSoilLayer / soilWaterUptake); // TODO: remove 1.0 here and in next function?
            }

            /* correct soil water uptake for each soil layer to maximum allowed uptake */
            community.totalSoilWaterUptakePerSoilLayer.at(soilLayer) = maximumAllowedSoilWaterUptakeAtSoilLayer;
        }

        /* recalculate updated total soil water uptake by plants */
        limitedTotalSoilWaterUptake += community.totalSoilWaterUptakePerSoilLayer.at(soilLayer);
    }
    community.totalSoilWaterUptake = limitedTotalSoilWaterUptake;

    /* return limitation factor for each soil layer */
    return limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer;
}

/**
 * @brief Reduces per-cohort GPP and water uptake based on PWP limitation
 *        factors computed per soil layer.
 *
 * For each cohort with positive water uptake, computes a weighted-average
 * reduction factor across rooting layers (weighted by per-layer uptake fraction)
 * and applies it to `gpp` and `soilWaterUptake`. Also sets `limitingFactorGppTotal`.
 *
 * @param utils                                    Utility object (reserved).
 * @param community                                Plant community; GPP and uptake updated.
 * @param limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer
 *        Per-layer reduction factors from `limitSoilWaterUptakeByPermanentWiltingPoint()`.
 */
void SOIL::reducePlantGppBasedOnLimitationByPermanentWiltingPoint(UTILS utils, COMMUNITY &community, std::vector<double> limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->soilWaterUptake > 0)
        {
            double waterUptakeReductionFactor = 0;
            int rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;

            for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
            {
                double percentageOfPlantSoilWaterUptakePerSoilLayerToTotalUptake = (community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer) / community.allPlants.at(cohortindex)->soilWaterUptake);
                // for uniform distribution of wateruptake among layers,
                // waterUptakePerLayer/waterUptake is equal to 1/numberOfSoilLayersRooting.
                // But in this way, robust against changes in distribution

                waterUptakeReductionFactor += (1 - limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer.at(soilLayer)) * percentageOfPlantSoilWaterUptakePerSoilLayerToTotalUptake;

                community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer.at(soilLayer) *= (1 - limitationFactorOfSoilWaterUptakeByPwpPerSoilLayer.at(soilLayer));
                // important, that this is calculated after the calculation of percentageOfPlantSoilWaterUptakePerSoilLayerToTotalUptake and waterUptakeReductionFactor
            }

            community.allPlants.at(cohortindex)->gpp *= waterUptakeReductionFactor;
            community.allPlants.at(cohortindex)->soilWaterUptake *= waterUptakeReductionFactor;
            community.allPlants.at(cohortindex)->limitingFactorGppTotal = community.allPlants.at(cohortindex)->soilWaterUptake / community.allPlants.at(cohortindex)->soilWaterDemand;
        }
    }
}

/**
 * @brief Subtracts per-layer community water uptake from the soil water pool
 *        (internal module) or exports it to the BODIUM coupling interface.
 *
 * @param utils     Utility object (reserved).
 * @param community Provides `totalSoilWaterUptakePerSoilLayer`.
 * @param parameter Provides module-mode flags and `numberOfSoilLayers`.
 */
void SOIL::subtractPlantWaterUptakeFromSoilWaterPool(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{
    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        //  update of soil water content due to water uptake
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            waterContent_soilWaterPoolPerSoilLayer.at(soilLayer) -= community.totalSoilWaterUptakePerSoilLayer.at(soilLayer);
        }
    }
    else if (parameter.useExternalSoilModule_BODIUM || parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
        {
            couplingInterface_plantSoilWaterUptakePerLayer.at(soilLayer) = community.totalSoilWaterUptakePerSoilLayer.at(soilLayer);
        }
    }
}

/**
 * @brief Orchestrates soil nitrogen demand, uptake, NPP limitation, and
 *        nitrogen allocation for all plant cohorts.
 *
 * Steps in order:
 * 1. calculateLimitingFactorOfSymbioticNitrogenFixationByRhizobiaPerPlant()
 * 2. calculateSoilNitrogenDemandPerPlant()
 * 3. summarizeTotalSoilNitrogenDemandFromAllPlants()
 * 4. calculateBodiumSoilNitrogenUptake() (BODIUM) or
 *    calculateSoilNitrogenUptakePerPlant() (internal/self-coupling)
 * 5. calculateNitrogenLimitationFactorPerPlant()
 * 6. allocateNitrogenUptakeToPlant()
 * 7. summarizeTotalSoilNitrogenUptakeFromAllPlants() + deduction from pool
 *    (internal module only).
 *
 * @param utils     Utility object for error handling.
 * @param parameter Provides module flags, PFT parameters, layer count.
 * @param community Plant community; all N demand/uptake and NPP limitation
 *                  fields updated.
 */
void SOIL::doPlantSoilNitrogenUptakeAndNppLimitationBySoilNitrogenConditions(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    /* Limiting factor of symbiotic nitrogen fixation by rhizobia per plant */
    calculateLimitingFactorOfSymbioticNitrogenFixationByRhizobiaPerPlant(utils, community, parameter);

    /* (Soil) nitrogen demand per plant */
    calculateSoilNitrogenDemandPerPlant(utils, community, parameter);

    /* Total demand of all plants from soil */
    summarizeTotalSoilNitrogenDemandFromAllPlants(utils, community);

    /* calculate plant nitrogen uptake from soil */
    if (parameter.useExternalSoilModule_BODIUM)
    {
        calculateBodiumSoilNitrogenUptake(utils, parameter, community);
    }
    else
    {
        /* only used in case of model self-coupling */
        transferInterfaceVariablesForSelfCoupling_soilNitrogen(utils, parameter);

        /* calculation of soil nitrogen uptake per plant */
        calculateSoilNitrogenUptakePerPlant(utils, parameter, community);

        /* only used in case of model self-coupling */
        resetInterfaceVariablesForSelfCoupling_soilNitrogen(utils, parameter);
    }

    // calculate nitrogen limitation factor for each plant
    calculateNitrogenLimitationFactorPerPlant(utils, community);

    // allocate nitrogen uptake to plant pools for growth (shoot, root, reproduction, exudates)
    allocateNitrogenUptakeToPlant(utils, community);

    if (parameter.useInternalSoilModule || parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        summarizeTotalSoilNitrogenUptakeFromAllPlants(utils, parameter, community);

        // updates soil mineral nitrogen pool for each layer
        subtractPlantNitrogenUptakeFromSoilMineralNitrogenPool(utils, parameter, community);
    }
}

/**
 * @brief Copies mineral nitrogen per layer to/from the self-coupling interface.
 *
 * In set mode: copies `nitrogenContent_soilMineralPoolPerSoilLayer` to
 * `couplingInterface_nitrogenContentPerSoilLayer`. In get mode: does the reverse.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides module-mode flags.
 */
void SOIL::transferInterfaceVariablesForSelfCoupling_soilNitrogen(UTILS utils, PARAMETER parameter)
{
    if (parameter.useInternalSoilModule_selfCoupled_setVariables)
    {
        couplingInterface_nitrogenContentPerSoilLayer = nitrogenContent_soilMineralPoolPerSoilLayer;
    }
    else if (parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        nitrogenContent_soilMineralPoolPerSoilLayer = couplingInterface_nitrogenContentPerSoilLayer;
    }
}

/**
 * @brief Resets the internal mineral nitrogen pool to NaN after it has been
 *        consumed from the coupling interface (get mode).
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `numberOfSoilLayers` and module-mode flags.
 */
void SOIL::resetInterfaceVariablesForSelfCoupling_soilNitrogen(UTILS utils, PARAMETER parameter)
{
    if (parameter.useExternalSoilModule_selfCoupled_getVariables)
    {
        nitrogenContent_soilMineralPoolPerSoilLayer.insert(nitrogenContent_soilMineralPoolPerSoilLayer.begin(), parameter.numberOfSoilLayers, NAN); // make sure that variabls aren't used anywhere else
    }
}

/**
 * @brief Calculates the limiting factor for symbiotic N fixation (rhizobia) per
 *        cohort based on the C-cost of fixation.
 *
 * For PFTs with `symbioticNitrogenFixationFraction > 0`, computes the
 * mean C/N ratio of growth biomass across allocation fractions and pools,
 * then sets `limitingFactorSymbiosisRhizobia` using:
 * @f[ f = \frac{C/N}{C/N + f_{\text{symb}} \cdot r_{\text{CN}}} @f]
 * where `f_symb` is the fixation fraction and `r_CN` is the rhizobia C/N
 * exchange rate. Otherwise `limitingFactorSymbiosisRhizobia` stays 1.0.
 *
 * @param utils     Utility object (reserved).
 * @param community Plant community; `limitingFactorSymbiosisRhizobia` updated.
 * @param parameter PFT-specific fixation fractions, exchange rate, C/N ratios.
 */
void SOIL::calculateLimitingFactorOfSymbioticNitrogenFixationByRhizobiaPerPlant(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {

            int pft = community.allPlants.at(cohortindex)->pft;
            if (parameter.symbioticNitrogenFixationFraction[pft] > 0)
            {
                double cnRatioBiomassGrowth = 1 / (community.allPlants.at(cohortindex)->nppAllocationShoot / parameter.plantCNRatioGreenLeaves[pft] +
                                                   community.allPlants.at(cohortindex)->nppAllocationRoot / parameter.plantCNRatioRoots[pft] +
                                                   community.allPlants.at(cohortindex)->nppAllocationRecruitment / parameter.plantCNRatioSeeds[pft] +
                                                   community.allPlants.at(cohortindex)->nppAllocationExudation / parameter.plantCNRatioExudates[pft]);

                community.allPlants.at(cohortindex)->limitingFactorSymbiosisRhizobia = cnRatioBiomassGrowth / (cnRatioBiomassGrowth + parameter.symbioticNitrogenFixationFraction[pft] * parameter.rhizobiaExchangeRateCToN);
            } // otherwise limitingFactorSymbiosisRhizobia stays 1.0
        }
    }
}

/**
 * @brief Calculates the nitrogen demand for each plant cohort and its
 *        remaining demand to be met from the soil mineral pool.
 *
 * For each cohort: deducts the C cost of rhizobial fixation from potential
 * NPP carbon, computes per-organ N demands and fraction vectors, checks that
 * rhizobia uptake does not exceed total demand, then derives the remaining
 * soil demand after accounting for rhizobia uptake and nitrogen surplus.
 *
 * @param utils     Utility object for error handling.
 * @param community Plant community; demand fields updated per cohort.
 * @param parameter PFT-specific C/N ratios, fixation parameters, allocation fractions.
 */
void SOIL::calculateSoilNitrogenDemandPerPlant(UTILS utils, COMMUNITY &community, PARAMETER parameter)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            double carbonContentOfPotentialNPP = community.allPlants.at(cohortindex)->npp * CARBON_CONTENT_ODM;

            /* substract rhizobia costs from potential carbon increment for legumes */
            carbonContentOfPotentialNPP = calculateNitrogenUptakeByRhizobiaAndSubtractCostsFromPotentialNPP(utils, parameter, community, cohortindex, carbonContentOfPotentialNPP);

            /* calculate nitrogen demands for respective organs from C:N ratios */
            calculateNitrogenDemandOfPlantFromPotentialNPP(utils, parameter, community, cohortindex, carbonContentOfPotentialNPP);

            /* check that rhizobia nitrogen uptake does not exceed total nitrogen demand of the plant */
            checkIfPlantNitrogenDemandIsNotExceededByRhizobiaUptake(utils, community, cohortindex);

            /* calculate the remaining nitrogen demand (after rhizobia uptake and surplus), trying to get from soil */
            calculateRemainingPlantNitrogenDemandFromSoil(utils, community, cohortindex);
        }
    }
}

/**
 * @brief Computes rhizobial N uptake and deducts the equivalent C cost from
 *        potential NPP carbon.
 *
 * Rhizobia N uptake = `carbonNPP * (1 - limitingFactorSymbiosisRhizobia)
 *                      / rhizobiaExchangeRateCToN`.
 * The carbon cost is deducted from `carbonContentOfPotentialNPP` and the
 * adjusted value is returned.
 *
 * @param utils                    Utility object (reserved).
 * @param parameter                Provides rhizobia exchange rate.
 * @param community                Plant community; `rhizobiaNitrogenUptake` set.
 * @param cohortindex              Index of the target cohort.
 * @param carbonContentOfPotentialNPP Potential NPP carbon before rhizobia cost.
 * @return Remaining NPP carbon after deducting rhizobia cost.
 */
double SOIL::calculateNitrogenUptakeByRhizobiaAndSubtractCostsFromPotentialNPP(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double carbonContentOfPotentialNPP)
{
    // if symbioticNitrogenFixationFraction > 0, i.e. if limitingFactorSymbiosisRhizobia < 1
    // TODO: this rhizobia uptake does not take into account potential nitrogen coming from surplus
    // (opposed to how it is done for nitrogen uptake from soil --> could be discussed)
    community.allPlants.at(cohortindex)->rhizobiaNitrogenUptake = carbonContentOfPotentialNPP * (1 - community.allPlants.at(cohortindex)->limitingFactorSymbiosisRhizobia) / parameter.rhizobiaExchangeRateCToN;
    carbonContentOfPotentialNPP *= community.allPlants.at(cohortindex)->limitingFactorSymbiosisRhizobia;

    return (carbonContentOfPotentialNPP);
}

/**
 * @brief Derives per-organ N demands and their allocation fractions from
 *        potential NPP carbon and PFT-specific C/N ratios.
 *
 * Computes `nitrogenDemandForGrowthOf{Shoot,Root,Reproduction,Exudation}`,
 * `totalPlantNitrogenDemand`, and the corresponding fraction vectors
 * `nitrogenDemandGrowthFraction*`. Fractions are set to zero if total demand
 * is zero.
 *
 * @param utils                      Utility object (reserved).
 * @param parameter                  Provides PFT-specific C/N ratios.
 * @param community                  Plant community; demand and fraction fields updated.
 * @param cohortindex                Index of the target cohort.
 * @param carbonContentOfPotentialNPP NPP carbon after rhizobia cost deduction.
 */
void SOIL::calculateNitrogenDemandOfPlantFromPotentialNPP(UTILS utils, PARAMETER parameter, COMMUNITY &community, int cohortindex, double carbonContentOfPotentialNPP)
{
    int pft = community.allPlants.at(cohortindex)->pft;
    community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfShoot = community.allPlants.at(cohortindex)->nppAllocationShoot * carbonContentOfPotentialNPP / parameter.plantCNRatioGreenLeaves[pft];
    community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfRoot = community.allPlants.at(cohortindex)->nppAllocationRoot * carbonContentOfPotentialNPP / parameter.plantCNRatioRoots[pft];
    community.allPlants.at(cohortindex)->nitrogenDemandForReproduction = community.allPlants.at(cohortindex)->nppAllocationRecruitment * carbonContentOfPotentialNPP / parameter.plantCNRatioSeeds[pft];
    community.allPlants.at(cohortindex)->nitrogenDemandForExudation = community.allPlants.at(cohortindex)->nppAllocationExudation * carbonContentOfPotentialNPP / parameter.plantCNRatioExudates[pft];
    community.allPlants.at(cohortindex)->totalPlantNitrogenDemand = community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfShoot + community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfRoot +
                                                                    community.allPlants.at(cohortindex)->nitrogenDemandForReproduction + community.allPlants.at(cohortindex)->nitrogenDemandForExudation;

    // calculate fractions for later distributing uptake to the respective usage
    // do not move more below as nitrogenDemand is changed depending on where resources come from
    // (own relocation vs. soil resource)
    if (community.allPlants.at(cohortindex)->totalPlantNitrogenDemand > 0)
    {
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionShoot = community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfShoot / community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionRoot = community.allPlants.at(cohortindex)->nitrogenDemandForGrowthOfRoot / community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionReproduction = community.allPlants.at(cohortindex)->nitrogenDemandForReproduction / community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionExudation = community.allPlants.at(cohortindex)->nitrogenDemandForExudation / community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;
    }
    else
    {
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionShoot = 0;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionRoot = 0;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionReproduction = 0;
        community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionExudation = 0;
    }
}

/**
 * @brief Raises an error if rhizobial N uptake exceeds total plant N demand.
 *
 * @param utils       Utility object for error handling.
 * @param community   Plant community; reads `rhizobiaNitrogenUptake` and
 *                    `totalPlantNitrogenDemand` from the target cohort.
 * @param cohortindex Index of the target cohort.
 */
void SOIL::checkIfPlantNitrogenDemandIsNotExceededByRhizobiaUptake(UTILS utils, COMMUNITY &community, int cohortindex)
{
    if (community.allPlants.at(cohortindex)->rhizobiaNitrogenUptake > community.allPlants.at(cohortindex)->totalPlantNitrogenDemand)
    {
        utils.handleError("Rhizobia nitrogen uptake exceeds total nitrogen demand of the plant!");
    }
}

/**
 * @brief Derives the remaining N demand to be met from the soil mineral pool
 *        after accounting for rhizobia uptake and internal nitrogen surplus.
 *
 * `totalSoilNitrogenDemand = max(0, totalDemand - rhizobiaN - surplus)`.
 * Distributed uniformly over rooting layers.
 *
 * @param utils       Utility object (reserved).
 * @param community   Plant community; `totalSoilNitrogenDemand` and per-layer
 *                    demand updated for the target cohort.
 * @param cohortindex Index of the target cohort.
 */
void SOIL::calculateRemainingPlantNitrogenDemandFromSoil(UTILS utils, COMMUNITY &community, int cohortindex)
{
    double totalPlantNitrogenDemand = community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;
    double rhizobiaNitrogenUptake = community.allPlants.at(cohortindex)->rhizobiaNitrogenUptake;
    double nitrogenSurplus = community.allPlants.at(cohortindex)->nitrogenSurplus;
    community.allPlants.at(cohortindex)->totalSoilNitrogenDemand = std::max(0.0, totalPlantNitrogenDemand - rhizobiaNitrogenUptake - nitrogenSurplus);

    // distribute total nitrogen demand from soil uniformly among rooting layers
    if (community.allPlants.at(cohortindex)->totalSoilNitrogenDemand > 0)
    {
        int rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;
        for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
        {
            community.allPlants.at(cohortindex)->soilNitrogenDemandPerSoilLayer.at(soilLayer) = community.allPlants.at(cohortindex)->totalSoilNitrogenDemand / rootingSoilLayers;
        }
    }
}

/**
 * @brief Aggregates per-cohort soil N demand into community-level totals.
 *
 * Accumulates `totalSoilNitrogenDemand`, per-layer demand, and the count of
 * competitors per layer (`numberOfPlantsCompetingForSoilNitrogenPerSoilLayer`).
 *
 * @param utils     Utility object (reserved).
 * @param community Plant community; community-level N demand fields updated.
 */
void SOIL::summarizeTotalSoilNitrogenDemandFromAllPlants(UTILS utils, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            /* sum up all plants' nitrogen demand in total */
            community.totalSoilNitrogenDemand += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->totalSoilNitrogenDemand;

            /* sum up number of plants competing for soil nitrogen and total nitrogen demand per soil layer */
            if (community.totalSoilNitrogenDemand > 0)
            {
                int rootingSoilLayers = community.allPlants.at(cohortindex)->numberOfSoilLayersRooting;
                for (int soilLayer = 0; soilLayer < rootingSoilLayers; soilLayer++)
                {
                    // if demand > 0: add number of plants to counter numberOfPlantsCompetingForSoilNitrogenPerSoilLayer
                    community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.at(soilLayer) += community.allPlants.at(cohortindex)->amount * (community.allPlants.at(cohortindex)->soilNitrogenDemandPerSoilLayer.at(soilLayer) > 0);
                    community.totalSoilNitrogenDemandPerSoilLayer.at(soilLayer) += community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilNitrogenDemandPerSoilLayer.at(soilLayer);
                }
            }
        }
    }
}

/**
 * @brief Calculates actual N uptake per cohort from available soil mineral N,
 *        shared equally among competing plants per layer.
 *
 * For each layer, divides available mineral N by the number of competitors to
 * obtain the maximum per-plant supply, then sets each cohort's uptake to
 * `min(demand, supply)`. Finally sums per-layer uptake into `totalSoilNitrogenUptake`
 * per cohort.
 *
 * @param utils     Utility object; raises an error for negative mineral N.
 * @param parameter Provides `numberOfSoilLayers`.
 * @param community Plant community; per-layer and total uptake updated per cohort.
 */
void SOIL::calculateSoilNitrogenUptakePerPlant(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    double maximumNitrogenSupplyPerPlant = 0.0;

    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        if (community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.at(soilLayer) > 0)
        {
            /* amount of soil nitrogen that is available to each plant */
            if (nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) > 0)
            {
                maximumNitrogenSupplyPerPlant = nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) / community.numberOfPlantsCompetingForSoilNitrogenPerSoilLayer.at(soilLayer);
            }
            else
            {
                utils.handleError("Error (soil.cpp): nitrogenContent_soilMineralPoolPerSoilLayer is negative!");
            }

            /* for each plant rooting in this soil layer, calculate uptake as amount that is needed relative to available supply */
            for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
            {
                if (community.allPlants.at(cohortindex)->amount > 0 && community.allPlants.at(cohortindex)->numberOfSoilLayersRooting > soilLayer)
                {
                    community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer.at(soilLayer) = std::min(community.allPlants.at(cohortindex)->soilNitrogenDemandPerSoilLayer.at(soilLayer), maximumNitrogenSupplyPerPlant);
                }
            }
        }
    }

    /* summarize nitrogen uptake from all soil layers for each plant */
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            community.allPlants.at(cohortindex)->totalSoilNitrogenUptake = 0.0;
            for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
            {
                community.allPlants.at(cohortindex)->totalSoilNitrogenUptake += community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer.at(soilLayer);
            }
        }
    }
}

/**
 * @brief Calculates N uptake via the BODIUM coupling model (convection +
 *        diffusion) for each plant cohort.
 *
 * First computes convective N uptake from water flow and pore-water
 * NH4/NO3 concentrations (capped at a minimum `n_lim`). If convection is
 * insufficient to meet demand, adds diffusive uptake proportional to root
 * surface area and concentration gradients. Exports per-layer NH4/NO3 uptake
 * to the coupling interface.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides BODIUM coupling constants (h2L, h2H) and soil-layer
 *                  widths for root-surface calculation.
 * @param community Plant community; `soilNitrogenUptakePerSoilLayer` and
 *                  `totalSoilNitrogenUptake` updated per cohort.
 * @cite Approach adapted from BODIUM soil model.
 */
void SOIL::calculateBodiumSoilNitrogenUptake(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    double meanRootDiameter = 5e-4; // m
    double meanSpecificRootLength =
        120000 / 1000; // BODIUM config summeroats (m per kg), conversion to m per g

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        community.allPlants.at(cohortindex)->totalSoilNitrogenUptake = 0.0;
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            double n_demand =
                community.allPlants.at(cohortindex)->totalSoilNitrogenDemand / 1000; // concversion from g to kg (per squaremeter and day)
            double total_n = 0, n_conv = 0, n_diff = 0;
            double water_up_n = 0;
            double n_lim = 0.0004;                         // value from BODIUM TODO: different values for different plants?
            n_lim = (n_lim > n_demand) ? n_demand : n_lim; // convective update should not be bigger than demand
            double surface;

            for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
            {
                double n_conv_node = 0;
                // water uptake per node
                water_up_n = community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer[soilLayer]; // mm per layer (on one m**2) per day

                // N uptake by convection per node
                double nh4conc =
                    couplingInterface_soilWaterNh4Concentration[soilLayer]; // TODO: check in coupling, that dim = dim
                                                                            // of par.Water_SoilLayer or .at() here
                double no3conc = couplingInterface_soilWaterNo3Concentration[soilLayer];
                nh4conc =
                    nh4conc > 0.043 ? 0.043 : nh4conc; // set a maximum concentration to prevent too high uptake
                no3conc = no3conc > 0.043 ? 0.043 : no3conc;
                n_conv_node = water_up_n * (nh4conc + no3conc);

                // total N uptake by convection
                n_conv += n_conv_node;
            }
            // compare n_conv with n_lim per node
            for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
            {
                double n_conv_node = 0;
                water_up_n = community.allPlants.at(cohortindex)->soilWaterUptakePerSoilLayer[soilLayer];
                double nh4conc = couplingInterface_soilWaterNh4Concentration[soilLayer];
                double no3conc = couplingInterface_soilWaterNo3Concentration[soilLayer];
                nh4conc = nh4conc > 0.043 ? 0.043 : nh4conc;
                no3conc = no3conc > 0.043 ? 0.043 : no3conc;

                n_conv_node = water_up_n * (nh4conc + no3conc);

                if (n_conv > n_lim)
                {
                    double new_nup = (n_conv_node / n_conv) * n_lim;
                    double ratio_nh4 = nh4conc / (nh4conc + no3conc);
                    ratio_nh4 = (std::isnan(ratio_nh4)) ? 0 : ratio_nh4;
                    couplingInterface_plantNh4UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (new_nup * ratio_nh4);       // kg
                    couplingInterface_plantNo3UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (new_nup * (1 - ratio_nh4)); // kg
                    community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer[soilLayer] += new_nup * 1000;                                     // in g
                    community.allPlants.at(cohortindex)->totalSoilNitrogenUptake += new_nup * 1000;                                                       // g
                }
                else
                {
                    couplingInterface_plantNh4UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (water_up_n * nh4conc); // kg
                    couplingInterface_plantNo3UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (water_up_n * no3conc); // kg
                    community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer[soilLayer] +=
                        water_up_n * (nh4conc + no3conc) * 1000;                                                             // in g
                    community.allPlants.at(cohortindex)->totalSoilNitrogenUptake += water_up_n * (nh4conc + no3conc) * 1000; // g
                }
            }
            n_conv = (n_conv > n_lim) ? n_lim : n_conv;
            n_demand = (n_demand > n_lim) ? n_lim : n_demand;
            // TODO also check, that conv isnt bigger than demand? (change here and above)

            // if convection is not enough, we need diffusion
            if (n_conv < n_demand)
            {
                double diff_coeff = 5E-7; // diff_coeff = D(theta)/l_d in m/s, D is 5e-11 m²/s,
                double n_diff_pot = n_demand - n_conv;
                bool usepot;
                double depth = 0.0;

                //  calculate max diffusion  (all layers)
                for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
                {
                    double n_diff_node = 0;

                    if (soilLayer < (community.allPlants.at(cohortindex)->numberOfSoilLayersRooting - 1))
                    {
                        surface = community.allPlants.at(cohortindex)->rootBiomass * parameter.soilLayerWidth[soilLayer] / community.allPlants.at(cohortindex)->rootingDepth *
                                  meanSpecificRootLength * 2 * PI * (meanRootDiameter / 2); // m^2
                        depth += parameter.soilLayerWidth[soilLayer];
                    }
                    else
                    {
                        surface = community.allPlants.at(cohortindex)->rootBiomass * (1 - (depth / community.allPlants.at(cohortindex)->rootingDepth)) *
                                  meanSpecificRootLength * 2 * PI * (meanRootDiameter / 2);
                    }
                    // double surface=2*(theRoot->getVolume()/(mean_dia/2)); // pi r² -> 2pi r, m²/Node
                    n_diff_node = surface * diff_coeff * DAY_IN_SECONDS *
                                  (couplingInterface_soilWaterNh4Concentration[soilLayer] +
                                   couplingInterface_soilWaterNo3Concentration[soilLayer]);
                    // total N uptake per diffusion
                    n_diff += n_diff_node;
                }

                // is the maximum possible diffusion smaller than the potential demand?
                usepot = (n_diff <= n_diff_pot) ? false : true;

                for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
                {
                    double n_diff_node = 0;

                    if (soilLayer < (community.allPlants.at(cohortindex)->numberOfSoilLayersRooting - 1))
                    {
                        surface = community.allPlants.at(cohortindex)->rootBiomass * parameter.soilLayerWidth[soilLayer] / community.allPlants.at(cohortindex)->rootingDepth *
                                  meanSpecificRootLength * 2 * PI * (meanRootDiameter / 2);
                    }
                    else
                    {
                        surface = community.allPlants.at(cohortindex)->rootBiomass * (1 - (depth / community.allPlants.at(cohortindex)->rootingDepth)) *
                                  meanSpecificRootLength * 2 * PI * (meanRootDiameter / 2);
                    }
                    // double surface=2*(theRoot->getVolume()/(mean_dia/2));
                    n_diff_node = surface * diff_coeff * DAY_IN_SECONDS *
                                  (couplingInterface_soilWaterNh4Concentration[soilLayer] +
                                   couplingInterface_soilWaterNo3Concentration[soilLayer]);

                    double ratio_NH4 = (couplingInterface_soilWaterNh4Concentration[soilLayer] +
                                            couplingInterface_soilWaterNo3Concentration[soilLayer] >
                                        0)
                                           ? (couplingInterface_soilWaterNh4Concentration[soilLayer]) /
                                                 (couplingInterface_soilWaterNh4Concentration[soilLayer] +
                                                  couplingInterface_soilWaterNo3Concentration[soilLayer])
                                           : 0;

                    n_diff_node = (usepot) ? n_diff_pot * (n_diff_node / n_diff) : n_diff_node;

                    couplingInterface_plantNh4UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (n_diff_node * ratio_NH4);       // kg
                    couplingInterface_plantNo3UptakePerSoilLayer[soilLayer] += community.allPlants.at(cohortindex)->amount * (n_diff_node * (1 - ratio_NH4)); // kg
                    community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer[soilLayer] += n_diff_node * 1000;                                     // g
                    community.allPlants.at(cohortindex)->totalSoilNitrogenUptake += n_diff_node * 1000;                                                       // g
                }
                n_diff = (usepot) ? n_diff_pot : n_diff;
                total_n = n_conv + n_diff;
            }
        }
    }
}

/**
 * @brief Computes the N limitation factor for NPP per cohort.
 *
 * `limitingFactorNppNitrogen = min(1, totalNSupply / totalNDemand)` where
 * supply = soil uptake + rhizobia uptake + nitrogen surplus. Set to 1 when
 * demand is zero.
 *
 * @param utils     Utility object (reserved).
 * @param community Plant community; `limitingFactorNppNitrogen` updated per cohort.
 */
void SOIL::calculateNitrogenLimitationFactorPerPlant(UTILS utils, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            double totalNitrogenSupply =
                community.allPlants.at(cohortindex)->totalSoilNitrogenUptake +
                community.allPlants.at(cohortindex)->rhizobiaNitrogenUptake +
                community.allPlants.at(cohortindex)->nitrogenSurplus;

            if (community.allPlants.at(cohortindex)->totalPlantNitrogenDemand > 0)
            {
                community.allPlants.at(cohortindex)->limitingFactorNppNitrogen = std::min(1.0, totalNitrogenSupply / community.allPlants.at(cohortindex)->totalPlantNitrogenDemand);
            }
            else
            {
                community.allPlants.at(cohortindex)->limitingFactorNppNitrogen = 1.0;
            }
        }
    }
}

/**
 * @brief Distributes the allocable nitrogen to per-organ uptake fields and
 *        updates the plant nitrogen surplus pool.
 *
 * `allocableNitrogen = limitingFactorNppNitrogen * totalNDemand`. Allocates
 * to shoot, root, recruitment, and exudate uptake fields using the previously
 * computed demand fractions. Calls `updateNitrogenSurplusPoolOfPlant()` to
 * adjust the surplus.
 *
 * @param utils     Utility object for error checking.
 * @param community Plant community; per-organ N uptake fields updated per cohort.
 */
void SOIL::allocateNitrogenUptakeToPlant(UTILS utils, COMMUNITY &community)
{
    double allocableNitrogen = 0.0;

    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            // nitrogen to be allocated: either the full plant's demand or less if soil uptake + rhizobia uptake + surplus is smaller than demand
            // note that uptake from rhizobia and surplus can be larger than demand (in this case no soil uptake occurs, but total uptake > demand)
            // therefore, nitrogen demand is not used here to calculate allocable nitrogen
            allocableNitrogen = community.allPlants.at(cohortindex)->limitingFactorNppNitrogen * community.allPlants.at(cohortindex)->totalPlantNitrogenDemand;

            if (allocableNitrogen > 0)
            {
                /* allocate to plant shoot */
                community.allPlants.at(cohortindex)->shootNitrogenUptakeForGreenLeaves = community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionShoot * allocableNitrogen;

                /* allocate to plant root */
                community.allPlants.at(cohortindex)->rootNitrogenUptake = community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionRoot * allocableNitrogen;

                /* allocate to plant reproduction */
                community.allPlants.at(cohortindex)->recruitmentNitrogenUptake = community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionReproduction * allocableNitrogen;

                /* allocate to plant exudation */
                community.allPlants.at(cohortindex)->exudationNitrogenUptake = community.allPlants.at(cohortindex)->nitrogenDemandGrowthFractionExudation * allocableNitrogen;

                /* update nitrogen surplus pool of plant */
                updateNitrogenSurplusPoolOfPlant(utils, community, cohortindex, allocableNitrogen);
            }
        }
    }
}

/**
 * @brief Updates the plant nitrogen surplus pool after allocation.
 *
 * If the surplus was positive, recalculates it as:
 * `surplus + soilUptake + rhizobiaUptake - allocatedN`.
 * Calls `utils.checkForNegativeValue()` to guard against numerical issues.
 *
 * @param utils           Utility object for negative-value checking.
 * @param community       Plant community; `nitrogenSurplus` updated for the cohort.
 * @param cohortindex     Index of the target cohort.
 * @param allocableNitrogen Total nitrogen allocated to growth this time step.
 */
void SOIL::updateNitrogenSurplusPoolOfPlant(UTILS utils, COMMUNITY &community, int cohortindex, double allocableNitrogen)
{
    // update nitrogenSurplus if it existed
    // surplus is usually generated from nitrogen relocation during senescence
    // if surplus + rhizobia uptake > demand, then soil uptake is zero, but surplus and rhizobia uptake have so far not been reduced (therefore, surplus is updated here with the remainings)
    // if surplus + rhizobia uptake + soil uptake < demand, then surplus is reduced (and should be zero here)
    if (community.allPlants.at(cohortindex)->nitrogenSurplus > 0)
    {
        community.allPlants.at(cohortindex)->nitrogenSurplus = community.allPlants.at(cohortindex)->nitrogenSurplus + community.allPlants.at(cohortindex)->totalSoilNitrogenUptake +
                                                               community.allPlants.at(cohortindex)->rhizobiaNitrogenUptake - allocableNitrogen;

        /* error checking of plant nitrogen surplus */
        utils.checkForNegativeValue(community.allPlants.at(cohortindex)->nitrogenSurplus, "Plant nitrogen surplus of cohort " + std::to_string(cohortindex));
    }
}

/**
 * @brief Accumulates per-cohort soil N uptake into community-level per-layer
 *        and total uptake accumulators.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides `numberOfSoilLayers`.
 * @param community Plant community; `totalSoilNitrogenUptakePerSoilLayer` and
 *                  `totalSoilNitrogenUptake` updated.
 */
void SOIL::summarizeTotalSoilNitrogenUptakeFromAllPlants(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    for (int cohortindex = 0; cohortindex < community.totalNumberOfCohortsInCommunity; cohortindex++)
    {
        if (community.allPlants.at(cohortindex)->amount > 0)
        {
            /* sum up total soil nitrogen uptake per soil layer */
            // this only includes N uptake from soil resources (surplus and rhizobia uptake is not considered here)
            for (int soilLayer = 0; soilLayer < community.allPlants.at(cohortindex)->numberOfSoilLayersRooting; soilLayer++)
            {
                community.totalSoilNitrogenUptakePerSoilLayer.at(soilLayer) += (community.allPlants.at(cohortindex)->amount * community.allPlants.at(cohortindex)->soilNitrogenUptakePerSoilLayer.at(soilLayer));
            }
        }
    }
    /* sum up total soil nitrogen uptake across all soil layers */
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        community.totalSoilNitrogenUptake += community.totalSoilNitrogenUptakePerSoilLayer.at(soilLayer);
    }
}

/**
 * @brief Deducts community N uptake from the soil mineral N pool per layer.
 *
 * @param utils     Utility object; calls `checkForNegativeValue()` per layer.
 * @param parameter Provides `numberOfSoilLayers`.
 * @param community Provides `totalSoilNitrogenUptakePerSoilLayer`.
 */
void SOIL::subtractPlantNitrogenUptakeFromSoilMineralNitrogenPool(UTILS utils, PARAMETER parameter, COMMUNITY &community)
{
    for (int soilLayer = 0; soilLayer < parameter.numberOfSoilLayers; soilLayer++)
    {
        /* subtract plant nitrogen uptake from soil mineral nitrogen pool */
        nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer) -= community.totalSoilNitrogenUptakePerSoilLayer.at(soilLayer);

        /* error checking of soil mineral nitrogen pool */
        utils.checkForNegativeValue(nitrogenContent_soilMineralPoolPerSoilLayer.at(soilLayer), "Soil mineral nitrogen pool at soil layer " + std::to_string(soilLayer));
    }
}

// ##################################################################################################
// carbon-nitrogen dynamics
// ##################################################################################################
/**
 * @brief Orchestrates all soil carbon and nitrogen decomposition dynamics.
 *
 * Steps in order:
 * 1. splitLitterFluxesToStructuralAndMetabolicLitterPools()
 * 2. calculateTemperatureAndWaterEffectsOnDecomposition() -> `decompositionFactor`
 * 3. doDecompositionFluxesInLitterAndSoilPools()
 * 4. updateSoilPoolsByRespirationAndFluxes()
 * 5. calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition()
 * 6. calculateNitrogenLossByVolatilization()
 *
 * @param utils       Utility object for error handling.
 * @param parameter   Provides soil-module flags and weather-day index.
 * @param interaction Provides soil temperature for the decomposition factor.
 * @param weather     Provides precipitation and temperature for litter splitting.
 *
 * @cite Adapted from the CENTURY 4.0 soil model.
 */
void SOIL::calculateSoilCarbonNitrogenDynamics(UTILS utils, PARAMETER parameter, INTERACTION interaction, WEATHER weather)
{
    splitLitterFluxesToStructuralAndMetabolicLitterPools(utils, parameter, weather);

    decompositionFactor = calculateTemperatureAndWaterEffectsOnDecomposition(utils, interaction, parameter);

    doDecompositionFluxesInLitterAndSoilPools(utils);

    updateSoilPoolsByRespirationAndFluxes(utils);

    // nonsymbiotic nitrogen fixation and atmospheric nitrogen deposition
    calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(utils, parameter, weather);

    // Volatilization loss of nitrogen as a function of gross mineralization
    calculateNitrogenLossByVolatilization(utils);
}

/**
 * @brief Routes surface and soil litter fluxes to structural and metabolic pools.
 *
 * Calls `addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools()`
 * for `"surface"` and `"soil"` pool types.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Provides simulation day for weather indexing.
 * @param weather   Provides precipitation for lignin fraction calculation.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::splitLitterFluxesToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather)
{
    addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(utils, parameter, weather, "surface");
    addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(utils, parameter, weather, "soil");
}

/**
 * @brief Splits a combined litter carbon/nitrogen flux into structural and
 *        metabolic fractions and updates the corresponding pools.
 *
 * Uses lignin fraction, C/N ratio, and a DIRABS (direct absorption) factor
 * to determine what fraction of C goes to the metabolic vs. structural pool.
 * Updates lignin content of structural litter and calls `processLitterFluxes()`
 * to transfer the C/N amounts to the appropriate pool variables.
 *
 * @param utils      Utility object for error handling.
 * @param parameter  Provides simulation day for weather indexing.
 * @param weather    Provides precipitation for lignin fraction calculation.
 * @param typeOfPool `"surface"` or `"soil"`.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::addCarbonNitrogenOfPlantLitterToStructuralAndMetabolicLitterPools(UTILS utils, PARAMETER parameter, WEATHER weather, std::string typeOfPool)
{
    double dirabs = 0;
    double fractionOfLignin, fractionOfNitrogen, ligninToNitrogenRatio;
    double fractionOfMetabolicLitter;
    double carbonAddedToMetabolicLitter, carbonAddedToStructuralLitter;
    double nitrogenAddedToMetabolicLitter, nitrogenAddedToStructuralLitter;
    double carbonFlux, nitrogenFlux;

    // define fluxes of litter pools
    if (typeOfPool == "surface")
    {
        carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
        nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
    }
    else if (typeOfPool == "soil")
    {
        carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
        nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
    }

    // split fluxes into metabolic and structural litter
    if (carbonFlux > 1E-13)
    {
        // ensure there is enough nitrogen
        dirabs = calculateDIRABS(utils, typeOfPool);

        // calculate lignin to nitrogen ratio
        fractionOfLignin = calculateLigninFraction(utils, weather, parameter, typeOfPool);
        fractionOfNitrogen = calculateNitrogenFraction(utils, dirabs, typeOfPool);
        ligninToNitrogenRatio = fractionOfLignin / fractionOfNitrogen;

        // Carbon added to metabolic and structural litter carbon pools
        fractionOfMetabolicLitter = calculateFractionOfMetabolicLitter(utils, fractionOfLignin, ligninToNitrogenRatio, typeOfPool);
        carbonAddedToMetabolicLitter = carbonFlux * fractionOfMetabolicLitter;
        carbonAddedToStructuralLitter = carbonFlux - carbonAddedToMetabolicLitter;

        // adjust lignin content
        if (typeOfPool == "surface")
        {
            ligninContent_surfaceStructuralLitterPool = adjustLigninContentOfStructuralLitter(utils, fractionOfLignin, carbonAddedToStructuralLitter, ligninContent_surfaceStructuralLitterPool, typeOfPool);
        }
        else if (typeOfPool == "soil")
        {
            ligninContent_soilStructuralLitterPool = adjustLigninContentOfStructuralLitter(utils, fractionOfLignin, carbonAddedToStructuralLitter, ligninContent_soilStructuralLitterPool, typeOfPool);
        }

        // Nitrogen added to metabolic and structural surface litter N pools
        nitrogenAddedToStructuralLitter = carbonAddedToStructuralLitter / 200.0;
        nitrogenAddedToMetabolicLitter = nitrogenFlux + dirabs - nitrogenAddedToStructuralLitter;

        processLitterFluxes(utils, dirabs, carbonAddedToStructuralLitter, carbonAddedToMetabolicLitter, nitrogenAddedToStructuralLitter, nitrogenAddedToMetabolicLitter, typeOfPool);
    }
}

/**
 * @brief Calculates the DIRABS factor: direct mineral N absorption from
 *        the soil pool to meet C/N requirements of structural litter.
 *
 * If the litter C/N ratio is below 15, mineral N is absorbed to raise the
 * N content. DIRABS is capped at 2 % of mineral N per unit litter carbon.
 *
 * @param utils      Utility object (reserved).
 * @param typeOfPool `"surface"` or `"soil"`; determines the damr coefficient.
 * @return DIRABS amount (g N).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
double SOIL::calculateDIRABS(UTILS utils, std::string typeOfPool)
{
    double rcetot = 0.0;
    double dirabs = 0.0;
    double carbonFlux = 0;
    double nitrogenFlux = 0;

    double damr;
    if (typeOfPool == "surface")
    {
        damr = 0.0; // TODO: parameter for testing
        carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
        nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
    }
    else if (typeOfPool == "soil")
    {
        damr = 0.02;
        carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
        nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
    }

    dirabs = damr * nitrogenContent_soilMineralPoolPerSoilLayer.at(0) * std::max(carbonFlux / 100.0, 1.);

    if ((nitrogenFlux + dirabs) > 0.0)
    {
        rcetot = carbonFlux / (nitrogenFlux + dirabs);
    }

    if (rcetot < 15.0)
    {
        dirabs = (carbonFlux / 15.0) - nitrogenFlux;
        if (dirabs < 0.0)
        {
            dirabs = 0.0;
        }
    }

    return (dirabs);
}

/**
 * @brief Computes the lignin fraction of incoming litter based on precipitation.
 *
 * Linearly scaled with daily precipitation; clamped to [0.02/365, 0.5/365].
 * Surface and soil litter use different intercept/slope parameters.
 *
 * @param utils      Utility object (reserved).
 * @param weather    Provides daily precipitation.
 * @param parameter  Provides simulation day for weather indexing.
 * @param typeOfPool `"surface"` or `"soil"`.
 * @return Lignin fraction (dimensionless, per day).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
double SOIL::calculateLigninFraction(UTILS utils, WEATHER weather, PARAMETER parameter, std::string typeOfPool)
{
    double ligninFraction;
    double param1, param2; // TODO: add as parameter

    if (typeOfPool == "surface")
    {
        param1 = 0.02 / 365.0;
        param2 = 0.0012;
    }
    else if (typeOfPool == "soil")
    {
        param1 = 0.26 / 365.0;
        param2 = 0.0015;
    }

    ligninFraction = param1 + (param2 * (weather.precipitation.at(parameter.day - 1) / 10.0));

    double lowerLimit = 0.02 / 365.0;
    double upperLimit = 0.5 / 365.0;
    ligninFraction = std::max(lowerLimit, ligninFraction);
    ligninFraction = std::min(upperLimit, ligninFraction);

    return (ligninFraction);
}

/**
 * @brief Computes the nitrogen fraction of the combined litter flux including DIRABS.
 *
 * @param utils      Utility object (reserved).
 * @param dirabs     Direct N absorption from soil computed by calculateDIRABS().
 * @param typeOfPool `"surface"` or `"soil"`.
 * @return Nitrogen fraction (g N / g C total litter).
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
double SOIL::calculateNitrogenFraction(UTILS utils, double dirabs, std::string typeOfPool)
{

    double fractionOfNitrogen;
    double carbonFlux, nitrogenFlux;

    if (typeOfPool == "surface")
    {
        carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
        nitrogenFlux = nitrogenContent_surfaceGreenLitterPool + nitrogenContent_surfaceBrownLitterPool;
    }
    else if (typeOfPool == "soil")
    {
        carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
        nitrogenFlux = nitrogenContent_soilRootLitterPool + nitrogenContent_soilSeedLitterPool;
    }

    fractionOfNitrogen = (nitrogenFlux + dirabs) / (carbonFlux / CARBON_CONTENT_ODM);
    return (fractionOfNitrogen);
}

/**
 * @brief Computes the metabolic litter fraction from lignin content and
 *        lignin-to-nitrogen ratio.
 *
 * `metabolicFraction = 0.85 - 0.018 * ligninToNitrogenRatio`, clamped to [0, 1].
 *
 * @param utils                 Utility object (reserved).
 * @param fractionOfLignin      Lignin fraction of litter.
 * @param ligninToNitrogenRatio Lignin fraction / nitrogen fraction.
 * @param type                  `"surface"` or `"soil"`.
 * @return Metabolic litter fraction (0–1).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
double SOIL::calculateFractionOfMetabolicLitter(UTILS utils, double fractionOfLignin, double ligninToNitrogenRatio, std::string type)
{
    double fractionOfMetabolicLitter;

    fractionOfMetabolicLitter = 0.85 - 0.013 * ligninToNitrogenRatio;

    if (fractionOfLignin > (1.0 - fractionOfMetabolicLitter))
    {
        fractionOfMetabolicLitter = (1.0 - fractionOfLignin);
    }

    // Make sure at least 1% goes to metabolic
    if (fractionOfMetabolicLitter < 0.20)
    {
        fractionOfMetabolicLitter = 0.20;
    }

    if (fractionOfMetabolicLitter < 0.0)
    {
        utils.handleError("Fraction of added carbon to metabolic litter pool is negative.");
    }

    return (fractionOfMetabolicLitter);
}

/**
 * @brief Adjusts the lignin content of the structural litter pool after
 *        adding new litter.
 *
 * New structural lignin = added structural C x ligninFraction; added to the
 * existing pool value.
 *
 * @param utils                        Utility object (reserved).
 * @param fractionOfLignin             Lignin fraction of newly added litter.
 * @param carbonAddedToStructuralLitter Carbon added to structural pool (g C).
 * @param strlig                       Current lignin content of the pool (g).
 * @param typeOfPool                   `"surface"` or `"soil"`.
 * @return Updated lignin content (g).
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
double SOIL::adjustLigninContentOfStructuralLitter(UTILS utils, double fractionOfLignin, double carbonAddedToStructuralLitter, double strlig, std::string typeOfPool)
{
    double adjustedFractionOfLignin;
    double carbonFlux, carbonPool;
    double ligninContent;

    if (typeOfPool == "surface")
    {
        carbonFlux = carbonContent_surfaceGreenLitterPool + carbonContent_surfaceBrownLitterPool;
        carbonPool = carbonContent_surfaceStructuralLitterPool;
        ligninContent = ligninContent_surfaceStructuralLitterPool;
    }
    else if (typeOfPool == "soil")
    {
        carbonFlux = carbonContent_soilRootLitterPool + carbonContent_soilSeedLitterPool;
        carbonPool = carbonContent_soilStructuralLitterPool;
        ligninContent = ligninContent_soilStructuralLitterPool;
    }

    adjustedFractionOfLignin = fractionOfLignin / (carbonAddedToStructuralLitter / carbonFlux);
    if (adjustedFractionOfLignin > 1.0)
        adjustedFractionOfLignin = 1.0;

    double previousLigninContent = ligninContent * carbonPool;
    double newLigninContent = adjustedFractionOfLignin * carbonAddedToStructuralLitter;
    ligninContent = (previousLigninContent + newLigninContent) / (carbonPool + carbonAddedToStructuralLitter);

    return (ligninContent);
}

/**
 * @brief Applies the computed structural/metabolic C and N flux amounts to the
 *        appropriate soil litter pool variables and resets the daily input pools.
 *
 * Increments structural and metabolic C and N pool contents and resets the
 * surface or soil green/brown/root/seed input pools to zero.
 *
 * @param utils                          Utility object (reserved).
 * @param dirabs                         Direct mineral N absorption (g N).
 * @param carbonAddedToStructuralLitter  C added to structural pool (g C).
 * @param carbonAddedToMetabolicLitter   C added to metabolic pool (g C).
 * @param nitrogenAddedToStructuralLitter N added to structural pool (g N).
 * @param nitrogenAddedToMetabolicLitter  N added to metabolic pool (g N).
 * @param typeOfPool                     `"surface"` or `"soil"`.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::processLitterFluxes(UTILS utils, double dirabs, double carbonAddedToStructuralLitter, double carbonAddedToMetabolicLitter, double nitrogenAddedToStructuralLitter, double nitrogenAddedToMetabolicLitter, std::string typeOfPool)
{
    if (typeOfPool == "surface")
    {
        carbonContent_surfaceStructuralLitterPool += carbonAddedToStructuralLitter;
        carbonContent_surfaceMetabolicLitterPool += carbonAddedToMetabolicLitter;
        nitrogenContent_surfaceStructuralLitterPool += nitrogenAddedToStructuralLitter;
        nitrogenContent_surfaceMetabolicLitterPool += nitrogenAddedToMetabolicLitter;
    }
    else if (typeOfPool == "soil")
    {
        carbonContent_soilStructuralLitterPool += carbonAddedToStructuralLitter;
        carbonContent_soilMetabolicLitterPool += carbonAddedToMetabolicLitter;
        nitrogenContent_soilStructuralLitterPool += nitrogenAddedToStructuralLitter;
        nitrogenContent_soilMetabolicLitterPool += nitrogenAddedToMetabolicLitter;
    }

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -= dirabs;
    if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + TOLERANCE < 0.0)
    {
        nitrogenContent_soilMineralPoolPerSoilLayer.at(0) = 0;
        // utils.handleError("Plants do not have access to soil nitrogen or soil nitrogen pool is negative!");
    }
}

/**
 * @brief Triggers decomposition for each active litter and soil C pool.
 *
 * Calls `startDecomposition()` for each pool if it is `decomposable()`,
 * using pool-specific rate constants and lignin constraints.
 *
 * @param utils Utility object for error handling inside decomposition steps.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::doDecompositionFluxesInLitterAndSoilPools(UTILS utils)
{
    startDecomposition(utils, carbonContent_surfaceStructuralLitterPool, 3.9 / 365.0, ligninContent_surfaceStructuralLitterPool, "surface_structural");
    startDecomposition(utils, carbonContent_soilStructuralLitterPool, 4.9 / 365.0, ligninContent_soilStructuralLitterPool, "soil_structural");
    startDecomposition(utils, carbonContent_surfaceMetabolicLitterPool, 14.8 / 365.0, 1, "surface_metabolic");
    startDecomposition(utils, carbonContent_soilMetabolicLitterPool, 18.5 / 365.0, 1, "soil_metabolic");
    startDecomposition(utils, carbonContent_soilMicrobesPool, 6.0 / 365.0, 1, "microbes");
    startDecomposition(utils, carbonContent_soilActivePool, 7.3 / 365.0, 1, "active");
    startDecomposition(utils, carbonContent_soilSlowPool, 0.2 / 365.0, 1, "slow");
    startDecomposition(utils, carbonContent_soilPassivePool, 0.0045 / 365.0, 1, "passive");
}

/**
 * @brief Initiates the decomposition flux from one pool, routing C and N to
 *        the destination pool(s) according to the pool-specific transfer scheme.
 *
 * Calls `decomposable()` to check preconditions, then `decompose()` to compute
 * the flux. The actual C/N transfer, respiratory losses, and N mineralisation/
 * immobilisation are handled inside `decompose()`.
 *
 * @param utils            Utility object for error handling.
 * @param carbonContentOfPool C content of the source pool (g C).
 * @param constFactor      Pool-specific rate constant (d\u207b\u00b9 at reference conditions).
 * @param lignin           Lignin content of the pool (affects decomposition rate).
 * @param transferFromPool Source pool identifier string (e.g. `"surface_structural"`).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::startDecomposition(UTILS utils, double carbonContentOfPool, double constFactor, double lignin, std::string transferFromPool)
{
    const double maximumCarbonFlux = 5000.0; // in [g/m²/day]
    double potentialCarbonFlux, actualCarbonFlux;
    double textureFactor = 1.0;

    if (carbonContentOfPool > 0)
    {
        potentialCarbonFlux = carbonContentOfPool;

        // modify the potential carbon flux
        if (transferFromPool == "surface_structural" || transferFromPool == "soil_structural")
        {
            potentialCarbonFlux = std::min(carbonContentOfPool, maximumCarbonFlux); // TODO: why this upper limit?
            textureFactor = exp(-3.0 * lignin);                                     // factor between 0 and 1
        }

        if (transferFromPool == "active")
        {
            textureFactor = 0.25 + 0.75 * sandContent; // factor between 0 and 1
        }

        // calculate the actual carbon flux
        actualCarbonFlux = potentialCarbonFlux * decompositionFactor * constFactor * textureFactor;

        // calculate decisive CN ratios to decide on decomposition, nitrogen immobilization or mineralization
        calculateDecisiveCarbonNitrogenRatiosForDecomposition(utils, transferFromPool);

        // transfer carbon and nitrogen from one pool to another
        decompose(utils, actualCarbonFlux, lignin, transferFromPool);
    }
}

/**
 * @brief Precomputes the decisive C/N ratios used to decide between
 *        mineralisation and immobilisation for a given pool-to-pool transfer.
 *
 * The decisive C/N ratio determines the N demand of the receiving microbial
 * biomass. If the supply pool's C/N is above this ratio, N must be immobilised
 * from the mineral pool; if below, net N is mineralised.
 *
 * @param utils           Utility object for error handling.
 * @param transferFromPool Source pool identifier string.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::calculateDecisiveCarbonNitrogenRatiosForDecomposition(UTILS utils, std::string transferFromPool)
{
    // decisive CN ratios depend on daily dynamically changing state variables (e.g. mineral soil nitrogen content or carbon content of pool)
    double mineralNitrogenContentTopSoilLayer = nitrogenContent_soilMineralPoolPerSoilLayer.at(0);
    double factorMineralNitrogenTopSoilLayer = (1.0 - (mineralNitrogenContentTopSoilLayer / 2.0));

    if (transferFromPool == "surface_structural")
    {
        double biomassContent = carbonContent_surfaceStructuralLitterPool / CARBON_CONTENT_ODM;
        double nitrogenContentInBiomass = (biomassContent > 0) ? (nitrogenContent_surfaceStructuralLitterPool / biomassContent) : 0;

        // for decomposition of surface structural litter to soil microbes pool
        decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool = std::min(16.0 + nitrogenContentInBiomass * ((10.0 - 16.0) / 0.02), 10.0);

        // for decomposition of surface structural litter to soil slow pool
        double auxillaryVariable = decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool + (12.0 + 3.0 * (decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool - 10.0));
        decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool = std::max(5.0, auxillaryVariable);
    }
    else if (transferFromPool == "soil_structural")
    {
        // transfer to soil active pool
        decisiveCNRatio_soilStructuralLitterPool_soilActivePool = 14.0;

        // transfer to soil slow pool
        decisiveCNRatio_soilStructuralLitterPool_soilSlowPool = 20.0;
    }
    else if (transferFromPool == "surface_metabolic")
    {
        // transfer to soil microbes pool
        // decisive carbon nitrogen ratio is dependent on nitrogen content (% of biomass) of surface metabolic litter pool
        double biomassContent_surfaceMetabolicLitterPool = carbonContent_surfaceMetabolicLitterPool / CARBON_CONTENT_ODM;
        double nitrogenContentInBiomass = (biomassContent_surfaceMetabolicLitterPool > 0) ? (nitrogenContent_surfaceMetabolicLitterPool / biomassContent_surfaceMetabolicLitterPool) : 0;
        decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool = std::min(16.0 + nitrogenContentInBiomass * ((10.0 - 16.0) / 0.02), 10.0);
    }
    else if (transferFromPool == "soil_metabolic")
    {
        // transfer to soil active pool
        // decisive carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = 14.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = 3.0;
        }
        else
        {
            decisiveCNRatio_soilMetabolicLitterPool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
        }
    }
    else if (transferFromPool == "microbes")
    {
        // transfer to soil slow pool
        // decisive carbon nitrogen ratio is dependent on cnRatio of contents in the pool and modifications
        double actualCNRatio_soilMicrobesPool = (carbonContent_soilMicrobesPool / nitrogenContent_soilMicrobesPool);
        double auxillaryVariable = actualCNRatio_soilMicrobesPool + 12.0 + 3.0 * (actualCNRatio_soilMicrobesPool - 10.0);
        decisiveCNRatio_soilMicrobesPool_soilSlowPool = std::max(auxillaryVariable, 5.0);
    }
    else if (transferFromPool == "active")
    {
        // transfer to soil slow pool
        // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilActivePool_soilSlowPool = 20.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilActivePool_soilSlowPool = 12.0;
        }
        else
        {
            decisiveCNRatio_soilActivePool_soilSlowPool = factorMineralNitrogenTopSoilLayer * (20.0 - 12.0) + 12.0;
        }

        // transfer from soil active to soil passive pool
        // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilActivePool_soilPassivePool = 8.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilActivePool_soilPassivePool = 6.0;
        }
        else
        {
            decisiveCNRatio_soilActivePool_soilPassivePool = factorMineralNitrogenTopSoilLayer * (8.0 - 6.0) + 6.0;
        }
    }
    else if (transferFromPool == "slow")
    {
        // transfer to soil active pool
        // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilSlowPool_soilActivePool = 14.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilSlowPool_soilActivePool = 3.0;
        }
        else
        {
            decisiveCNRatio_soilSlowPool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
        }

        // transfer to soil passive pool
        // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilSlowPool_soilPassivePool = 8.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilSlowPool_soilPassivePool = 6.0;
        }
        else
        {
            decisiveCNRatio_soilSlowPool_soilPassivePool = factorMineralNitrogenTopSoilLayer * (8.0 - 6.0) + 6.0;
        }
    }
    else if (transferFromPool == "passive")
    {
        // transfer to soil active pool
        // carbon nitrogen ratio is dependent on mineral nitrogen content in top soil layer
        if (mineralNitrogenContentTopSoilLayer <= 0)
        {
            decisiveCNRatio_soilPassivePool_soilActivePool = 14.0;
        }
        else if (mineralNitrogenContentTopSoilLayer > 2.0)
        {
            decisiveCNRatio_soilPassivePool_soilActivePool = 3.0;
        }
        else
        {
            decisiveCNRatio_soilPassivePool_soilActivePool = factorMineralNitrogenTopSoilLayer * (14.0 - 3.0) + 3.0;
        }
    }
    else
    {
        utils.handleError("No correct type of pool provided.");
    }
}

/**
 * @brief Computes the temperature and water effects on decomposition rates
 *        and returns a combined scalar factor.
 *
 * Temperature effect: exponential Q10 function based on soil temperature.
 * Water effect: linear scaling from a lower threshold to field capacity.
 * The two effects are multiplied.
 *
 * @param utils       Utility object (reserved).
 * @param interaction Provides `soilTemperature`.
 * @param parameter   Provides `numberOfSoilLayers` and soil-module flags.
 * @return Combined decomposition factor (dimensionless, stored in
 *         `decompositionFactor`).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
double SOIL::calculateTemperatureAndWaterEffectsOnDecomposition(UTILS utils, INTERACTION interaction, PARAMETER parameter)
{
    // TODO: move somewhere else??
    if (solidSnowContent > 0.0)
    {
        interaction.soilTemperature = 0.0;
    }

    double temperatureFunction = (((atan(((interaction.soilTemperature - 15.4) + (2 * PI)) / (0.031 * 11.75 * 29.7))) + atan(0.031 * 29.7 * PI)) /
                                  (2 * atan(0.031 * 29.7 * PI)));

    double relativeWaterContent = (waterContent_soilWaterPoolPerSoilLayer.at(0) - permanentWiltingPoint.at(0)) / (fieldCapacity.at(0) - permanentWiltingPoint.at(0));
    double waterFunction = (relativeWaterContent > 13.0) ? (1.0) : (1.0 / (1.0 + 4.0 * exp(-6.0 * relativeWaterContent)));
    waterFunction = std::min(waterFunction, 1.0);

    double factor = temperatureFunction * waterFunction;
    factor = std::max(factor, 0.0);

    return (factor);
}

/**
 * @brief Checks whether a pool is eligible for decomposition this time step.
 *
 * Returns `true` if the pool's C content is above a minimum threshold and
 * other preconditions (e.g. positive decomposition factor) are met.
 *
 * @param utils      Utility object (reserved).
 * @param typeOfPool Pool identifier string.
 * @return `true` if decomposition should proceed.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
bool SOIL::decomposable(UTILS utils, std::string typeOfPool)
{
    bool doDecomposition = true;

    // if there is not enough mineral nitrogen in the top soil layer AND the carbon-nitrogen ratio of a pool is exceeding an upper ratio limit, no decomposition is possible due to limited nitrogen resources
    if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) < 1e-07) // in [g/m²]
    {
        if (typeOfPool == "surface_structural")
        {
            if ((carbonContent_surfaceStructuralLitterPool / nitrogenContent_surfaceStructuralLitterPool) > decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "soil_structural")
        {
            if ((carbonContent_soilStructuralLitterPool / nitrogenContent_soilStructuralLitterPool) > decisiveCNRatio_soilStructuralLitterPool_soilActivePool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "surface_metabolic")
        {
            if ((carbonContent_surfaceMetabolicLitterPool / nitrogenContent_surfaceMetabolicLitterPool) > decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "soil_metabolic")
        {
            if ((carbonContent_soilMetabolicLitterPool / nitrogenContent_soilMetabolicLitterPool) > decisiveCNRatio_soilMetabolicLitterPool_soilActivePool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "microbes")
        {
            if ((carbonContent_soilMicrobesPool / nitrogenContent_soilMicrobesPool) > decisiveCNRatio_soilMicrobesPool_soilSlowPool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "active")
        {
            // soil active pool is decomposed to slow and passive pool
            // here, for the decision, the decisive CN ratio to the slow pool is used
            if ((carbonContent_soilActivePool / nitrogenContent_soilActivePool) > decisiveCNRatio_soilActivePool_soilSlowPool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "slow")
        {
            // soil slow pool is decomposed to active and passive pool
            // here, for the decision, the decisive CN ratio to the active pool is used
            if ((carbonContent_soilSlowPool / nitrogenContent_soilSlowPool) > decisiveCNRatio_soilSlowPool_soilActivePool)
            {
                doDecomposition = false;
            }
        }
        else if (typeOfPool == "passive")
        {
            if ((carbonContent_soilPassivePool / nitrogenContent_soilPassivePool) > decisiveCNRatio_soilPassivePool_soilActivePool)
            {
                doDecomposition = false;
            }
        }
    }

    return (doDecomposition);
}

/**
 * @brief Computes the decomposition flux from one pool, calculates C and N
 *        respiratory losses, determines N flow direction, and updates flux
 *        tracking variables.
 *
 * Uses the decomposition factor, rate constant, and lignin content to compute
 * how much C decomposes. Delegates to `calculateCarbonRespirationOfDecomposition()`,
 * `calculateNitrogenRespirationOfDecomposition()`, and `determineNitrogenFlux()`.
 *
 * @param utils           Utility object for error handling.
 * @param carbonFlux      C available for decomposition (g C).
 * @param ligninContent   Lignin content of the decomposing material.
 * @param transferFromPool Source pool identifier string.
 * @return `true` if decomposition occurred; `false` otherwise.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
bool SOIL::decompose(UTILS utils, double carbonFlux, double ligninContent, std::string transferFromPool)
{
    double compareRatio = 1;
    std::string transferToPool;
    double nitrogenContentTopSoilLayer = nitrogenContent_soilMineralPoolPerSoilLayer.at(0);

    if (decomposable(utils, transferFromPool))
    {
        if (transferFromPool == "surface_structural")
        {
            // ******* to soil slow pool ********
            transferToPool = "slow";

            // determine carbon flux
            carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool = carbonFlux * ligninContent;

            // subtract carbon respiration from carbon flux
            // calculate respiratory nitrogen flow proportional to carbon respiration (based on actual CN ratio of origin pool)
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // determine nitrogen flux proportional to remaining carbon flux (based on actual CN ratio of origin pool)
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // save nitrogen flux in extra state variable for storage (as nitrogenFlux will be changed in case of mineralization)
            nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool = nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;
            // decision based on actual CN ratio of fluxes compared to decisiveCNRatio
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool,
                                           nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool, decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool,
                                           transferFromPool, transferToPool);

            // ******  to mirobial soil pool *******
            transferToPool = "microbes";
            // respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool has already been subtracted from carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool in previous call of calculateRespirationOfDecomposition
            // therefore, it needs to be accounted here to calculate the remaining carbon flux transferred to the microbes pool
            carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = carbonFlux - (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool);
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from litter to soil microbial pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool = nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool,
                                           nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool, decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool,
                                           transferFromPool, transferToPool);
        }

        // -----------------------------------------------------
        if (transferFromPool == "soil_structural")
        {
            //*** to slow soil pool ****
            transferToPool = "slow";
            carbonFlux_soilStructuralLitterPool_to_soilSlowPool = carbonFlux * ligninContent;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from litter to soil slow pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool = nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilStructuralLitterPool_to_soilSlowPool,
                                           nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool,
                                           decisiveCNRatio_soilStructuralLitterPool_soilSlowPool, transferFromPool, transferToPool);

            // **** to active soil pool ****
            transferToPool = "active";
            carbonFlux_soilStructuralLitterPool_to_soilActivePool =
                carbonFlux - carbonFlux_soilStructuralLitterPool_to_soilSlowPool - respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from litter to soil active pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilStructuralLitterPool_to_soilActivePool = nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilStructuralLitterPool_to_soilActivePool,
                                           nitrogenFlow_soilStructuralLitterPool_to_soilActivePool,
                                           decisiveCNRatio_soilStructuralLitterPool_soilActivePool, transferFromPool, transferToPool);
        }

        // ---------------------------------------------------------
        if (transferFromPool == "surface_metabolic")
        {
            transferToPool = "microbes";
            carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = std::min(carbonFlux, carbonContent_surfaceMetabolicLitterPool);
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from litter to microbial pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool = nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool,
                                           nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool,
                                           decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool, transferFromPool, transferToPool);
        }

        // ------------------------------------------------
        if (transferFromPool == "soil_metabolic")
        {
            transferToPool = "active";
            carbonFlux_soilMetabolicLitterPool_to_soilActivePool = std::min(carbonFlux, carbonContent_soilMetabolicLitterPool);
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from litter to soil active pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool = nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilMetabolicLitterPool_to_soilActivePool,
                                           nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool,
                                           decisiveCNRatio_soilMetabolicLitterPool_soilActivePool, transferFromPool, transferToPool);
        }

        // -------------------------------------------
        if (transferFromPool == "microbes")
        {
            transferToPool = "slow";
            carbonFlux_soilMicrobesPool_to_soilSlowPool = carbonFlux;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from microbes to soil slow pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilMicrobesPool_to_soilSlowPool = nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilMicrobesPool_to_soilSlowPool,
                                           nitrogenFlow_soilMicrobesPool_to_soilSlowPool,
                                           decisiveCNRatio_soilMicrobesPool_soilSlowPool, transferFromPool, transferToPool);
        }

        // -------------------------------------------
        if (transferFromPool == "active")
        {
            transferToPool = "slow_passive";
            carbonFlux_soilActivePool_to_soilPassiveAndSlowPool = carbonFlux;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // -------------- flux to passive soil pool incl. leaching
            transferToPool = "passive";
            carbonFlux_soilActivePool_to_soilPassivePool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool * (0.003 + 0.032 * clayContent);

            // proportional nitrogen flow from active to soil passive pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilActivePool_to_soilPassivePool = nitrogenFlux_soilActivePool_to_soilPassivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilActivePool_to_soilPassivePool,
                                           nitrogenFlow_soilActivePool_to_soilPassivePool,
                                           decisiveCNRatio_soilActivePool_soilPassivePool, transferFromPool, transferToPool);

            // leaching
            doLeaching(utils);

            // -------------- flux to slow soil pool
            transferToPool = "slow";
            carbonFlux_soilActivePool_to_soilSlowPool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool - respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool -
                                                        carbonFlux_soilActivePool_to_soilPassivePool - leachingCarbon;

            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilActivePool_to_soilSlowPool = nitrogenFlux_soilActivePool_to_soilSlowPool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilActivePool_to_soilSlowPool,
                                           nitrogenFlow_soilActivePool_to_soilSlowPool,
                                           decisiveCNRatio_soilActivePool_soilSlowPool, transferFromPool, transferToPool);
        }

        // -------------------------------------------
        if (transferFromPool == "slow")
        {
            transferToPool = "active_passive";
            carbonFlux_soilSlowPool_to_soilPassiveAndActivePool = carbonFlux;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // --------- to passive pool
            transferToPool = "passive";
            carbonFlux_soilSlowPool_to_soilPassivePool = carbonFlux_soilSlowPool_to_soilPassiveAndActivePool * (0.003 + 0.009 * clayContent);

            // proportional nitrogen flow from soil slow pool to passive pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilSlowPool_to_soilPassivePool = nitrogenFlux_soilSlowPool_to_soilPassivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilSlowPool_to_soilPassivePool,
                                           nitrogenFlow_soilSlowPool_to_soilPassivePool,
                                           decisiveCNRatio_soilSlowPool_soilPassivePool, transferFromPool, transferToPool);

            // --------- to active pool
            // proportional nitrogen flow from soil slow pool to active pool
            transferToPool = "active";
            carbonFlux_soilSlowPool_to_soilActivePool =
                carbonFlux_soilSlowPool_to_soilPassiveAndActivePool - respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool - carbonFlux_soilSlowPool_to_soilPassivePool;

            determineNitrogenFlux(utils, transferFromPool, transferToPool);
            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilSlowPool_to_soilActivePool = nitrogenFlux_soilSlowPool_to_soilActivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilSlowPool_to_soilActivePool,
                                           nitrogenFlow_soilSlowPool_to_soilActivePool,
                                           decisiveCNRatio_soilSlowPool_soilActivePool, transferFromPool, transferToPool);
        }

        // -------------------------------------------
        if (transferFromPool == "passive")
        {
            transferToPool = "active";
            carbonFlux_soilPassivePool_to_soilActivePool = carbonFlux;
            calculateCarbonRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            // proportional nitrogen flow from soil passive pool to active pool
            determineNitrogenFlux(utils, transferFromPool, transferToPool);

            calculateNitrogenRespirationOfDecomposition(utils, transferFromPool, transferToPool);

            nitrogenFlow_soilPassivePool_to_soilActivePool = nitrogenFlux_soilPassivePool_to_soilActivePool;
            immobilizeOrMineralizeNitrogen(utils, carbonFlux_soilPassivePool_to_soilActivePool,
                                           nitrogenFlow_soilPassivePool_to_soilActivePool,
                                           decisiveCNRatio_soilPassivePool_soilActivePool, transferFromPool, transferToPool);
        }
    }
    return false;
}

/**
 * @brief Computes the carbon respiratory loss from a pool-to-pool transfer
 *        and stores it in the corresponding tracking variable.
 *
 * @param utils           Utility object (reserved).
 * @param transferFromPool Source pool identifier string.
 * @param transferToPool   Destination pool identifier string.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::calculateCarbonRespirationOfDecomposition(UTILS utils, std::string transferFromPool, std::string transferToPool)
{
    // respiratory carbon flow for the transfer from active or slow soil pools is calculated jointly for both fluxes (to slow & passive, to active and passive)
    if (transferFromPool == "surface_structural")
    {
        if (transferToPool == "slow")
        {
            respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool = carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool * 0.3;
            carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool -= respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool;
        }
        else if (transferToPool == "microbes")
        {
            respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool = carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool * 0.45;
            carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool -= respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_structural")
    {
        if (transferToPool == "slow")
        {
            respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool = carbonFlux_soilStructuralLitterPool_to_soilSlowPool * 0.3;
            carbonFlux_soilStructuralLitterPool_to_soilSlowPool -= respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool;
        }
        else if (transferToPool == "active")
        {
            respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool = carbonFlux_soilStructuralLitterPool_to_soilActivePool * 0.55;
            carbonFlux_soilStructuralLitterPool_to_soilActivePool -= respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool;
        }
    }
    else if (transferFromPool == "surface_metabolic")
    {
        if (transferToPool == "microbes")
        {
            respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool = carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool * 0.55;
            carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool -= respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_metabolic")
    {
        if (transferToPool == "active")
        {
            respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool = carbonFlux_soilMetabolicLitterPool_to_soilActivePool * 0.55;
            carbonFlux_soilMetabolicLitterPool_to_soilActivePool -= respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;
        }
    }
    else if (transferFromPool == "microbes")
    {
        if (transferToPool == "slow")
        {

            respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool = carbonFlux_soilMicrobesPool_to_soilSlowPool * 0.6;
            carbonFlux_soilMicrobesPool_to_soilSlowPool -= respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;
        }
    }
    else if (transferFromPool == "active")
    {
        if (transferToPool == "slow_passive")
        {
            respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool * (0.17 + 0.68 * sandContent);
        }
    }
    else if (transferFromPool == "slow")
    {
        if (transferToPool == "active_passive")
        {
            respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool = carbonFlux_soilSlowPool_to_soilPassiveAndActivePool * 0.55;
        }
    }
    else if (transferFromPool == "passive")
    {
        if (transferToPool == "active")
        {
            respiration_decompositionCarbon_soilPassivePool_soilActivePool = carbonFlux_soilPassivePool_to_soilActivePool * 0.55;
            carbonFlux_soilPassivePool_to_soilActivePool -= respiration_decompositionCarbon_soilPassivePool_soilActivePool;
        }
    }
}

/**
 * @brief Computes the nitrogen respiratory loss associated with a C-pool
 *        decomposition transfer and updates the corresponding N respiration variable.
 *
 * @param utils            Utility object (reserved).
 * @param transferPoolFrom Source pool identifier string.
 * @param transferPoolTo   Destination pool identifier string.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::calculateNitrogenRespirationOfDecomposition(UTILS utils, std::string transferPoolFrom, std::string transferPoolTo)
{
    // respiratory nitrogen flow for the transfer from active or slow soil pools is calculated jointly for both fluxes (to slow & passive, to active and passive)
    double actualCNRatioOfPool = 1;
    double respirationNitrogen = 0.0;

    if (transferPoolFrom == "surface_structural")
    {
        if (carbonContent_surfaceStructuralLitterPool > 0)
        {
            actualCNRatioOfPool = (nitrogenContent_surfaceStructuralLitterPool / carbonContent_surfaceStructuralLitterPool);

            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool = respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool;
            }
            else if (transferPoolTo == "microbes")
            {
                respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool = respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool;
            }
        }
        else
        {
            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool = 0;
                respirationNitrogen = 0;
            }
            else if (transferPoolTo == "microbes")
            {
                respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "soil_structural")
    {
        if (carbonContent_soilStructuralLitterPool > 0)
        {
            actualCNRatioOfPool = (nitrogenContent_soilStructuralLitterPool / carbonContent_soilStructuralLitterPool);

            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool = respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool;
            }
            else if (transferPoolTo == "active")
            {
                respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool = respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool;
            }
        }
        else
        {
            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool = 0;
                respirationNitrogen = 0;
            }
            else if (transferPoolTo == "active")
            {
                respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "surface_metabolic")
    {
        if (carbonContent_surfaceMetabolicLitterPool > 0)
        {
            actualCNRatioOfPool = (nitrogenContent_surfaceMetabolicLitterPool / carbonContent_surfaceMetabolicLitterPool);

            if (transferPoolTo == "microbes")
            {
                respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool = respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;
            }
        }
        else
        {
            if (transferPoolTo == "microbes")
            {
                respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "soil_metabolic")
    {
        if (carbonContent_soilMetabolicLitterPool > 0)
        {
            actualCNRatioOfPool = (nitrogenContent_soilMetabolicLitterPool / carbonContent_soilMetabolicLitterPool);

            if (transferPoolTo == "active")
            {
                respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool = respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;
            }
        }
        else
        {
            if (transferPoolTo == "active")
            {
                respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "microbes")
    {
        if (carbonContent_soilMicrobesPool > 0)
        {
            actualCNRatioOfPool = (nitrogenContent_soilMicrobesPool / carbonContent_soilMicrobesPool);

            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool = respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;
            }
        }
        else
        {
            if (transferPoolTo == "slow")
            {
                respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "active")
    {
        if (transferPoolTo == "slow_passive")
        {
            if (carbonContent_soilActivePool > 0)
            {
                actualCNRatioOfPool = (nitrogenContent_soilActivePool / carbonContent_soilActivePool);

                respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool = respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;
            }
            else
            {
                respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "slow")
    {
        if (transferPoolTo == "active_passive")
        {
            if (carbonContent_soilSlowPool > 0)
            {
                actualCNRatioOfPool = (nitrogenContent_soilSlowPool / carbonContent_soilSlowPool);

                respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool = respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;
            }
            else
            {
                respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool = 0;
                respirationNitrogen = 0;
            }
        }
    }
    else if (transferPoolFrom == "passive")
    {
        if (transferPoolTo == "active")
        {
            if (carbonContent_soilPassivePool > 0)
            {
                actualCNRatioOfPool = (nitrogenContent_soilPassivePool / carbonContent_soilPassivePool);

                respiration_decompositionNitrogen_soilPassivePool_soilActivePool = respiration_decompositionCarbon_soilPassivePool_soilActivePool * actualCNRatioOfPool;
                respirationNitrogen = respiration_decompositionNitrogen_soilPassivePool_soilActivePool;
            }
            else
            {
                respiration_decompositionNitrogen_soilPassivePool_soilActivePool = 0;
                respirationNitrogen = 0;
            }
        }
    }

    // added to mineralization rates as respiratory nitrogen fluxes will be added to soil mineral nitrogen pool later
    nitrogenGrossMineralization += respirationNitrogen;
    nitrogenNetMineralization += respirationNitrogen;
}

/**
 * @brief Determines the direction and amount of net nitrogen flow associated
 *        with a decomposition transfer and routes to
 *        `immobilizeOrMineralizeNitrogen()`.
 *
 * @param utils           Utility object for error handling.
 * @param transferFromPool Source pool identifier string.
 * @param transferToPool   Destination pool identifier string.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::determineNitrogenFlux(UTILS utils, std::string transferFromPool, std::string transferToPool)
{
    if (transferFromPool == "surface_structural")
    {
        if (transferToPool == "microbes")
        {
            if (carbonContent_surfaceStructuralLitterPool > 0.0)
            {
                nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = nitrogenContent_surfaceStructuralLitterPool * (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / carbonContent_surfaceStructuralLitterPool);
            }
            else
            {
                nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = 0.0;
            }
        }
        if (transferToPool == "slow")
        {
            if (carbonContent_surfaceStructuralLitterPool > 0.0)
            {
                nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = nitrogenContent_surfaceStructuralLitterPool * (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / carbonContent_surfaceStructuralLitterPool);
            }
            else
            {
                nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = 0.0;
            }
        }
    }
    else if (transferFromPool == "soil_structural")
    {
        if (transferToPool == "active")
        {
            if (carbonContent_soilStructuralLitterPool > 0.0)
            {
                nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = nitrogenContent_soilStructuralLitterPool * (carbonFlux_soilStructuralLitterPool_to_soilActivePool / carbonContent_soilStructuralLitterPool);
            }
            else
            {
                nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = 0.0;
            }
        }
        if (transferToPool == "slow")
        {
            if (carbonContent_soilStructuralLitterPool > 0.0)
            {
                nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = nitrogenContent_soilStructuralLitterPool * (carbonFlux_soilStructuralLitterPool_to_soilSlowPool / carbonContent_soilStructuralLitterPool);
            }
            else
            {
                nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = 0.0;
            }
        }
    }
    else if (transferFromPool == "surface_metabolic")
    {
        if (transferToPool == "microbes")
        {
            if (carbonContent_surfaceMetabolicLitterPool > 0.0)
            {
                nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = nitrogenContent_surfaceMetabolicLitterPool * (carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / carbonContent_surfaceMetabolicLitterPool);
            }
            else
            {
                nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = 0.0;
            }
        }
    }
    else if (transferFromPool == "soil_metabolic")
    {
        if (transferToPool == "active")
        {
            if (carbonContent_soilMetabolicLitterPool > 0.0)
            {
                nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = nitrogenContent_soilMetabolicLitterPool * (carbonFlux_soilMetabolicLitterPool_to_soilActivePool / carbonContent_soilMetabolicLitterPool);
            }
            else
            {
                nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = 0.0;
            }
        }
    }
    else if (transferFromPool == "microbes")
    {
        if (transferToPool == "slow")
        {
            if (carbonContent_soilMicrobesPool > 0.0)
            {
                nitrogenFlux_soilMicrobesPool_to_soilSlowPool = nitrogenContent_soilMicrobesPool * (carbonFlux_soilMicrobesPool_to_soilSlowPool / carbonContent_soilMicrobesPool);
            }
            else
            {
                nitrogenFlux_soilMicrobesPool_to_soilSlowPool = 0.0;
            }
        }
    }
    else if (transferFromPool == "active")
    {
        if (transferToPool == "slow")
        {
            if (carbonContent_soilActivePool > 0.0)
            {
                nitrogenFlux_soilActivePool_to_soilSlowPool = nitrogenContent_soilActivePool * (carbonFlux_soilActivePool_to_soilSlowPool / carbonContent_soilActivePool);
            }
            else
            {
                nitrogenFlux_soilActivePool_to_soilSlowPool = 0.0;
            }
        }
        if (transferToPool == "passive")
        {
            if (carbonContent_soilActivePool > 0.0)
            {
                nitrogenFlux_soilActivePool_to_soilPassivePool = nitrogenContent_soilActivePool * (carbonFlux_soilActivePool_to_soilPassivePool / carbonContent_soilActivePool);
            }
            else
            {
                nitrogenFlux_soilActivePool_to_soilPassivePool = 0.0;
            }
        }
    }
    else if (transferFromPool == "slow")
    {
        if (transferToPool == "active")
        {
            if (carbonContent_soilSlowPool > 0.0)
            {
                nitrogenFlux_soilSlowPool_to_soilActivePool = nitrogenContent_soilSlowPool * (carbonFlux_soilSlowPool_to_soilActivePool / carbonContent_soilSlowPool);
            }
            else
            {
                nitrogenFlux_soilSlowPool_to_soilActivePool = 0.0;
            }
        }
        if (transferToPool == "passive")
        {
            if (carbonContent_soilSlowPool > 0.0)
            {
                nitrogenFlux_soilSlowPool_to_soilPassivePool = nitrogenContent_soilSlowPool * (carbonFlux_soilSlowPool_to_soilPassivePool / carbonContent_soilSlowPool);
            }
            else
            {
                nitrogenFlux_soilSlowPool_to_soilPassivePool = 0.0;
            }
        }
    }
    else if (transferFromPool == "passive")
    {
        if (transferToPool == "active")
        {
            if (carbonContent_soilPassivePool > 0.0)
            {
                nitrogenFlux_soilPassivePool_to_soilActivePool = nitrogenContent_soilPassivePool * (carbonFlux_soilPassivePool_to_soilActivePool / carbonContent_soilPassivePool);
            }
            else
            {
                nitrogenFlux_soilPassivePool_to_soilActivePool = 0.0;
            }
        }
    }
}

/**
 * @brief Dispatches to `immobilizeNitrogen()` or `mineralizeNitrogen()` based
 *        on the decisive C/N ratio of the transfer.
 *
 * @param utils           Utility object for error handling.
 * @param carbonFlux      C flux of the decomposition step (g C).
 * @param nitrogenFlow    N flow estimate before immobilisation/mineralisation.
 * @param decisiveCNratio Decisive C/N ratio for this pool-to-pool transfer.
 * @param transferFromPool Source pool identifier string.
 * @param transferToPool   Destination pool identifier string.
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::immobilizeOrMineralizeNitrogen(UTILS utils, double carbonFlux, double nitrogenFlow, double decisiveCNratio, std::string transferFromPool, std::string transferToPool)
{
    double mineralize_fromPool_toPool, immobilize_fromPool_toPool;

    if (carbonFlux > 0.0 && nitrogenFlow > 0.0)
    {
        double actualCNRatioOfFluxes = carbonFlux / nitrogenFlow;

        if (actualCNRatioOfFluxes > decisiveCNratio)
        { // immobilization occurs
            // nitrogen resources are required from soil mineral nitrogen pool for decomposition
            // and will be used from top soil mineral nitrogen pool
            immobilizeNitrogen(utils, transferFromPool, transferToPool, decisiveCNratio);
        }
        else
        { // mineralization occurs as sufficient nitrogen resources are available in the material itself for decomposition
            // nitrogenFlux state variable will be changed (reduced to amount that is required for decomposition only)
            // nitrogen surplus will be added to soil mineral nitrogen pool
            mineralizeNitrogen(utils, transferFromPool, transferToPool, decisiveCNratio, nitrogenFlow);
        }
    }
}

/**
 * @brief Calculates the amount of mineral N that must be immobilised from the
 *        soil mineral pool to support decomposition of N-poor material.
 *
 * @param utils           Utility object for error handling.
 * @param transferFromPool Source pool identifier string.
 * @param transferToPool   Destination pool identifier string.
 * @param decisiveCNratio Decisive C/N ratio for this transfer.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::immobilizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio)
{
    if (transferFromPool == "surface_structural")
    {
        if (transferToPool == "slow")
        {
            immobilize_surfaceStructuralLitterPool_to_soilSlowPool = (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;
            nitrogenNetMineralization -= immobilize_surfaceStructuralLitterPool_to_soilSlowPool;
        }

        if (transferToPool == "microbes")
        {
            immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool = (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / decisiveCNratio) - nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
            nitrogenNetMineralization -= immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_structural")
    {
        if (transferToPool == "slow")
        {
            immobilize_soilStructuralLitterPool_to_soilSlowPool = (carbonFlux_soilStructuralLitterPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
            nitrogenNetMineralization -= immobilize_soilStructuralLitterPool_to_soilSlowPool;
        }

        if (transferToPool == "active")
        {
            immobilize_soilStructuralLitterPool_to_soilActivePool = (carbonFlux_soilStructuralLitterPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
            nitrogenNetMineralization -= immobilize_soilStructuralLitterPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "surface_metabolic")
    {
        if (transferToPool == "microbes")
        {
            immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool = (carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / decisiveCNratio) - nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
            nitrogenNetMineralization -= immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_metabolic")
    {
        if (transferToPool == "active")
        {
            immobilize_soilMetabolicLitterPool_to_soilActivePool = (carbonFlux_soilMetabolicLitterPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
            nitrogenNetMineralization -= immobilize_soilMetabolicLitterPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "microbes")
    {
        if (transferToPool == "slow")
        {
            immobilize_soilMicrobesPool_to_soilSlowPool = (carbonFlux_soilMicrobesPool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
            nitrogenNetMineralization -= immobilize_soilMicrobesPool_to_soilSlowPool;
        }
    }
    else if (transferFromPool == "active")
    {
        if (transferToPool == "passive")
        {
            immobilize_soilActivePool_to_soilPassivePool = (carbonFlux_soilActivePool_to_soilPassivePool / decisiveCNratio) - nitrogenFlux_soilActivePool_to_soilPassivePool;
            nitrogenNetMineralization -= immobilize_soilActivePool_to_soilPassivePool;
        }

        if (transferToPool == "slow")
        {
            immobilize_soilActivePool_to_soilSlowPool = (carbonFlux_soilActivePool_to_soilSlowPool / decisiveCNratio) - nitrogenFlux_soilActivePool_to_soilSlowPool;
            nitrogenNetMineralization -= immobilize_soilActivePool_to_soilSlowPool;
        }
    }
    else if (transferFromPool == "slow")
    {
        if (transferToPool == "passive")
        {
            immobilize_soilSlowPool_to_soilPassivePool = (carbonFlux_soilSlowPool_to_soilPassivePool / decisiveCNratio) - nitrogenFlux_soilSlowPool_to_soilPassivePool;
            nitrogenNetMineralization -= immobilize_soilSlowPool_to_soilPassivePool;
        }

        if (transferToPool == "active")
        {
            immobilize_soilSlowPool_to_soilActivePool = (carbonFlux_soilSlowPool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilSlowPool_to_soilActivePool;
            nitrogenNetMineralization -= immobilize_soilSlowPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "passive")
    {
        if (transferToPool == "active")
        {
            immobilize_soilPassivePool_to_soilActivePool = (carbonFlux_soilPassivePool_to_soilActivePool / decisiveCNratio) - nitrogenFlux_soilPassivePool_to_soilActivePool;
            nitrogenNetMineralization -= immobilize_soilPassivePool_to_soilActivePool;
        }
    }
}

/**
 * @brief Calculates net nitrogen mineralisation from N-rich decomposing material
 *        and adds it to the soil mineral N pool.
 *
 * @param utils              Utility object for error handling.
 * @param transferFromPool   Source pool identifier string.
 * @param transferToPool     Destination pool identifier string.
 * @param decisiveCNratio    Decisive C/N ratio for this transfer.
 * @param previousNitrogenFlow N flow before this step.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::mineralizeNitrogen(UTILS utils, std::string transferFromPool, std::string transferToPool, double decisiveCNratio, double previousNitrogenFlow)
{
    if (transferFromPool == "surface_structural")
    {
        if (transferToPool == "slow")
        {
            // nitrogenFlux is reduced to only required amounts for decomposition
            nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool = carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool / decisiveCNratio;

            // remaining nitrogen can be mineralized
            mineralize_surfaceStructuralLitterPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool;

            nitrogenGrossMineralization += mineralize_surfaceStructuralLitterPool_to_soilSlowPool;
            nitrogenNetMineralization += mineralize_surfaceStructuralLitterPool_to_soilSlowPool;
        }
        if (transferToPool == "microbes")
        {
            nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool = carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool / decisiveCNratio;
            mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool = previousNitrogenFlow - nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool;
            nitrogenGrossMineralization += mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool;
            nitrogenNetMineralization += mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_structural")
    {
        if (transferToPool == "slow")
        {
            nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool = carbonFlux_soilStructuralLitterPool_to_soilSlowPool / decisiveCNratio;
            mineralize_soilStructuralLitterPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool;
            nitrogenGrossMineralization += mineralize_soilStructuralLitterPool_to_soilSlowPool;
            nitrogenNetMineralization += mineralize_soilStructuralLitterPool_to_soilSlowPool;
        }
        if (transferToPool == "active")
        {
            nitrogenFlux_soilStructuralLitterPool_to_soilActivePool = carbonFlux_soilStructuralLitterPool_to_soilActivePool / decisiveCNratio;
            mineralize_soilStructuralLitterPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilStructuralLitterPool_to_soilActivePool;
            nitrogenGrossMineralization += mineralize_soilStructuralLitterPool_to_soilActivePool;
            nitrogenNetMineralization += mineralize_soilStructuralLitterPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "surface_metabolic")
    {
        if (transferToPool == "microbes")
        {
            nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool = carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool / decisiveCNratio;
            mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool = previousNitrogenFlow - nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
            nitrogenGrossMineralization += mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
            nitrogenNetMineralization += mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool;
        }
    }
    else if (transferFromPool == "soil_metabolic")
    {
        if (transferToPool == "active")
        {
            nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool = carbonFlux_soilMetabolicLitterPool_to_soilActivePool / decisiveCNratio;
            mineralize_soilMetabolicLitterPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool;
            nitrogenGrossMineralization += mineralize_soilMetabolicLitterPool_to_soilActivePool;
            nitrogenNetMineralization += mineralize_soilMetabolicLitterPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "microbes")
    {
        if (transferToPool == "slow")
        {
            nitrogenFlux_soilMicrobesPool_to_soilSlowPool = carbonFlux_soilMicrobesPool_to_soilSlowPool / decisiveCNratio;
            mineralize_soilMicrobesPool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilMicrobesPool_to_soilSlowPool;
            nitrogenGrossMineralization += mineralize_soilMicrobesPool_to_soilSlowPool;
            nitrogenNetMineralization += mineralize_soilMicrobesPool_to_soilSlowPool;
        }
    }
    else if (transferFromPool == "active")
    {
        if (transferToPool == "passive")
        {
            nitrogenFlux_soilActivePool_to_soilPassivePool = carbonFlux_soilActivePool_to_soilPassivePool / decisiveCNratio;
            mineralize_soilActivePool_to_soilPassivePool = previousNitrogenFlow - nitrogenFlux_soilActivePool_to_soilPassivePool;
            nitrogenGrossMineralization += mineralize_soilActivePool_to_soilPassivePool;
            nitrogenNetMineralization += mineralize_soilActivePool_to_soilPassivePool;
        }

        if (transferToPool == "slow")
        {
            nitrogenFlux_soilActivePool_to_soilSlowPool = carbonFlux_soilActivePool_to_soilSlowPool / decisiveCNratio;
            mineralize_soilActivePool_to_soilSlowPool = previousNitrogenFlow - nitrogenFlux_soilActivePool_to_soilSlowPool;
            nitrogenGrossMineralization += mineralize_soilActivePool_to_soilSlowPool;
            nitrogenNetMineralization += mineralize_soilActivePool_to_soilSlowPool;
        }
    }
    else if (transferFromPool == "slow")
    {
        if (transferToPool == "passive")
        {
            nitrogenFlux_soilSlowPool_to_soilPassivePool = carbonFlux_soilSlowPool_to_soilPassivePool / decisiveCNratio;
            mineralize_soilSlowPool_to_soilPassivePool = previousNitrogenFlow - nitrogenFlux_soilSlowPool_to_soilPassivePool;
            nitrogenGrossMineralization += mineralize_soilSlowPool_to_soilPassivePool;
            nitrogenNetMineralization += mineralize_soilSlowPool_to_soilPassivePool;
        }

        if (transferToPool == "active")
        {
            nitrogenFlux_soilSlowPool_to_soilActivePool = carbonFlux_soilSlowPool_to_soilActivePool / decisiveCNratio;
            mineralize_soilSlowPool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilSlowPool_to_soilActivePool;
            nitrogenGrossMineralization += mineralize_soilSlowPool_to_soilActivePool;
            nitrogenNetMineralization += mineralize_soilSlowPool_to_soilActivePool;
        }
    }
    else if (transferFromPool == "passive")
    {
        if (transferToPool == "active")
        {
            nitrogenFlux_soilPassivePool_to_soilActivePool = carbonFlux_soilPassivePool_to_soilActivePool / decisiveCNratio;
            mineralize_soilPassivePool_to_soilActivePool = previousNitrogenFlow - nitrogenFlux_soilPassivePool_to_soilActivePool;
            nitrogenGrossMineralization += mineralize_soilPassivePool_to_soilActivePool;
            nitrogenNetMineralization += mineralize_soilPassivePool_to_soilActivePool;
        }
    }
}

/**
 * @brief Applies all computed C/N fluxes and respiratory losses to update
 *        the soil pool contents at the end of each decomposition time step.
 *
 * Iterates over all transfer routes and adds/subtracts the accumulated
 * carbon and nitrogen flow and respiration values to the respective pool
 * content variables.
 *
 * @param utils Utility object for error handling.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::updateSoilPoolsByRespirationAndFluxes(UTILS utils)
{

    // ############## Respiration ####################
    // carbon respiration: subtracted from pools and added to cumulative output variables
    carbonContent_surfaceStructuralLitterPool -= (respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool);
    respirationCarbon_surface_litter += (respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool);

    carbonContent_surfaceMetabolicLitterPool -= respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;
    respirationCarbon_surface_litter += respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool;

    carbonContent_soilStructuralLitterPool -= (respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool);
    respirationCarbon_soil_litter += (respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool + respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool);

    carbonContent_soilMetabolicLitterPool -= respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;
    respirationCarbon_soil_litter += respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool;

    respirationCarbon_litter += respirationCarbon_soil_litter + respirationCarbon_surface_litter;

    carbonContent_soilMicrobesPool -= respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool;
    carbonContent_soilActivePool -= respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool;
    carbonContent_soilSlowPool -= respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool;
    carbonContent_soilPassivePool -= respiration_decompositionCarbon_soilPassivePool_soilActivePool;

    respirationCarbon_soilpools += (respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool + respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool +
                                    respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool + respiration_decompositionCarbon_soilPassivePool_soilActivePool);

    // nitrogen respiratory fluxes: subtracted from pools and added to cumulative output variables
    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) +=
        (respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool +
         respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool +
         respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool +
         respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool);

    nitrogenContent_surfaceStructuralLitterPool -= (respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool);
    nitrogenContent_surfaceMetabolicLitterPool -= respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool;

    nitrogenContent_soilStructuralLitterPool -= (respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool + respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool);
    nitrogenContent_soilMetabolicLitterPool -= respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool;

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += (respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool + respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool +
                                                          respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool + respiration_decompositionNitrogen_soilPassivePool_soilActivePool);

    nitrogenContent_soilActivePool -= respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool;
    nitrogenContent_soilMicrobesPool -= respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool;
    nitrogenContent_soilSlowPool -= respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool;
    nitrogenContent_soilPassivePool -= respiration_decompositionNitrogen_soilPassivePool_soilActivePool;

    // ############## Fluxes between soil pools ####################
    // carbon fluxes: added and subtracted to/from pools
    carbonContent_soilSlowPool += (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + carbonFlux_soilStructuralLitterPool_to_soilSlowPool);
    carbonContent_soilMicrobesPool += (carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool + carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool);
    carbonContent_soilActivePool += (carbonFlux_soilStructuralLitterPool_to_soilActivePool + carbonFlux_soilMetabolicLitterPool_to_soilActivePool);

    carbonContent_surfaceStructuralLitterPool -= (carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool + carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool);
    carbonContent_soilStructuralLitterPool -= (carbonFlux_soilStructuralLitterPool_to_soilSlowPool + carbonFlux_soilStructuralLitterPool_to_soilActivePool);
    carbonContent_surfaceMetabolicLitterPool -= carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool;
    carbonContent_soilMetabolicLitterPool -= carbonFlux_soilMetabolicLitterPool_to_soilActivePool;

    carbonContent_soilSlowPool += (carbonFlux_soilMicrobesPool_to_soilSlowPool + carbonFlux_soilActivePool_to_soilSlowPool);
    carbonContent_soilSlowPool -= (carbonFlux_soilSlowPool_to_soilActivePool + carbonFlux_soilSlowPool_to_soilPassivePool);

    carbonContent_soilPassivePool += (carbonFlux_soilActivePool_to_soilPassivePool + carbonFlux_soilSlowPool_to_soilPassivePool);
    carbonContent_soilPassivePool -= carbonFlux_soilPassivePool_to_soilActivePool;

    carbonContent_soilActivePool += (carbonFlux_soilSlowPool_to_soilActivePool + carbonFlux_soilPassivePool_to_soilActivePool);
    carbonContent_soilActivePool -= (carbonFlux_soilActivePool_to_soilSlowPool + carbonFlux_soilActivePool_to_soilPassivePool + leachingCarbon);

    carbonContent_soilMicrobesPool -= carbonFlux_soilMicrobesPool_to_soilSlowPool;

    carbonContent_leachedFromSoil += leachingCarbon;

    // nitrogen fluxes: added and subtracted to/from pools
    // nitrogenFlow is subtracted from pools (contains nitrogenFlux + mineralizableNitrogen in case of mineralization; in case of immobilization nitrogenFlow = nitrogenFlux)
    // nitrogenFlux is added to soil pools (proportional to added carbon; might be reduced in case of mineralization; in case of immobilization nitrogenFlow = nitrogenFlux)
    nitrogenContent_surfaceStructuralLitterPool -= (nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool + nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool);
    nitrogenContent_soilStructuralLitterPool -= nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool + nitrogenFlow_soilStructuralLitterPool_to_soilActivePool;
    nitrogenContent_surfaceMetabolicLitterPool -= nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool;
    nitrogenContent_soilMetabolicLitterPool -= nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool;

    nitrogenContent_soilSlowPool += (nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool + nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool);
    nitrogenContent_soilSlowPool += (nitrogenFlux_soilMicrobesPool_to_soilSlowPool + nitrogenFlux_soilActivePool_to_soilSlowPool);
    nitrogenContent_soilSlowPool -= (nitrogenFlow_soilSlowPool_to_soilActivePool + nitrogenFlow_soilSlowPool_to_soilPassivePool);

    nitrogenContent_soilPassivePool += (nitrogenFlux_soilActivePool_to_soilPassivePool + nitrogenFlux_soilSlowPool_to_soilPassivePool);
    nitrogenContent_soilPassivePool -= nitrogenFlow_soilPassivePool_to_soilActivePool;

    nitrogenContent_soilActivePool += (nitrogenFlux_soilStructuralLitterPool_to_soilActivePool + nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool);
    nitrogenContent_soilActivePool += (nitrogenFlux_soilSlowPool_to_soilActivePool + nitrogenFlux_soilPassivePool_to_soilActivePool);
    nitrogenContent_soilActivePool -= (nitrogenFlow_soilActivePool_to_soilSlowPool + nitrogenFlow_soilActivePool_to_soilPassivePool + leachingNitrogen);

    nitrogenContent_soilMicrobesPool += (nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool + nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool);
    nitrogenContent_soilMicrobesPool -= nitrogenFlow_soilMicrobesPool_to_soilSlowPool;

    nitrogenContent_leachedFromSoil += leachingNitrogen;

    // ############## Immobilization/Mineralization ####################
    // either immobilization or mineralization occurs per soil pool flux
    // if immobilization occurs, nitrogen is added to the target soil pool (n)
    // and subtracted from mineral N pool in upper soil layer
    // (i.e. miner_... is negative immobilization rate)
    // if mineralization occurs, immob_... is zero

    nitrogenContent_soilSlowPool += (immobilize_surfaceStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilSlowPool +
                                     immobilize_soilMicrobesPool_to_soilSlowPool + immobilize_soilActivePool_to_soilSlowPool);

    nitrogenContent_soilMicrobesPool += (immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool + immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool);

    nitrogenContent_soilActivePool += (immobilize_soilMetabolicLitterPool_to_soilActivePool + immobilize_soilPassivePool_to_soilActivePool +
                                       immobilize_soilSlowPool_to_soilActivePool + immobilize_soilStructuralLitterPool_to_soilActivePool);

    nitrogenContent_soilPassivePool += (immobilize_soilActivePool_to_soilPassivePool + immobilize_soilSlowPool_to_soilPassivePool);

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -=
        (immobilize_surfaceStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilSlowPool + immobilize_soilStructuralLitterPool_to_soilActivePool +
         immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool + immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool + immobilize_soilMetabolicLitterPool_to_soilActivePool +
         immobilize_soilMicrobesPool_to_soilSlowPool + immobilize_soilActivePool_to_soilSlowPool + immobilize_soilPassivePool_to_soilActivePool +
         immobilize_soilSlowPool_to_soilActivePool + immobilize_soilActivePool_to_soilPassivePool + immobilize_soilSlowPool_to_soilPassivePool);

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) +=
        (mineralize_surfaceStructuralLitterPool_to_soilSlowPool + mineralize_soilStructuralLitterPool_to_soilSlowPool +
         mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool + mineralize_soilStructuralLitterPool_to_soilActivePool +
         mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool + mineralize_soilMetabolicLitterPool_to_soilActivePool +
         mineralize_soilActivePool_to_soilPassivePool + mineralize_soilActivePool_to_soilSlowPool +
         mineralize_soilMicrobesPool_to_soilSlowPool + mineralize_soilSlowPool_to_soilPassivePool +
         mineralize_soilSlowPool_to_soilActivePool + mineralize_soilPassivePool_to_soilActivePool);
}

/**
 * @brief Computes non-symbiotic N fixation and atmospheric N deposition and
 *        adds the combined amount to the topsoil mineral N pool.
 *
 * Non-symbiotic fixation scales linearly with annual precipitation
 * (converted to cm). Atmospheric deposition is a fixed value per unit time.
 *
 * @param utils     Utility object (reserved).
 * @param parameter Provides simulation day for weather indexing.
 * @param weather   Provides annual precipitation for the fixation estimate.
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::calculateNonsymbioticNitrogenFixationAndAthmosphericDeposition(UTILS utils, PARAMETER parameter, WEATHER weather)
{
    double nonsymbioticNitrogenFixation;
    double athomsphericDeposition;

    nonsymbioticNitrogenFixation = 0;
    athomsphericDeposition = 0.01 * (weather.potEvapoTranspiration.at(parameter.day - 1) - (30.0 / 365.0));
    athomsphericDeposition = std::max(athomsphericDeposition, 0.0);

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) += (athomsphericDeposition + nonsymbioticNitrogenFixation);
    nitrogenFixationToSoil += (athomsphericDeposition + nonsymbioticNitrogenFixation);

    if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + TOLERANCE < 0.0)
    {
        nitrogenContent_soilMineralPoolPerSoilLayer.at(0) = 0.0;
        // utils.handleError("Soil mineral nitrogen in the top soil layer is negative!");
    }
}

/**
 * @brief Calculates daily nitrogen loss by volatilization from the topsoil
 *        mineral N pool.
 *
 * Volatilization is proportional to gross N mineralisation; the lost N is
 * deducted from `nitrogenContent_soilMineralPoolPerSoilLayer[0]` and added
 * to `nitrogenVolatilization`.
 *
 * @param utils Utility object (reserved).
 *
 * @cite Adapted from the CENTURY 4.0 model.
 */
void SOIL::calculateNitrogenLossByVolatilization(UTILS utils)
{
    double nitrogenLossByVolatilization = 0.0;

    nitrogenLossByVolatilization = 0.0 * nitrogenGrossMineralization;

    nitrogenContent_soilMineralPoolPerSoilLayer.at(0) -= nitrogenLossByVolatilization;
    nitrogenVolatilization += nitrogenLossByVolatilization;

    if (nitrogenContent_soilMineralPoolPerSoilLayer.at(0) + TOLERANCE < 0.0)
    {
        nitrogenContent_soilMineralPoolPerSoilLayer.at(0) = 0.0;
        // utils.handleError("Soil mineral nitrogen in the top soil layer is negative!");
    }
}

/**
 * @brief Deducts carbon leached from the active soil pool and accumulates
 *        the cumulative leached carbon.
 *
 * @param utils Utility object (reserved).
 * @cite Function and code adapted from the CENTURY 4.0 soil model.
 */
void SOIL::doLeaching(UTILS utils)
{
    // leaching of organics
    // only occurs if water flow out of water layer 2 exceeds a critical value
    // uses the same C/N ratio as for the flow to passive soil pool

    leachingCarbon = 0.0;
    leachingNitrogen = 0.0;

    if (waterContent_soilWaterPoolPerSoilLayer.at(1) > 0.0)
    {
        double soilWaterFactor = std::max(1.0, 1.0 - (1.9 - (waterContent_soilWaterPoolPerSoilLayer.at(1) / 10.0)) / 1.9);
        double soilTypeFactor = (0.05 + 0.15 * sandContent);
        double carbonNitrogenRatioOfActivePool = carbonContent_soilActivePool / nitrogenContent_soilActivePool;

        leachingCarbon = carbonFlux_soilActivePool_to_soilPassiveAndSlowPool * soilTypeFactor * soilWaterFactor;
        leachingNitrogen = leachingCarbon / carbonNitrogenRatioOfActivePool;
    }
}
