#include "output.h"
#include <sys/stat.h> // Required for mkdir

OUTPUT::OUTPUT() {};
OUTPUT::~OUTPUT() {};

/**
 * @brief Aggregates per-cohort state variables into PFT-level and community-level
 *        output accumulators, and computes ecosystem-scale balances.
 *
 * Iterates over all plant cohorts and accumulates the following quantities:
 * - **Per-PFT**: plant count, covered area, shoot biomass (total, green, brown,
 *   clipped above `clippingHeightOfBiomassMeasurement`), root biomass, recruitment
 *   biomass, exudation biomass, GPP, NPP, and respiration.
 * - **Community-wide**: total plant count, carbon respiration, carbon NPP,
 *   aboveground biomass, and aboveground litter biomass (from soil surface pools).
 *
 * After the cohort loop:
 * - PFT composition fractions are normalised to percentages of total plant count.
 * - Seedling C ingrowth is summed across PFTs from successful germination counts.
 * - Ecosystem-level N and C balances are computed:
 *   - N balance = net mineralisation − volatilisation + fixation + fertilisation.
 *   - C respiration = litter + soil-pool + plant respiration.
 *   - C balance = plant NPP + seedling ingrowth − ecosystem respiration − C leaching
 *     − harvested yield C.
 *
 * @param parameter   Read-only; provides `pftCount`, `clippingHeightOfBiomassMeasurement`,
 *                    and `seedMasses`.
 * @param community   Plant community; per-PFT and community-level accumulators updated
 *                    in place.
 * @param soil        Soil state; surface litter and flux fields read for litter biomass
 *                    and ecosystem balance calculations.
 * @param management  Management state; `biomassYield` read for C-balance correction.
 * @param recruitment Recruitment state; `successfullGerminatedSeeds` read for seedling
 *                    C ingrowth.
 */
