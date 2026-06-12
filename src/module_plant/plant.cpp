#include "plant.h"

/**
 * @brief Constructs a PLANT object with default-initialised members.
 *
 * All member variables are value-initialised by their in-class default
 * initialisers declared in plant.h. Actual field values are set by the
 * recruitment module when a new cohort is created
 * (see `RECRUITMENT::createNewPlantCohort()`).
 */
PLANT::PLANT() {};

/** @brief Destructor. No dynamic resources are owned; all members are destroyed automatically. */
PLANT::~PLANT() {};
