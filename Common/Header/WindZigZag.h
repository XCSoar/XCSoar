#ifndef WINDZIGZAG_H
#define WINDZIGZAG_H

#include "XCSoar.h"
#include "Parser.h"

bool WindZigZagUpdate(NMEA_INFO* Basic, DERIVED_INFO* Calculated,
		      double *zzwindspeed, double *zzwindbearing);


#endif