void OUTPUT::updateVegetationStateVariablesForOutput(PARAMETER parameter, COMMUNITY &community, SOIL soil, MANAGEMENT management, RECRUITMENT recruitment)
{
    // Summing up plant-specific variables for population- and community-based variables
    if (community.allPlants.size() > 0)
    {
        for (int cohortindex = 0; cohortindex < community.allPlants.size(); cohortindex++)
        {
            // PFT-specific calculations
            community.pftComposition[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->amount;
            community.numberOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->amount;
            community.coveredAreaOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->coveredArea * community.allPlants[cohortindex]->amount;
            community.shootBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->shootBiomass * community.allPlants[cohortindex]->amount;
            community.greenShootBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->shootBiomassGreenLeaves * community.allPlants[cohortindex]->amount;
            community.brownShootBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->shootBiomassBrownLeaves * community.allPlants[cohortindex]->amount;

            if (community.allPlants.at(cohortindex)->height > parameter.clippingHeightOfBiomassMeasurement)
            {
                community.allPlants.at(cohortindex)->shootBiomassAboveClippingHeight =
                    ((community.allPlants.at(cohortindex)->height - parameter.clippingHeightOfBiomassMeasurement) / community.allPlants.at(cohortindex)->height) * community.allPlants.at(cohortindex)->shootBiomass;
                community.clippedShootBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->shootBiomassAboveClippingHeight * community.allPlants[cohortindex]->amount;
            }

            community.rootBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->rootBiomass * community.allPlants[cohortindex]->amount;
            community.recruitmentBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->recruitmentBiomass * community.allPlants[cohortindex]->amount;
            community.exudationBiomassOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->exudationBiomass * community.allPlants[cohortindex]->amount;

            community.gppOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->gpp * community.allPlants[cohortindex]->amount;
            community.nppOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->npp * community.allPlants[cohortindex]->amount;
            community.carbonRespirationOfPlantsPerPFT[community.allPlants[cohortindex]->pft] += community.allPlants[cohortindex]->totalRespiration * community.allPlants[cohortindex]->amount;

            // Community-wide calculations
            community.totalNumberOfPlantsInCommunity += community.allPlants[cohortindex]->amount;
            community.carbonRespirationOfAllPlants += community.allPlants[cohortindex]->totalRespiration * community.allPlants[cohortindex]->amount * CARBON_CONTENT_ODM;
            community.carbonNPPOfAllPlants += community.allPlants[cohortindex]->npp * community.allPlants[cohortindex]->amount * CARBON_CONTENT_ODM;
            community.abovegroundBiomassOfAllPlants += community.allPlants[cohortindex]->shootBiomass * community.allPlants[cohortindex]->amount;
        }

        // Normalizations
        if (community.totalNumberOfPlantsInCommunity > 0)
        {
            for (int pft = 0; pft < parameter.pftCount; pft++)
            {
                community.pftComposition[pft] *= 100.0 / community.totalNumberOfPlantsInCommunity;
            }
        }
    }
    // calculations from soil state variables
    community.abovegroundLitterBiomass += (soil.carbonContent_surfaceStructuralLitterPool + soil.carbonContent_surfaceMetabolicLitterPool) * (1.0 / CARBON_CONTENT_ODM);

    // Summing up PFT-specific variables for community-based variables
    for (int pft = 0; pft < parameter.pftCount; pft++)
    {
        community.carbonSeedlingIngrowthOfAllPlants += recruitment.successfullGerminatedSeeds.at(pft) * parameter.seedMasses[pft] * CARBON_CONTENT_ODM;
    }

    // Summing up community-specific variables for ecosystem-based variables
    community.ecosystemNitrogenBalance = soil.nitrogenNetMineralization - soil.nitrogenVolatilization + soil.nitrogenFixationToSoil + soil.addedMineralNitrogenToSoilByFertilization;
    community.ecosystemCarbonRespiration = soil.respirationCarbon_litter + soil.respirationCarbon_soilpools + community.carbonRespirationOfAllPlants;
    community.ecosystemCarbonBalance = community.carbonNPPOfAllPlants + community.carbonSeedlingIngrowthOfAllPlants - community.ecosystemCarbonRespiration - soil.carbonContent_leachedFromSoil - (CARBON_CONTENT_ODM * community.biomassYield);
}

/**
 * @brief Prepares all model output infrastructure before the simulation loop starts.
 *
 * Executes the following steps in order:
 * 1. createOutputFolder() — creates the `output/` directory next to the config file.
 * 2. createAndOpenOutputFiles() — constructs file names and opens `std::ofstream`
 *    objects for each enabled output type.
 * 3. writeHeaderInOutputFiles() — writes tab-separated column headers to each open file.
 * 4. openAndReadOutputWritingDates() — loads optional date list controlling which days
 *    are written; defaults to daily if no file is provided.
 *
 * @param path      Absolute path to the main configuration file; used to derive the
 *                  output directory and the output-writing-dates file location.
 * @param utils     Utility object for path splitting and error handling.
 * @param parameter Model parameters; file names, flags, and coordinate strings read;
 *                  no fields are written.
 */
void OUTPUT::prepareModelOutput(std::string path, UTILS utils, PARAMETER &parameter)
{
    createOutputFolder(path, utils);
    createAndOpenOutputFiles(utils, parameter);
    writeHeaderInOutputFiles(utils, parameter);
    openAndReadOutputWritingDates(path, utils, parameter);
}

/**
 * @brief Constructs output file names and opens one `std::ofstream` per enabled
 *        output type.
 *
 * The file name pattern is:
 * @code
 * <outputDirectory>lat<lat>_lon<lon>__<firstYear>-01-01_<lastYear>-12-31
 *     __run<NNN>__output<Type>__<parameterSuffix>
 * @endcode
 * where `<NNN>` is the zero-padded RNG seed (3 digits) and `<parameterSuffix>`
 * is derived from the last two underscore-separated tokens of `plantTraitsFile`.
 *
 * The following output files are opened when their respective boolean flag in
 * `parameter` is `true`:
 * - Community variables (`communityOutputFile`)
 * - PFT population variables (`pftOutputFile`)
 * - Plant cohort variables (`plantCohortOutputFile`)
 * - Soil carbon pools and fluxes (`soilCarbonOutputFile`)
 * - Soil nitrogen pools and fluxes (`soilNitrogenOutputFile`)
 * - Soil water fluxes (`soilWaterOutputFile`)
 * - Soil resources per layer (`soilResourcesPerSoilLayerOutputFile`)
 * - Detailed soil flux breakdown (`soilFluxesDetailsOutputFile`)
 *
 * Calls `utils.handleError()` if any enabled file cannot be opened.
 *
 * @param utils     Utility object for string splitting and error handling.
 * @param parameter Read-only; provides coordinates, year range, RNG seed, file flags,
 *                  and `plantTraitsFile` for suffix derivation.
 */
void OUTPUT::createAndOpenOutputFiles(UTILS utils, PARAMETER parameter)
{
    utils.strings.clear();
    utils.splitString(parameter.plantTraitsFile, '/');
    std::string endingLocation = "lat" + parameter.latitude + "_lon" + parameter.longitude;
    std::string endingYears = "__" + std::to_string(parameter.firstYear) + "-01-01_" + std::to_string(parameter.lastYear) + "-12-31";
    std::string runNumber = (parameter.randomNumberGeneratorSeed < 10) ? ("00" + std::to_string(parameter.randomNumberGeneratorSeed)) : ((parameter.randomNumberGeneratorSeed < 100) ? ("0" + std::to_string(parameter.randomNumberGeneratorSeed)) : (std::to_string(parameter.randomNumberGeneratorSeed)));
    std::string endingRandomSeed = "__run" + runNumber;
    std::string plantTraitsFile = utils.strings.at(1);
    utils.strings.clear();
    utils.splitString(plantTraitsFile, '_');
    std::string endingParameter = utils.strings.at(utils.strings.size() - 2) + "_" + utils.strings.at(utils.strings.size() - 1);

    std::string filenameCommunity = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputCommunity__" + endingParameter;
    std::string filenamePFTPopulation = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputPFT__" + endingParameter;
    std::string filenamePlant = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputPlant__" + endingParameter;
    std::string filenameSoilCarbon = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputSoilCarbon__" + endingParameter;
    std::string filenameSoilNitrogen = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputSoilNitrogen__" + endingParameter;
    std::string filenameSoilWater = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputSoilWater__" + endingParameter;
    std::string filenameSoilResourcesPerSoilLayer = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputSoilResourcesPerSoilLayer__" + endingParameter;
    std::string filenameSoilFluxesDetails = outputDirectory + endingLocation + endingYears + endingRandomSeed + "__outputSoilFluxesDetails__" + endingParameter;

    if (parameter.communityOutputFile)
    {
        outputCommunity.open(filenameCommunity);
        if (!outputCommunity.is_open())
        {
            utils.handleError("Error writing to the community output file.");
        }
    }

    if (parameter.pftOutputFile)
    {
        outputPFTPopulation.open(filenamePFTPopulation);
        if (!outputPFTPopulation.is_open())
        {
            utils.handleError("Error writing to the PFT population output file.");
        }
    }

    if (parameter.plantCohortOutputFile)
    {
        outputPlant.open(filenamePlant);
        if (!outputPlant.is_open())
        {
            utils.handleError("Error writing to the plant / cohorte output file.");
        }
    }

    if (parameter.soilCarbonOutputFile)
    {
        outputSoilCarbon.open(filenameSoilCarbon);
        if (!outputSoilCarbon.is_open())
        {
            utils.handleError("Error writing to the soil carbon output file.");
        }
    }

    if (parameter.soilNitrogenOutputFile)
    {
        outputSoilNitrogen.open(filenameSoilNitrogen);
        if (!outputSoilNitrogen.is_open())
        {
            utils.handleError("Error writing to the soil nitrogen output file.");
        }
    }

    if (parameter.soilWaterOutputFile)
    {
        outputSoilWater.open(filenameSoilWater);
        if (!outputSoilWater.is_open())
        {
            utils.handleError("Error writing to the soil water output file.");
        }
    }

    if (parameter.soilResourcesPerSoilLayerOutputFile)
    {
        outputSoilResourcesPerSoilLayer.open(filenameSoilResourcesPerSoilLayer);
        if (!outputSoilResourcesPerSoilLayer.is_open())
        {
            utils.handleError("Error writing to the soil resources per soil layer output file.");
        }
    }

    if (parameter.soilFluxesDetailsOutputFile)
    {
        outputSoilFluxesDetails.open(filenameSoilFluxesDetails);
        if (!outputSoilFluxesDetails.is_open())
        {
            utils.handleError("Error writing to the soil fluxes details output file.");
        }
    }
}

/**
 * @brief Writes tab-separated column headers to all open output files.
 *
 * Each enabled output file receives a single header line. The column sets are:
 * - **Community**: Date, DayCount, NumberPlants, NumberCohorts, LeafAreaIndex,
 *   VegetationHeight, VegetationCover, CBalance, NBalance.
 * - **PFT**: Date, DayCount, PFT, Fraction, NumberPlants, CoveredArea, ShootBiomass,
 *   GreenShootBiomass, BrownShootBiomass, ClippedShootBiomass, RootBiomass,
 *   RecruitmentBiomass, ExudationBiomass, GPP, NPP, Respiration.
 * - **Plant cohort**: Date, DayCount, PFT, Age, NumberPlants, Height, Width, LAI,
 *   CoveredArea, RootDepth, NumberSoilLayers, ShootBiomass, GreenShootBiomass,
 *   BrownShootBiomass, ClippedShootBiomass, RootBiomass, RecruitmentBiomass,
 *   ExudationBiomass, GPP, NPP, Respiration, Radiation, ShadingIndicator,
 *   LimitingFactorWater, LimitingFactorNitrogen, AllocationShoot, AllocationRoot,
 *   AllocationRecruitment, AllocationExudation.
 * - **Soil carbon**: Date, DayCount, all C pool contents and inter-pool C fluxes.
 * - **Soil nitrogen**: Date, DayCount, all N pool contents, inter-pool N fluxes,
 *   mineralisation, volatilisation, fixation, fertilisation, and leaching.
 * - **Soil water**: Date, DayCount, irrigation input, interception, surface/soil
 *   run-off, snow stores, evaporation, soil temperature.
 * - **Soil resources per layer**: Date, DayCount, layer number and width, downward
 *   water flux, water content, mineral N content.
 * - **Soil fluxes details**: Date, DayCount, decisive C/N ratios, lignin contents,
 *   decomposition C/N respiratory losses, immobilisation and mineralisation fluxes,
 *   and decomposition factor.
 *
 * Calls `utils.handleError()` if an enabled file is not open when this method runs.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Read-only; boolean output-file flags used to guard each block.
 */
void OUTPUT::writeHeaderInOutputFiles(UTILS utils, PARAMETER parameter)
{

    if (parameter.communityOutputFile)
    {
        if (!outputCommunity.is_open())
        {
            utils.handleError("Error writing to the community output file.");
        }
        else
        {
            outputCommunity << "Date\tDayCount\tNumberPlants(m-2)\tNumberCohorts(m-2)\tLeafAreaIndex(-)\tVegetationHeight(cm)\tVegetationCover(percent)\tCBalance(gC m-2)\tNBalance(gN m-2)";
            outputCommunity << std::endl;
        }
    }

    if (parameter.pftOutputFile)
    {
        if (!outputPFTPopulation.is_open())
        {
            utils.handleError("Error writing to the PFT population output file.");
        }
        else
        {
            outputPFTPopulation << "Date\tDayCount\tPFT\tFraction\tNumberPlants\t";
            outputPFTPopulation << "CoveredArea\tShootBiomass\tGreenShootBiomass\tBrownShootBiomass\t";
            outputPFTPopulation << "ClippedShootBiomass\tRootBiomass\tRecruitmentBiomass\tExudationBiomass\t";
            outputPFTPopulation << "GPP\tNPP\tRespiration";
            outputPFTPopulation << std::endl;
        }
    }

    if (parameter.plantCohortOutputFile)
    {
        if (!outputPlant.is_open())
        {
            utils.handleError("Error writing to the plant / cohorte output file.");
        }
        else
        {
            outputPlant << "Date\tDayCount\tPFT\tAge\tNumberPlants\tHeight\tWidth\tLAI\t";
            outputPlant << "CoveredArea\tRootDepth\tNumberSoilLayers\t";
            outputPlant << "ShootBiomass\tGreenShootBiomass\tBrownShootBiomass\t";
            outputPlant << "ClippedShootBiomass\tRootBiomass\tRecruitmentBiomass\tExudationBiomass\t";
            outputPlant << "GPP\tNPP\tRespiration\t";
            outputPlant << "Radiation\tShadingIndicator\tLimitingFactorWater\tLimitingFactorNitrogen\t";
            outputPlant << "AllocationShoot\tAllocationRoot\tAllocationRecruitment\tAllocationExudation";
            outputPlant << std::endl;
        }
    }

    if (parameter.soilCarbonOutputFile)
    {
        if (!outputSoilCarbon.is_open())
        {
            utils.handleError("Error writing to the soil carbon output file.");
        }
        else
        {
            outputSoilCarbon << "Date\tDayCount\t";
            outputSoilCarbon << "carbonContent_surfaceGreenLitterPool\t";
            outputSoilCarbon << "carbonContent_surfaceBrownLitterPool\t";
            outputSoilCarbon << "carbonContent_soilRootLitterPool\tcarbonContent_soilSeedLitterPool\t";
            outputSoilCarbon << "carbonContent_surfaceStructuralLitterPool\tcarbonContent_surfaceMetabolicLitterPool\t";
            outputSoilCarbon << "carbonContent_soilStructuralLitterPool\tcarbonContent_soilMetabolicLitterPool\t";
            outputSoilCarbon << "carbonContent_soilMicrobesPool\tcarbonContent_soilActivePool\t";
            outputSoilCarbon << "carbonContent_soilSlowPool\tcarbonContent_soilPassivePool\t";
            outputSoilCarbon << "carbonContent_leachedFromSoil\t";
            outputSoilCarbon << "carbonFlux_surfaceStructuralLitterPool_to_soilMicrobesPool\t";
            outputSoilCarbon << "carbonFlux_surfaceStructuralLitterPool_to_soilSlowPool\t";
            outputSoilCarbon << "carbonFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool\t";
            outputSoilCarbon << "carbonFlux_soilStructuralLitterPool_to_soilActivePool\t";
            outputSoilCarbon << "carbonFlux_soilStructuralLitterPool_to_soilSlowPool\t";
            outputSoilCarbon << "carbonFlux_soilSlowPool_to_soilPassivePool\t";
            outputSoilCarbon << "carbonFlux_soilSlowPool_to_soilActivePool\t";
            outputSoilCarbon << "carbonFlux_soilSlowPool_to_soilPassiveAndActivePool\t";
            outputSoilCarbon << "carbonFlux_soilActivePool_to_soilPassivePool\t";
            outputSoilCarbon << "carbonFlux_soilActivePool_to_soilSlowPool\t";
            outputSoilCarbon << "carbonFlux_soilActivePool_to_soilPassiveAndSlowPool\t";
            outputSoilCarbon << "carbonFlux_soilMetabolicLitterPool_to_soilActivePool\t";
            outputSoilCarbon << "carbonFlux_soilMicrobesPool_to_soilSlowPool\t";
            outputSoilCarbon << "carbonFlux_soilPassivePool_to_soilActivePool\t";
            outputSoilCarbon << "leachingCarbon";
            outputSoilCarbon << std::endl;
        }
    }

    if (parameter.soilNitrogenOutputFile)
    {
        if (!outputSoilNitrogen.is_open())
        {
            utils.handleError("Error writing to the soil nitrogen output file.");
        }
        else
        {
            outputSoilNitrogen << "Date\tDayCount\t";
            outputSoilNitrogen << "nitrogenContent_surfaceGreenLitterPool\t";
            outputSoilNitrogen << "nitrogenContent_surfaceBrownLitterPool\t";
            outputSoilNitrogen << "nitrogenContent_soilRootLitterPool\t"
                               << "nitrogenContent_soilSeedLitterPool\t";
            outputSoilNitrogen << "nitrogenContent_surfaceStructuralLitterPool\t"
                               << "nitrogenContent_surfaceMetabolicLitterPool\t";
            outputSoilNitrogen << "nitrogenContent_soilStructuralLitterPool\t"
                               << "nitrogenContent_soilMetabolicLitterPool\t";
            outputSoilNitrogen << "nitrogenContent_soilMicrobesPool\t"
                               << "nitrogenContent_soilActivePool\t";
            outputSoilNitrogen << "nitrogenContent_soilSlowPool\t"
                               << "nitrogenContent_soilPassivePool\t";
            outputSoilNitrogen << "nitrogenContent_leachedFromSoil\t";
            outputSoilNitrogen << "nitrogenFlux_surfaceStructuralLitterPool_to_soilMicrobesPool\t";
            outputSoilNitrogen << "nitrogenFlux_surfaceStructuralLitterPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlux_surfaceMetabolicLitterPool_to_soilMicrobesPool\t";
            outputSoilNitrogen << "nitrogenFlux_soilStructuralLitterPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlux_soilStructuralLitterPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlux_soilSlowPool_to_soilPassivePool\t";
            outputSoilNitrogen << "nitrogenFlux_soilSlowPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlux_soilActivePool_to_soilPassivePool\t";
            outputSoilNitrogen << "nitrogenFlux_soilActivePool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlux_soilMetabolicLitterPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlux_soilMicrobesPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlux_soilPassivePool_to_soilActivePool\t";
            outputSoilNitrogen << "leachingNitrogen\t";
            outputSoilNitrogen << "addedMineralNitrogenToSoilByFertilization\t";
            outputSoilNitrogen << "nitrogenFixationToSoil\t";
            outputSoilNitrogen << "nitrogenGrossMineralization\t";
            outputSoilNitrogen << "nitrogenNetMineralization\t";
            outputSoilNitrogen << "nitrogenVolatilization\t";
            outputSoilNitrogen << "nitrogenFlow_soilActivePool_to_soilPassivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilActivePool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlow_soilMetabolicLitterPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilMicrobesPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlow_soilPassivePool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilSlowPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilSlowPool_to_soilPassivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilStructuralLitterPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilStructuralLitterPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlow_surfaceMetabolicLitterPool_to_soilMicrobesPool\t";
            outputSoilNitrogen << "nitrogenFlow_surfaceStructuralLitterPool_to_soilMicrobesPool\t";
            outputSoilNitrogen << "nitrogenFlow_surfaceStructuralLitterPool_to_soilSlowPool\t";
            outputSoilNitrogen << "nitrogenFlow_soilSlowPool_to_soilActivePool\t";
            outputSoilNitrogen << "nitrogenFlow_soilSlowPool_to_soilPassivePool";
            outputSoilNitrogen << std::endl;
        }
    }

    if (parameter.soilWaterOutputFile)
    {
        if (!outputSoilWater.is_open())
        {
            utils.handleError("Error writing to the soil water output file.");
        }
        else
        {
            outputSoilWater << "Date\tDayCount\t";
            outputSoilWater << "addedWaterToSoilByIrrigation\t";
            outputSoilWater << "interception\t";
            outputSoilWater << "surfaceRunOff\t";
            outputSoilWater << "soilRunOff\t";
            outputSoilWater << "solidSnowContent\t";
            outputSoilWater << "liquidSnowContent\t";
            outputSoilWater << "evaporation\t";
            outputSoilWater << "soilTemperature";
            outputSoilWater << std::endl;
        }
    }

    if (parameter.soilResourcesPerSoilLayerOutputFile)
    {
        if (!outputSoilResourcesPerSoilLayer.is_open())
        {
            utils.handleError("Error writing to the soil resources per soil layer output file.");
        }
        else
        {
            outputSoilResourcesPerSoilLayer << "Date\tDayCount\t";
            outputSoilResourcesPerSoilLayer << "SoilLayerNumber\t";
            outputSoilResourcesPerSoilLayer << "SoilLayerWidth\t";
            outputSoilResourcesPerSoilLayer << "soilWaterFluxDownwardsOutOfSoilLayer\t";
            outputSoilResourcesPerSoilLayer << "waterContent_soilWaterPoolPerSoilLayer\t";
            outputSoilResourcesPerSoilLayer << "nitrogenContent_soilMineralPoolPerSoilLayer";
            outputSoilResourcesPerSoilLayer << std::endl;
        }
    }

    if (parameter.soilFluxesDetailsOutputFile)
    {
        if (!outputSoilFluxesDetails.is_open())
        {
            utils.handleError("Error writing to the soil fluxes details output file.");
        }
        else
        {
            outputSoilFluxesDetails << "Date\tDayCount\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilMicrobesPool_soilSlowPool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilPassivePool_soilActivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilSlowPool_soilActivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilSlowPool_soilPassivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilActivePool_soilSlowPool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilActivePool_soilPassivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilMetabolicLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_surfaceMetabolicLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_surfaceStructuralLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_surfaceStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilStructuralLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "decisiveCNRatio_soilStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "ligninContent_surfaceStructuralLitterPool\t";
            outputSoilFluxesDetails << "ligninContent_soilStructuralLitterPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_surfaceStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_surfaceStructuralLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilStructuralLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_surfaceMetabolicLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilMetabolicLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilMicrobesPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilActivePool_soilPassiveAndSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilSlowPool_soilPassiveAndActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionCarbon_soilPassivePool_soilActivePool\t";
            outputSoilFluxesDetails << "respirationCarbon_surface_litter\t";
            outputSoilFluxesDetails << "respirationCarbon_soil_litter\t";
            outputSoilFluxesDetails << "respirationCarbon_litter\t";
            outputSoilFluxesDetails << "respirationCarbon_soilpools\t";

            // respiratory nitrogen fluxes
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_surfaceStructuralLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilStructuralLitterPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilStructuralLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_surfaceMetabolicLitterPool_soilMicrobesPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilMetabolicLitterPool_soilActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilMicrobesPool_soilSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilActivePool_soilPassiveAndSlowPool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilSlowPool_soilPassiveAndActivePool\t";
            outputSoilFluxesDetails << "respiration_decompositionNitrogen_soilPassivePool_soilActivePool\t";
            outputSoilFluxesDetails << "immobilize_surfaceStructuralLitterPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "immobilize_surfaceStructuralLitterPool_to_soilMicrobesPool\t";
            outputSoilFluxesDetails << "immobilize_soilStructuralLitterPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "immobilize_soilStructuralLitterPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "immobilize_surfaceMetabolicLitterPool_to_soilMicrobesPool\t";
            outputSoilFluxesDetails << "immobilize_soilMetabolicLitterPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "immobilize_soilMicrobesPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "immobilize_soilActivePool_to_soilPassivePool\t";
            outputSoilFluxesDetails << "immobilize_soilActivePool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "immobilize_soilSlowPool_to_soilPassivePool\t";
            outputSoilFluxesDetails << "immobilize_soilSlowPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "immobilize_soilPassivePool_to_soilActivePool\t";
            outputSoilFluxesDetails << "mineralize_surfaceStructuralLitterPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "mineralize_surfaceStructuralLitterPool_to_soilMicrobesPool\t";
            outputSoilFluxesDetails << "mineralize_soilStructuralLitterPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "mineralize_soilStructuralLitterPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "mineralize_surfaceMetabolicLitterPool_to_soilMicrobesPool\t";
            outputSoilFluxesDetails << "mineralize_soilMetabolicLitterPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "mineralize_soilMicrobesPool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "mineralize_soilActivePool_to_soilPassivePool\t";
            outputSoilFluxesDetails << "mineralize_soilActivePool_to_soilSlowPool\t";
            outputSoilFluxesDetails << "mineralize_soilSlowPool_to_soilPassivePool\t";
            outputSoilFluxesDetails << "mineralize_soilSlowPool_to_soilActivePool\t";
            outputSoilFluxesDetails << "mineralize_soilPassivePool_to_soilActivePool\t";
            outputSoilFluxesDetails << "decompositionFactor";
            outputSoilFluxesDetails << std::endl;
        }
    }
}

/**
 * @brief Flushes the in-memory string buffers to the corresponding output files.
 *
 * For each enabled output type, writes the content of the associated
 * `std::ostringstream` buffer to the open `std::ofstream`, then clears the buffer
 * so it is ready for the next time step. This batched write approach reduces
 * the number of filesystem operations during the simulation loop.
 *
 * Calls `utils.handleError()` if an enabled file is not open when this method runs.
 *
 * @param utils     Utility object for error handling.
 * @param parameter Read-only; boolean output-file flags used to guard each block.
 */
void OUTPUT::writeSimulationResultsToOutputFiles(UTILS utils, PARAMETER parameter)
{
    if (parameter.communityOutputFile)
    {
        if (outputCommunity.is_open())
        {
            outputCommunity << bufferCommunity.str();
            bufferCommunity.str("");
            bufferCommunity.clear();
        }
        else
        {
            utils.handleError("The output file of community variables is not open for writing.");
        }
    }

    if (parameter.pftOutputFile)
    {
        if (outputPFTPopulation.is_open())
        {
            outputPFTPopulation << bufferPFTPopulation.str();
            bufferPFTPopulation.str("");
            bufferPFTPopulation.clear();
        }
        else
        {
            utils.handleError("The output file of PFT population variables is not open for writing.");
        }
    }

    if (parameter.plantCohortOutputFile)
    {
        if (outputPlant.is_open())
        {
            outputPlant << bufferPlant.str();
            bufferPlant.str("");
            bufferPlant.clear();
        }
        else
        {
            utils.handleError("The output file of single plant variables / cohorts is not open for writing.");
        }
    }

    if (parameter.soilCarbonOutputFile)
    {
        if (outputSoilCarbon.is_open())
        {
            outputSoilCarbon << bufferSoilCarbon.str();
            bufferSoilCarbon.str("");
            bufferSoilCarbon.clear();
        }
        else
        {
            utils.handleError("The output file of soil carbon variables is not open for writing.");
        }
    }

    if (parameter.soilNitrogenOutputFile)
    {
        if (outputSoilNitrogen.is_open())
        {
            outputSoilNitrogen << bufferSoilNitrogen.str();
            bufferSoilNitrogen.str("");
            bufferSoilNitrogen.clear();
        }
        else
        {
            utils.handleError("The output file of soil nitrogen variables is not open for writing.");
        }
    }

    if (parameter.soilWaterOutputFile)
    {
        if (outputSoilWater.is_open())
        {
            outputSoilWater << bufferSoilWater.str();
            bufferSoilWater.str("");
            bufferSoilWater.clear();
        }
        else
        {
            utils.handleError("The output file of soil water variables is not open for writing.");
        }
    }

    if (parameter.soilResourcesPerSoilLayerOutputFile)
    {
        if (outputSoilResourcesPerSoilLayer.is_open())
        {
            outputSoilResourcesPerSoilLayer << bufferSoilResourcesPerSoilLayer.str();
            bufferSoilResourcesPerSoilLayer.str("");
            bufferSoilResourcesPerSoilLayer.clear();
        }
        else
        {
            utils.handleError("The output file of soil resources per soil layer variables is not open for writing.");
        }
    }

    if (parameter.soilFluxesDetailsOutputFile)
    {
        if (outputSoilFluxesDetails.is_open())
        {
            outputSoilFluxesDetails << bufferSoilFluxesDetails.str();
            bufferSoilFluxesDetails.str("");
            bufferSoilFluxesDetails.clear();
        }
        else
        {
            utils.handleError("The output file of soil fluxes details variables is not open for writing.");
        }
    }
}

/**
 * @brief Closes all open output files at the end of the simulation.
 *
 * Iterates over each enabled output type and closes its `std::ofstream`.
 * Calls `utils.handleError()` if an enabled file is not open when this method
 * is called (which would indicate an earlier failure to open or premature close).
 *
 * @param utils     Utility object for error handling.
 * @param parameter Read-only; boolean output-file flags used to guard each block.
 */
void OUTPUT::closeOutputFiles(UTILS utils, PARAMETER parameter)
{
    if (parameter.communityOutputFile)
    {
        if (outputCommunity.is_open())
        {
            outputCommunity.close();
        }
        else
        {
            utils.handleError("The output file of community variables is not open.");
        }
    }

    if (parameter.pftOutputFile)
    {
        if (outputPFTPopulation.is_open())
        {
            outputPFTPopulation.close();
        }
        else
        {
            utils.handleError("The output file of PFT population variables is not open.");
        }
    }

    if (parameter.plantCohortOutputFile)
    {
        if (outputPlant.is_open())
        {
            outputPlant.close();
        }
        else
        {
            utils.handleError("The output file of single plant variables / cohorts is not open.");
        }
    }

    if (parameter.soilCarbonOutputFile)
    {
        if (outputSoilCarbon.is_open())
        {
            outputSoilCarbon.close();
        }
        else
        {
            utils.handleError("The output file of soil carbon variables is not open.");
        }
    }

    if (parameter.soilNitrogenOutputFile)
    {
        if (outputSoilNitrogen.is_open())
        {
            outputSoilNitrogen.close();
        }
        else
        {
            utils.handleError("The output file of soil nitrogen variables is not open.");
        }
    }

    if (parameter.soilWaterOutputFile)
    {
        if (outputSoilWater.is_open())
        {
            outputSoilWater.close();
        }
        else
        {
            utils.handleError("The output file of soil water variables is not open.");
        }
    }

    if (parameter.soilResourcesPerSoilLayerOutputFile)
    {
        if (outputSoilResourcesPerSoilLayer.is_open())
        {
            outputSoilResourcesPerSoilLayer.close();
        }
        else
        {
            utils.handleError("The output file of soil resources per soil layer variables is not open.");
        }
    }

    if (parameter.soilFluxesDetailsOutputFile)
    {
        if (outputSoilFluxesDetails.is_open())
        {
            outputSoilFluxesDetails.close();
        }
        else
        {
            utils.handleError("The output file of soil fluxes details variables is not open.");
        }
    }
}

/**
 * @brief Creates the `output/` subdirectory adjacent to the configuration file.
 *
 * Splits `path` on the platform path separator, rebuilds all components except
 * the file name, appends `output/`, and calls `_mkdir` (Windows) or `mkdir`
 * (POSIX) to create the directory. If the directory already exists the call
 * is silently ignored. The resulting path is stored in `outputDirectory` for
 * use by createAndOpenOutputFiles().
 *
 * @param path  Absolute path to the main configuration file; the last component
 *              (the file name) is stripped to obtain the base directory.
 * @param utils Utility object for path-separator detection and string splitting.
 */
void OUTPUT::createOutputFolder(std::string path, UTILS utils)
{
    char pathSeparator = utils.getPathSeparator();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 1; it++)
    {
        outputDirectory = outputDirectory + utils.strings.at(it) + pathSeparator;
    }

    outputDirectory = outputDirectory + "output" + pathSeparator;
#ifdef _WIN32
    _mkdir(outputDirectory.c_str());
#else
    mkdir(outputDirectory.c_str(), 0777);
#endif
}
/**
 * @brief Loads the optional list of simulation days on which output should be written.
 *
 * Resolves the output-writing-dates file path relative to `path`, opens the file,
 * skips the header row, and parses each subsequent line as a `YYYY-MM-DD` date.
 * Each date is converted to a day-count offset from `parameter.referenceJulianDayStart`
 * and appended to `outputWritingDates`. Windows CR artifacts are stripped.
 *
 * If the file cannot be opened (e.g. the parameter is `NaN` or the file is absent),
 * a warning is issued and `outputWritingDatesFileOpened` remains `false`, causing the
 * simulation to fall back to daily output writing.
 *
 * @param path      Absolute path to the main configuration file; used to derive the
 *                  directory in which the dates file resides.
 * @param utils     Utility object for string splitting, date conversion, and warnings.
 * @param parameter Read-only; provides `outputWritingDatesFile` name and
 *                  `referenceJulianDayStart` for day-count conversion.
 */
