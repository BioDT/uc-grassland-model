#include "parameter.h"

/**
 * @brief Constructs a PARAMETER object with default-initialised members.
 *
 * All member variables are value-initialised by their in-class default initialisers
 * declared in parameter.h. Actual parameter values are populated later by
 * `INPUT::transferConfigParameterValueToModelParameter()`,
 * `INPUT::transferPlantTraitsParameterValueToModelParameter()`, and
 * `INPUT::transferProcessSetupParameterValueToModelParameter()`.
 */
PARAMETER::PARAMETER() {};

/** @brief Destructor. No dynamic resources are owned; all members are destroyed automatically. */
PARAMETER::~PARAMETER() {};
