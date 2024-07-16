#include "mortality.h"

MORTALITY::MORTALITY(){};
MORTALITY::~MORTALITY(){};

/* main mortality function */
void MORTALITY::doPlantMortality(PARAMETER param, STATE &state, UTILS ut)
{
    int pft;

    if (state.community.size() > 0)
    {
        for (int plantIndex = 0; plantIndex < state.community.size(); plantIndex++)
        {
            pft = state.community[plantIndex]->pft;
            // TODO: add crowding and senescence / litter fall
            doSenescenceAndLitterFall();

            state.randomNumberIndex++;
            doThinning();

            state.randomNumberIndex++;
            doBasicMortality(param, ut, state, plantIndex, pft);
        }
    }

    state.checkPlantsAreAliveInCommunity(ut);
}

/* Leaf and root senescence and litter fall */
void MORTALITY::doSenescenceAndLitterFall()
{
}

/* Plant mortality due to thinning of the community */
void MORTALITY::doThinning()
{
}

/* Plant mortality due to instrinsic mortality rate */
void MORTALITY::doBasicMortality(PARAMETER param, UTILS ut, STATE &state, int plantIndex, int pft)
{
    double mortalityRate = getPlantMortalityRate(param, state, plantIndex, pft);

    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::mt19937 gen(state.randomNumberIndex); // generator initialized with the incremental variable
    double randomNumber = dis(gen);

    /* let plants die according to the mortality rate */
    if (randomNumber <= mortalityRate)
    {
        if (state.community[plantIndex]->N > 0)
        {
            state.community[plantIndex]->N -= 1;
        }
        else
        {
            ut.handleError("Error (mortality): no more plants available in the cohort to die.");
        }
    }
}

double MORTALITY::getPlantMortalityRate(PARAMETER param, STATE state, int plantIndex, int pft)
{
    /* check which age plants have and get the correct mortality rate */
    if (state.community[plantIndex]->isAdult)
    {
        return (param.plantMortalityRates[pft]);
    }
    else
    {
        return (param.seedlingMortalityRates[pft]);
    }
}
