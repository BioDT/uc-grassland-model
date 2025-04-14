#include "init.h"

INIT::INIT() {};
INIT::~INIT() {};

/* main function to initialize state variables of the simulation start */
void INIT::initModelSimulation(PARAMETER &parameter, COMMUNITY &community, RECRUITMENT &recruitment, SOIL &soil, INTERACTION &interaction)
{
   /* init time variables */
   initTimeVariables(parameter);

   /* init random number generator seed */
   initRandomNumberGeneratorSeed(parameter, community);

   /* init state variables of community */
   initStateVariables(community, parameter, recruitment, soil);

   /* init process-specific state variables */
   initAndResetProcessVariables(parameter, recruitment, community, interaction);
}

/* initialization of state variables of time */
void INIT::initTimeVariables(PARAMETER &parameter)
{
   parameter.day = 1; // start with the first day
}

/* initialization of random number generator seed */
void INIT::initRandomNumberGeneratorSeed(PARAMETER &parameter, COMMUNITY &community)
{
   if (parameter.randomNumberGeneratorSeed == std::numeric_limits<int>::min())
   {
      std::random_device rd; // seed generator
      parameter.randomNumberGeneratorSeed = rd();
   }
   community.randomNumberIndex = parameter.randomNumberGeneratorSeed;
}

/* initialization of the community vector and grassland state variables */
void INIT::initStateVariables(COMMUNITY &community, PARAMETER parameter, RECRUITMENT &recruitment, SOIL &soil)
{
   // simulation-related variables
   community.allPlants.clear();
   community.totalNumberOfCohortsInCommunity = 0;

   /* Recruitment variables */
   recruitment.seedPool.clear();
   recruitment.seedGerminationTimeCounter.clear();
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      recruitment.seedPool.push_back(std::vector<int>());
      recruitment.seedPool[pft].clear();
      recruitment.seedGerminationTimeCounter.push_back(std::vector<int>());
      recruitment.seedGerminationTimeCounter[pft].clear();
   }

   /* Mortality variables */
   soil.greenCarbonSurfaceLitter = 0;
   soil.brownCarbonSurfaceLitter = 0;
   soil.rootCarbonSoilLitter = 0;
   soil.seedCarbonSoilLitter = 0;
   soil.greenNitrogenSurfaceLitter = 0;
   soil.brownNitrogenSurfaceLitter = 0;
   soil.rootNitrogenSoilLitter = 0;
   soil.seedNitrogenSoilLitter = 0;
}

void INIT::initAndResetProcessVariables(PARAMETER parameter, RECRUITMENT &recruitment, COMMUNITY &community, INTERACTION &interaction)
{
   /// Process-related variables
   // 1. Recruitment
   recruitment.incomingSeeds.clear();
   recruitment.outgoingSeeds.clear();
   recruitment.successfullGerminatedSeeds.clear();
   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      recruitment.incomingSeeds.push_back(0);
      recruitment.outgoingSeeds.push_back(0);
      recruitment.successfullGerminatedSeeds.push_back(0);
   }

   // 2. Light availability (interaction)
   interaction.LAI.clear();
   interaction.LAIwithLightExtinction.clear();
   for (int layerindex = 0; layerindex <= maximumHeightLayer; layerindex++)
   {
      interaction.LAI.push_back(0.0);
      interaction.LAIwithLightExtinction.push_back(0.0);
   }
   community.maximumHeightOfAllPlants = 0;

   /// Output-related variables
   community.totalNumberOfPlantsInCommunity = 0;
   community.leafAreaIndexOfPlantsInCommunity = 0;
   community.greenYield = 0.0;
   community.brownYield = 0.0;
   community.yield = 0.0;

   community.numberOfPlantsPerPFT.clear();
   community.pftComposition.clear();
   community.coveredAreaOfPlantsPerPFT.clear();
   community.shootBiomassOfPlantsPerPFT.clear();
   community.brownShootBiomassOfPlantsPerPFT.clear();
   community.greenShootBiomassOfPlantsPerPFT.clear();
   community.clippedShootBiomassOfPlantsPerPFT.clear();
   community.rootBiomassOfPlantsPerPFT.clear();
   community.recruitmentBiomassOfPlantsPerPFT.clear();
   community.exudationBiomassOfPlantsPerPFT.clear();
   community.gppOfPlantsPerPFT.clear();
   community.nppOfPlantsPerPFT.clear();
   community.respirationOfPlantsPerPFT.clear();
   community.greenBiomassYield.clear();
   community.brownBiomassYield.clear();
   community.biomassYield.clear();

   for (int pft = 0; pft < parameter.pftCount; pft++)
   {
      community.pftComposition.push_back(0);
      community.numberOfPlantsPerPFT.push_back(0);

      community.coveredAreaOfPlantsPerPFT.push_back(0);
      community.shootBiomassOfPlantsPerPFT.push_back(0);
      community.brownShootBiomassOfPlantsPerPFT.push_back(0);
      community.greenShootBiomassOfPlantsPerPFT.push_back(0);
      community.clippedShootBiomassOfPlantsPerPFT.push_back(0);
      community.rootBiomassOfPlantsPerPFT.push_back(0);
      community.recruitmentBiomassOfPlantsPerPFT.push_back(0);
      community.exudationBiomassOfPlantsPerPFT.push_back(0);
      community.gppOfPlantsPerPFT.push_back(0);
      community.nppOfPlantsPerPFT.push_back(0);
      community.respirationOfPlantsPerPFT.push_back(0);

      community.greenBiomassYield.push_back(0);
      community.brownBiomassYield.push_back(0);
      community.biomassYield.push_back(0);
   }
}