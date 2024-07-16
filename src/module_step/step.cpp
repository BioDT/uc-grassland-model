#include "step.h"

STEP::STEP(){};
STEP::~STEP(){};

/* Simulates the entire simulation period */
void STEP::runModelSimulation(UTILS ut, PARAMETER &param, ALLOMETRY allo, STATE &state, RECRUITMENT recruit, MORTALITY death, GROWTH grow, MANAGEMENT manage, OUTPUT &out)
{
    /* Perform daily plant processes for each day to be simulated */
    for (int day = 1; day <= param.numberOfDaysToSimulate; day++)
    {
        param.day = day; // increase day according to for-loop
        doDayStepOfModelSimulation(ut, param, allo, state, recruit, death, grow, manage);

        /* Writing of daily output of simulation results */
        out.writeSimulationResultsToOutputFiles(param, ut, state);
    }
}

/* Performs one day step of all plant processes */
void STEP::doDayStepOfModelSimulation(UTILS ut, PARAMETER &param, ALLOMETRY allo, STATE &state, RECRUITMENT recruit, MORTALITY death, GROWTH grow, MANAGEMENT manage)
{
    /* Plant recruitment */
    recruit.doPlantRecruitment(param, allo, state, manage);

    /* Plant mortality */
    death.doPlantMortality(param, state, ut);

    /* Plant growth */
    grow.doPlantGrowth(param, state);

    /* Calculation of dynamic state variables of the community */
    state.updateCommunityStateVariables(param);
}
