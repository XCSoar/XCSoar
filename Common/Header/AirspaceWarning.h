#ifndef AIRSPACE_WARNING_H
#define AIRSPACE_WARNING_H

#include "Calculations.h"

extern void AirspaceWarnListAdd(NMEA_INFO *Basic,
                                bool Predicted, bool IsCircle, int AsIdx,
                                bool ackDay=false);

extern void AirspaceWarnListProcess(NMEA_INFO *Basic);


#endif
