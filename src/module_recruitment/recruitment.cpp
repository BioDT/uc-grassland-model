#include "recruitment.h"

RECRUITMENT::RECRUITMENT() {};
RECRUITMENT::~RECRUITMENT() {};

/**
 * @brief Main function for plant recruitment processes.
 *
 * This function orchestrates the recruitment of plants by managing seed
 * influx from various sources, storing the seeds in a local seed pool,
 * and handling the germination of seeds into seedlings.
 *
 * The function performs the following steps:
 * 1. Gathers incoming seeds from plant reproduction, sowing activities,
 *    and external sources.
 * 2. Stores the collected seeds into the local seed pool.
 * 3. Calculates germination based on the stored seeds, accounting for
 *    germination times and rates to determine how many seedlings emerge.
 *
 * @param utils Utility functions for various operations including error
 *              handling and string manipulation.
 * @param parameter A `PARAMETER` object containing simulation parameters
 *                  that influence the recruitment process.
 * @param allometry An `ALLOMETRY` object used for allometric calculations
 *                  relevant to plant growth.
 * @param community Reference to a `COMMUNITY` object representing the
 *                  plant community in the simulation, which will be updated
 *                  with new seedlings.
 * @param management A `MANAGEMENT` object that contains predefined management regimes like
 *                   sowing of seeds for plant recruitment.
 * @param soil Reference to a `SOIL` object representing the soil characteristics.
 */
void RECRUITMENT::doPlantRecruitment(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, MANAGEMENT management, SOIL &soil)
{
   // 1. seed influx by different seed sources
   getIncomingSeedsByPlantReproduction(parameter, community);
   getIncomingSeedsBySowing(parameter, management);
   getIncomingSeedsByExternalInflux(parameter);

   // 2. storage of seed influx to local seed pool
   saveIncomingSeedsInSeedPool(parameter);

   // 3. seed germination from seed pool accounting for germination times and rates
   calculateSeedGerminationToSeedlings(utils, parameter, allometry, community, soil);

   // 4. Update number of cohorts in allPlants-vector
   community.totalNumberOfCohortsInCommunity = community.allPlants.size();
}

/**
 * @brief Handles the recruitment of seeds from an external area.
 *
 * This function simulates the influx of seeds into the local seed pool
 * from an external source based on predefined parameters. The process
 * occurs only if the external seed influx feature is activated and the
 * current simulation day meets the threshold for starting the influx.
 *
 * The function performs the following checks and operations:
 * - Verifies if the external seed influx is activated.
 * - Checks if the current day is greater than or equal to the specified
 *   start day for external seed influx as defined in the plant traits file.
 * - If both conditions are met, it iterates through the different plant
 *   functional types (PFTs) and adds the number of incoming seeds from
 *   the external influx to the local `incomingSeeds` vector for each PFT.
 *
 * @param parameter A `PARAMETER` object containing relevant simulation
 *                  parameters, including flags and numbers associated
 *                  with the external seed influx.
 */
void RECRUITMENT::getIncomingSeedsByExternalInflux(PARAMETER parameter)
{
   if (parameter.externalSeedInfluxActivated && parameter.day >= parameter.dayOfExternalSeedInfluxStart)
   {
      for (int pft = 0; pft < parameter.pftCount; pft++)
      {
         incomingSeeds[pft] += parameter.externalSeedInfluxNumber[pft];
      }
   }
}

/**
 * @brief Handles the recruitment of seeds via sowing activities.
 *
 * This function processes the influx of seeds that are sown on specified
 * days as defined in the management file. It checks whether the current
 * simulation day matches any of the sowing days and, if so, updates the
 * local seed pool by adding the amount of sown seeds for each plant
 * functional type (PFT).
 *
 * The function performs the following operations:
 * - Checks if there are any specified sowing dates in the management
 *   file.
 * - Iterates through the sowing dates to find any that match the
 *   current simulation day.
 * - If a match is found, it updates the `incomingSeeds` vector by
 *   adding the amount of seeds sown for each PFT on that day.
 *
 * @param parameter A `PARAMETER` object that contains simulation parameters,
 *                  including the current day of the simulation.
 * @param management A `MANAGEMENT` object that provides information about
 *                   sowing dates and the corresponding amounts of seeds
 *                   sown for each PFT.
 */
void RECRUITMENT::getIncomingSeedsBySowing(PARAMETER parameter, MANAGEMENT management)
{
   if (management.sowingDate.size() > 0)
   {
      /* for each sowing day from the management file */
      for (int sowingDayIndex = 0; sowingDayIndex < management.sowingDate.size(); sowingDayIndex++)
      {
         /* if the current day is exactly a sowing day */
         if (parameter.day == management.sowingDate[sowingDayIndex])
         {
            for (int pft = 0; pft < parameter.pftCount; pft++)
            {
               incomingSeeds[pft] += management.amountOfSownSeeds[pft][sowingDayIndex];
            }
         }
      }
   }
}

