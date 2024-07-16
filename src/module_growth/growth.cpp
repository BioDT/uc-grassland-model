#include "growth.h"

GROWTH::GROWTH(){};
GROWTH::~GROWTH(){};

/* main function of plant growth */
void GROWTH::doPlantGrowth(PARAMETER param, STATE &state)
{
    int pft;
    if (state.community.size() > 0)
    {
        for (int plantIndex = 0; plantIndex < state.community.size(); plantIndex++)
        {
            pft = state.community[plantIndex]->pft;
            if (state.community[plantIndex]->N > 0)
            {
                // TODO: add correct growth processes
                state.community[plantIndex]->height += 1;
                state.community[plantIndex]->age += 1;
            }
        }
    }
}