void OUTPUT::openAndReadOutputWritingDates(std::string path, UTILS utils, PARAMETER &parameter)
{
    char pathSeparator = utils.getPathSeparator();
    utils.splitString(path, pathSeparator);
    for (int it = 0; it < utils.strings.size() - 1; it++)
    {
        fileDirectory = fileDirectory + utils.strings.at(it) + pathSeparator;
    }
    fileDirectory = fileDirectory + parameter.outputWritingDatesFile;

    const char *filename = fileDirectory.c_str();
    std::ifstream file(filename);

    std::string line; // current line text in parser
    int m = 0;        // current line number in parser
    outputWritingDates.clear();

    outputWritingDatesFileOpened = false;
    if (file.is_open())
    {
        outputWritingDatesFileOpened = true;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r') /* if windows artifact, remove it*/
            {
                line.pop_back();
            }
            m++;
            if (m > 1)
            { // skip header line
                utils.strings.clear();
                utils.splitString(line, '-');
                int day = std::stoi(utils.strings.at(2).c_str());
                int month = std::stoi(utils.strings.at(1).c_str());
                int year = std::stoi(utils.strings.at(0).c_str());
                outputWritingDates.push_back(utils.calculateDayCountFromDate(day, month, year, parameter.referenceJulianDayStart));
            }
        }
        file.close();
    }
    else
    {
        utils.handleWarning("No OutputWritingDates file has been opened. Simulation results will then be written at a daily resolution.");
    }
}

