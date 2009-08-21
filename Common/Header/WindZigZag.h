#ifndef WINDZIGZAG_H
#define WINDZIGZAG_H

#include "Calculations.h"
#include "NMEA/Info.h"

int WindZigZagUpdate(NMEA_INFO* Basic, DERIVED_INFO* Calculated,
		      double *zzwindspeed, double *zzwindbearing);


#endif