/**
 * @brief Handles the recruitment of seeds via seed production from mature plants.
 *
 * This function processes the influx of seeds produced by mature plants
 * in the community. It checks if the plant seed production is activated
 * and iterates through all plants to determine if they are mature enough
 * to produce seeds. If mature, it calculates the number of seeds based
 * on the plant's recruitment biomass and the specific seed mass for the
 * corresponding plant functional type (PFT). The produced seeds are then
 * added to the local seed pool for each PFT.
 *
 * The function performs the following operations:
 * - Checks if seed production is activated in the simulation parameters.
 * - Iterates over all plants in the community.
 * - For each plant, checks if it has reached maturity based on its height.
 * - If mature, calculates the number of seeds produced using the
 *   recruitment biomass and updates the `incomingSeeds` vector accordingly.
 * - Resets the plant's recruitment biomass to zero after seed production.
 *
 * @param parameter A `PARAMETER` object that contains simulation parameters,
 *                  including flags for seed production activation and
 *                  maturity height thresholds for each PFT.
 * @param community A `COMMUNITY` object that holds information about all
 *                  plants, including their characteristics and biomass.
 */
void RECRUITMENT::getIncomingSeedsByPlantReproduction(PARAMETER parameter, COMMUNITY &community)
{
   int pft, numberOfSeeds;

   if (parameter.plantSeedProductionActivated)
   {
      for (int plantIndex = 0; plantIndex < community.totalNumberOfCohortsInCommunity; plantIndex++)
      {
         /* new plant cohorts are stored at the end of the community vector */
         pft = community.allPlants[plantIndex]->pft;

         /* if plants have reached maturity, their recruitment biomass pool is used for seed production (based on PFT-specific seed mass) */
         // TODO: could this be simplified by using nppAllocationToReproduction?
         if (community.allPlants[plantIndex]->height >= parameter.maturityHeights[pft - 1])
         {
            if (community.allPlants[plantIndex]->recruitmentBiomass > 0)
            {
               numberOfSeeds = (int)floor((community.allPlants[plantIndex]->recruitmentBiomass / parameter.seedMasses[pft - 1]) + 0.5);
               incomingSeeds[pft] += numberOfSeeds;
               community.allPlants[plantIndex]->recruitmentBiomass = 0;

               // TODO: update C and N fluxes
               //  patch->nitrogenContentReproduction += plant->nitrogenContentReproduction;
               //  plant->nitrogenContentReproduction = 0;
            }
         }
      }
   }
}

/**
 * @brief Saves the incoming seeds into the local seed pool.
 *
 * This function takes the number of incoming seeds for each plant functional
 * type (PFT) and adds them to the corresponding seed pool. It also records
 * the germination time for each PFT based on the specified germination times
 * in the parameters. This allows for later tracking of seed germination
 * processes within the simulation.
 *
 * The function performs the following operations:
 * - Iterates over each plant functional type (PFT) defined in the parameter.
 * - Checks if there are any incoming seeds for the current PFT.
 * - If seeds are present, adds the amount of incoming seeds to the
 *   `seedPool` for that PFT.
 * - Simultaneously, the corresponding seed germination time is pushed to
 *   the `seedGerminationTimeCounter` for that PFT.
 *
 * @param parameter A `PARAMETER` object that contains simulation parameters,
 *                  including the number of plant functional types (PFTs)
 *                  and their respective germination times.
 */
void RECRUITMENT::saveIncomingSeedsInSeedPool(PARAMETER parameter)
{
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      if (incomingSeeds[pft] > 0)
      {
         seedPool[pft].push_back(incomingSeeds[pft]);
         seedGerminationTimeCounter[pft].push_back(parameter.seedGerminationTimes[pft]);
      }
   }
}

/**
 * @brief Calculates the germination of seeds from the seed pool into seedlings.
 *
 * This function processes the germination of seeds for each plant functional
 * type (PFT) based on their respective germination times. For each PFT,
 * the function decreases the germination time counter for each seed cohort
 * and checks if the seeds are ready to germinate. If so, it calculates the
 * number of successfully germinated seeds, transfers failed seeds to the
 * litter pool, and updates the seed pool accordingly. Additionally, it
 * adds the successfully germinated seedlings to the community.
 *
 * The function performs the following operations:
 * - Iterates over each plant functional type (PFT).
 * - For each cohort of seeds, decreases the germination time counter by one
 *   to account for the passage of a day.
 * - Checks if the counter has reached zero, indicating that the seeds are
 *   ready to germinate.
 * - Calculates the number of successfully germinated seeds and handles the
 *   transfer of failed germinated seeds to the litter pool.
 * - Updates the seed pool to reflect the number of seeds that have germinated.
 * - Adds newly germinated seedlings to the community vector of all plants.
 *
 * @param utils An object of type `UTILS` that provides utility functions
 *              used in the seed germination process.
 * @param parameter A `PARAMETER` object that contains simulation parameters,
 *                  including the number of plant functional types (PFTs) and
 *                  germination times.
 * @param allometry An `ALLOMETRY` object that contains information related
 *                  to plant growth and structure.
 * @param community A `COMMUNITY` object that represents the current plant
 *                  community, including all existing plants.
 * @param soil A `SOIL` object that contains information about the soil
 *             environment.
 */
