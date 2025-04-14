#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
#include "../module_growth/growth.h"
#include "../utils/utils.h"
#include <random>

/**
 * @class MORTALITY
 * @brief Manages plant mortality processes within a plant community simulation.
 *
 * The MORTALITY class contains methods for handling various aspects of plant
 * mortality, including basic mortality, senescence, litter fall, and thinning.
 * It provides functionality to determine mortality probabilities based on plant age
 * and plant functional type (PFT).
 */
class MORTALITY
{
public:
   MORTALITY();
   ~MORTALITY();

   void doPlantMortality(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL soil);
   void doSenescenceAndLitterFall(UTILS utils, PARAMETER parameter, COMMUNITY &community, ALLOMETRY allometry, GROWTH growth, INTERACTION interaction, SOIL soil, int cohortIndex, int pft);
   double doLeafSenescence(COMMUNITY &community, PARAMETER parameter, GROWTH growth, INTERACTION interaction, int cohortIndex, int pft);
   void doLeafLitterFall(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, SOIL soil, int cohortIndex, int pft);
   void updatePlantSize(UTILS utils, COMMUNITY &community, ALLOMETRY allometry, PARAMETER parameter, int fractionLeavesFalling, int cohortIndex, int pft);
   void doNitrogenRelocation(UTILS utils, COMMUNITY &community, PARAMETER parameter);
   void doRootSenescenceAndLitterFall(COMMUNITY &community, PARAMETER parameter, SOIL soil, int cohortIndex, int pft);
   void doThinning();
   void doBasicMortality(PARAMETER parameter, UTILS utils, COMMUNITY &community, int cohortIndex, int pft);
   double getPlantMortalityProbability(PARAMETER parameter, COMMUNITY community, int cohortIndex, int pft);
};