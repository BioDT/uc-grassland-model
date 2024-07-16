#include "init.h"

INIT::INIT(){};
INIT::~INIT(){};

/* main function to initialize state variables of the simulation start */
void INIT::initModelSimulation(PARAMETER &param, STATE &state)
{
    /* init time variables */
    initTimeVariables(param);

    /* init random number generator seed */
    initRandomNumberGeneratorSeed(param, state);

    /* init state variables of community */
    initCommunityStateVariables(state, param);
}

/* initialization of state variables of time */
void INIT::initTimeVariables(PARAMETER &param)
{
    param.day = 1;                    // start with the first day
    param.numberOfDaysToSimulate = 0; // calculate number of days to simulate considering leap years
    for (int year = param.firstYear; year <= param.lastYear; year++)
    {
        if (year % 4)
        {
            param.numberOfDaysToSimulate += 366;
        }
        else
        {
            param.numberOfDaysToSimulate += 365;
        }
    }
}

/* initialization of random number generator seed */
void INIT::initRandomNumberGeneratorSeed(PARAMETER &param, STATE &state)
{
    if (!param.reproducibleResults)
    {
        std::random_device rd; // seed generator
        param.randomNumberGeneratorSeed = rd();
    }

    state.randomNumberIndex = param.randomNumberGeneratorSeed;
}

/* initialization of the community vector and grassland state variables */
void INIT::initCommunityStateVariables(STATE &state, PARAMETER param)
{
    state.community.clear();
    state.totalNumberOfPlantsInCommunity = 0;
    state.pftComposition.clear();
    for (int pft = 0; pft < param.numberOfSpecies; pft++)
    {
        state.pftComposition.push_back(0);
    }
}