void RECRUITMENT::calculateSeedGerminationToSeedlings(UTILS utils, PARAMETER parameter, ALLOMETRY allometry, COMMUNITY &community, SOIL &soil)
{
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      for (int cohortindex = 0; cohortindex < seedGerminationTimeCounter[pft].size(); cohortindex++)
      {
         seedGerminationTimeCounter[pft].at(cohortindex) -= 1;     // account for this day for germination by decreasing counter by one
         if (seedGerminationTimeCounter[pft].at(cohortindex) == 0) /* only if counter is 0 seeds are now ready to germinate as seedlings */
         {
            // calculate number of successful germinated seeds from seedpool
            calculateNumberOfGerminatingSeeds(utils, parameter, community, pft, cohortindex);

            // calculate number of failed germinated seeds from seedpool and transfer to litter pool
            transferFailedToGerminateSeedsToLitterPool(utils, parameter, soil, pft, cohortindex);

            // update seed pool after germination
            updateSeedPool(pft, cohortindex);

            // TODO: think about how / whether to include competition for space here?
            // create plant entry in community vector 'allPlants' for the successfully germinated seeds
            // if ((ATSum[seedlingCrownCellIndex] < 1.0))
            //{
            /*if (parameter.crowdingMortalityActivated): TODO: maybe exchange by thinning rule?
            {
               correctSeedsForSpace;
            }*/

            addGerminatedSeedlingsToCommunity(parameter, community, allometry, pft);
            //}
         }
      }
   }
}

/**
 * @brief Calculates the number of successfully germinated seeds from the seed pool.
 *
 * This function determines the number of seeds that successfully germinate
 * based on the current seed pool and the germination rate specific to the
 * given plant functional type (PFT). If the calculated number of successfully
 * germinated seeds is greater than zero, it is returned. If the calculation
 * results in a negative value, an error is handled through the provided
 * utility function.
 *
 * @param utils An object of type `UTILS` that provides utility functions
 *              used for error handling.
 * @param parameter A `PARAMETER` object that contains simulation parameters,
 *                  including seed germination rates for each PFT.
 * @param pft An integer representing the index of the plant functional type
 *            for which the germination is calculated.
 * @param cohortindex An integer representing the index of the seed cohort
 *                    within the seed pool for the specified PFT.
 *
 * @return The number of successfully germinated seeds.
 *
 * @throw std::runtime_error If the calculated number of successfully
 *                            germinated seeds is negative.
 */
void RECRUITMENT::calculateNumberOfGerminatingSeeds(UTILS utils, PARAMETER parameter, COMMUNITY &community, int pft, int cohortindex)
{
   // TODO: add impact of heat, soil water and nitrogen to classical germination rate
   int integerPartOfCalculatedNumberOfSeeds;
   double calculatedNumberOfSeeds;
   calculatedNumberOfSeeds = seedPool[pft].at(cohortindex) * parameter.seedGerminationRates[pft];
   integerPartOfCalculatedNumberOfSeeds = std::floor(calculatedNumberOfSeeds);

   double randomNumber = -1;
   community.randomNumberIndex++;
   // stochasticity in ceiling or flooring of the calculated number of germinating seeds if not integer
   if ((calculatedNumberOfSeeds - integerPartOfCalculatedNumberOfSeeds) > 0)
   {
      std::uniform_real_distribution<> dis(0.0, 1.0);
      std::mt19937 gen(community.randomNumberIndex); // generator initialized with the incremental variable
      randomNumber = dis(gen);
      if (randomNumber <= (calculatedNumberOfSeeds - integerPartOfCalculatedNumberOfSeeds))
      {
         successfullGerminatedSeeds.at(pft) = integerPartOfCalculatedNumberOfSeeds + 1;
      }
      else
      {
         successfullGerminatedSeeds.at(pft) = integerPartOfCalculatedNumberOfSeeds;
      }
   }
   else
   {
      successfullGerminatedSeeds.at(pft) = integerPartOfCalculatedNumberOfSeeds;
   }

   if (successfullGerminatedSeeds.at(pft) < 0)
   {
      utils.handleError("Calculation of germinating seeds return a negative value!");
   }
}

