#ifndef WINDZIGZAG_H
#define WINDZIGZAG_H

#include "XCSoar.h"
#include "Parser.h"

int WindZigZagUpdate(NMEA_INFO* Basic, DERIVED_INFO* Calculated,
		      double *zzwindspeed, double *zzwindbearing);


#endif
