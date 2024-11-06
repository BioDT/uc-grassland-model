#pragma once
#include "../module_plant/community.h"
#include "../module_parameter/parameter.h"
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

   void doPlantMortality(PARAMETER parameter, COMMUNITY &community, UTILS utils);
   void doSenescenceAndLitterFall();
   void doThinning();
   void doBasicMortality(PARAMETER parameter, UTILS utils, COMMUNITY &community, int cohortIndex, int pft);
   double getPlantMortalityProbability(PARAMETER parameter, COMMUNITY community, int cohortIndex, int pft);
};