/**
 * @brief Transfers failed-to-germinate seeds to the litter pool for decomposition.
 *
 * This function calculates the number of seeds that failed to germinate by
 * subtracting the number of successfully germinated seeds from the total
 * number of seeds in the specified cohort of the seed pool. It then transfers
 * the carbon and nitrogen content of these failed seeds to the litter pool
 * for further decomposition.
 *
 * It also checks for consistency in the number of seeds, ensuring that the
 * sum of successfully germinated and failed-to-germinate seeds equals the
 * original number of seeds in the cohort. If there is significant numerical
 * variation due to rounding, a warning is issued.
 *
 * @param utils An object of type `UTILS` used for utility functions and
 *              handling warnings.
 * @param parameter A `PARAMETER` object that holds simulation parameters,
 *                  including seed masses.
 * @param soil A reference to a `SOIL` object that manages the transfer
 *             of organic material to litter pools.
 * @param successfullGerminatedSeeds The number of seeds that successfully
 *                                    germinated.
 * @param pft An integer representing the index of the plant functional type
 *            for which the seeds are processed.
 * @param cohortindex An integer representing the index of the seed cohort
 *                    within the seed pool for the specified PFT.
 *
 * @throw std::runtime_error If the sum of germinated and failed seeds does
 *                            not match the total seeds, a warning is logged
 *                            for numerical consistency.
 */
void RECRUITMENT::transferFailedToGerminateSeedsToLitterPool(UTILS utils, PARAMETER parameter, SOIL &soil, int pft, int cohortindex)
{
   /* calculate number of failed germinated seeds from seedpool */
   int failedToGerminateSeeds;
   failedToGerminateSeeds = seedPool[pft].at(cohortindex) - successfullGerminatedSeeds.at(pft);

   /* check for consistency */
   if (std::abs((successfullGerminatedSeeds.at(pft) + failedToGerminateSeeds) - seedPool[pft].at(cohortindex)) >= tolerance)
   {
      utils.handleWarning("There is more numerical variation in the rounding of germinated / non-germinated seeds than expected.");
   }

   /* transfer carbon and nitrogen content of failed seeds to the respective litter pools for decomposition */
   soil.transferDyingPlantPartsToLitterPools(parameter, failedToGerminateSeeds, parameter.seedMasses[pft], 3, pft);
}

/**
 * @brief Updates the seed pool by removing entries for completed germination processes.
 *
 * This function deletes the entries from both the seed pool and the
 * seed germination time counter vectors for the specified plant functional type
 * (PFT) and cohort index. This is done after the germination process is
 * completed, indicating that those seeds are no longer in the seed pool
 * and their germination time has been accounted for.
 *
 * @param pft An integer representing the index of the plant functional type
 *            whose seed pool and germination time counter are being updated.
 * @param cohortindex An integer representing the index of the seed cohort
 *                    to be removed from the seed pool and time counter.
 */
void RECRUITMENT::updateSeedPool(int pft, int cohortindex)
{
   // delete entry in both vectors (as germination process is now completed for those seeds from the seedpool)
   seedGerminationTimeCounter[pft].erase(seedGerminationTimeCounter[pft].begin() + cohortindex);
   seedPool[pft].erase(seedPool[pft].begin() + cohortindex);
}

/**
 * @brief Adds successfully germinated seedlings to the community.
 *
 * This function creates new plant objects for the successfully germinated
 * seedlings and adds them to the community's collection of plants.
 * The new plant is initialized with the given parameters, including the
 * plant functional type (PFT) and the number of successful seedlings.
 *
 * @param parameter A reference to a PARAMETER object containing the parameters
 *                  necessary for initializing the new plant.
 * @param community A reference to the COMMUNITY object representing the
 *                  current state of the plant community. The new seedlings
 *                  will be added to this community.
 * @param allometry An ALLOMETRY object used for allometric calculations
 *                  to initialize a new seedlings state variables.
 * @param pft An integer representing the index of the plant functional type
 *            for the newly added seedlings.
 * @param successfullGerminatedSeeds An integer representing the number of
 *                                    successfully germinated seedlings to be added.
 */
void RECRUITMENT::addGerminatedSeedlingsToCommunity(PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, int pft)
{
   if (successfullGerminatedSeeds.at(pft) > 0)
   {
      community.allPlants.emplace_back(std::make_shared<PLANT>(parameter, allometry, pft, seedlingHeight, successfullGerminatedSeeds.at(pft)));

      // TODO: only relevant for calculating Cflux
      // biomass = TreeToGrass(plant)->biomassShoot;
      // ingrowth_month += biomass * successfullGerminatedSeeds;
   }
}
