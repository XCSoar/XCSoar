#ifndef WINDZIGZAG_H
#define WINDZIGZAG_H

#include "Calculations.h"
#include "Parser.h"

int WindZigZagUpdate(NMEA_INFO* Basic, DERIVED_INFO* Calculated,
		      double *zzwindspeed, double *zzwindbearing);


#endif