/**
 * @brief Prints a summary of simulation settings and input-file status to stdout.
 *
 * Outputs the following sections to `std::cout`:
 * - Site identification: DEIMS ID, latitude, longitude, first and last year.
 * - Input files: names of all five input files (weather, soil, management, plant
 *   traits, process setup), each followed by a failure note if the file could
 *   not be opened.
 * - Output writing: the RNG seed for reproducibility, and whether results are
 *   written daily or only on the dates listed in `outputWritingDatesFile`.
 *   Reports if the dates file was absent, empty, or not specified.
 *
 * @param parameter Read-only; provides site metadata, file names, year range, and
 *                  the RNG seed.
 * @param input     Read-only; provides `*FileOpened` boolean flags to report file
 *                  opening success for each input file.
 */
void OUTPUT::printSimulationSettingsToConsole(PARAMETER parameter, INPUT input)
{

    std::cout << "*******************************************" << std::endl;
    std::cout << "******* Grassland site simulation *********" << std::endl;
    std::cout << "*******************************************" << std::endl
              << std::endl;

    std::cout << "DEIMS.id = " << parameter.deimsID << std::endl;
    std::cout << "Latitude = " << parameter.latitude << std::endl;
    std::cout << "Longitude = " << parameter.longitude << std::endl;
    std::cout << "First year = " << parameter.firstYear << std::endl;
    std::cout << "Last year = " << parameter.lastYear << std::endl
              << std::endl;

    std::cout << "******* Input files *********" << std::endl
              << std::endl;

    std::cout << "Weather data: " << parameter.weatherFile << std::endl;
    if (!input.weatherFileOpened)
    {
        std::cout << "File failed to be opened!" << std::endl
                  << std::endl;
    }
    std::cout << "Soil data: " << parameter.soilFile << std::endl;
    if (!input.soilFileOpened)
    {
        std::cout << "File failed to be opened!" << std::endl
                  << std::endl;
    }
    std::cout << "Management data: " << parameter.managementFile << std::endl;
    if (!input.managementFileOpened)
    {
        std::cout << "File failed to be opened!" << std::endl
                  << std::endl;
    }
    std::cout << "Plant traits: " << parameter.plantTraitsFile << std::endl
              << std::endl;
    if (!input.plantTraitsFileOpened)
    {
        std::cout << "File failed to be opened!" << std::endl
                  << std::endl;
    }

    std::cout << "Process setup: " << parameter.processSetupFile << std::endl
              << std::endl;
    if (!input.processSetupFileOpened)
    {
        std::cout << "File failed to be opened!" << std::endl
                  << std::endl;
    }

    std::cout << "******* Simulation output writing *********" << std::endl
              << std::endl;

    std::cout << "Seed of random number generator to reproduce simulation output: " << std::to_string(parameter.randomNumberGeneratorSeed) << std::endl
              << std::endl;

    std::string dates = "daily";
    if (outputWritingDatesFileOpened == true)
    {
        dates = "at dates provided in " + parameter.outputWritingDatesFile;
    }
    std::cout << "Simulation output is written: " << dates << std::endl;
    if (!outputWritingDatesFileOpened)
    {
        if (parameter.outputWritingDatesFile != "NaN" && parameter.outputWritingDatesFile != "")
        {
            std::cout << "File on OutputWritingDates failed to be opened!" << std::endl
                      << std::endl;
        }
        else
        {
            std::cout << "File on OutputWritingDates was not included as parameter!" << std::endl
                      << std::endl;
        }
    }

    std::cout << std::endl;
}
