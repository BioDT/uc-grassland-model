#include "recruitment.h"

RECRUITMENT::RECRUITMENT(){};
RECRUITMENT::~RECRUITMENT(){};

/* main function of plant recruitment */
void RECRUITMENT::doPlantRecruitment(PARAMETER param, ALLOMETRY allo, STATE &state, MANAGEMENT manage)
{
    // TODO: germination rate, times, seed pool
    createSeedlingsByExternalInflux(param, allo, state);
    createSeedlingsBySowing(param, allo, state, manage);
    createSeedlingsByPlantReproduction(param, allo, state);
}

/* plant recruitment via seed influx from an outside area */
void RECRUITMENT::createSeedlingsByExternalInflux(PARAMETER param, ALLOMETRY allo, STATE &state)
{
    /* process is only done when activated and current day reached seed influx start (species file parameter) */
    if (param.externalSeedInfluxActivated && param.day >= param.dayOfExternalSeedInfluxStart)
    {
        for (int pft = 0; pft < param.numberOfSpecies; pft++)
        {
            /* new plant cohorts are stored at the end of the community vector */
            state.community.emplace_back(std::make_shared<PLANT>(param, allo, pft, seedlingHeight, param.externalSeedInfluxNumber[pft]));
        }
    }
}

/* plant recruitment via seed sowing as management activity (management input file) */
void RECRUITMENT::createSeedlingsBySowing(PARAMETER param, ALLOMETRY allo, STATE &state, MANAGEMENT manage)
{
    if (manage.sowingDate.size() > 0)
    {
        /* for each sowing day from the management file */
        for (int sowingDayIndex = 0; sowingDayIndex < manage.sowingDate.size(); sowingDayIndex++)
        {
            /* if the current day is exactly a sowing day */
            if (param.day == manage.sowingDate[sowingDayIndex])
            {
                for (int pft = 0; pft < param.numberOfSpecies; pft++)
                {
                    state.community.emplace_back(std::make_shared<PLANT>(param, allo, pft, seedlingHeight, manage.amountOfSownSeeds[pft][sowingDayIndex]));
                }
            }
        }
    }
}

/* plant recruitment via seed production from mature plants */
void RECRUITMENT::createSeedlingsByPlantReproduction(PARAMETER param, ALLOMETRY allo, STATE &state)
{
    int pft, numberOfSeeds;

    /* process is only done if activated (species parameter file) */
    if (param.plantSeedProductionActivated)
    {
        for (int plantIndex = 0; plantIndex < state.community.size(); plantIndex++)
        {
            /* new plant cohorts are stored at the end of the community vector */
            pft = state.community[plantIndex]->pft;

            /* if plants have reached maturity, their recruitment biomass pool is used for seed production (based on PFT-specific seed mass) */
            if (state.community[plantIndex]->age >= param.maturityAges[pft - 1])
            {
                if (state.community[plantIndex]->recruitmentBiomass > 0)
                {
                    numberOfSeeds = (int)floor((state.community[plantIndex]->recruitmentBiomass / param.seedMasses[pft - 1]) + 0.5);
                    state.community.emplace_back(std::make_shared<PLANT>(param, allo, pft, seedlingHeight, numberOfSeeds));
                }
            }
        }
    }
}