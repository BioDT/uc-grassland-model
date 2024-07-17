#include "growth.h"

GROWTH::GROWTH(){};
GROWTH::~GROWTH(){};

/* main function of plant growth */
void GROWTH::doPlantGrowth(PARAMETER parameter, COMMUNITY &community)
{
    int pft;
    if (community.allPlants.size() > 0)
    {
        for (int plantIndex = 0; plantIndex < community.allPlants.size(); plantIndex++)
        {
            pft = community.allPlants[plantIndex]->pft;
            if (community.allPlants[plantIndex]->N > 0)
            {
                // TODO: add correct growth processes
                community.allPlants[plantIndex]->height += 1;
                community.allPlants[plantIndex]->age += 1;
            }
        }
    }
}