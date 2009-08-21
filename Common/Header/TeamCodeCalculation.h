#ifndef	TEAMCALCULATION_H
#define	TEAMCALCULATION_H

#include <tchar.h>

void GetTeamCode(TCHAR *code, double bearing, double range);
void CalcTeammateBearingRange(double ownDist, double ownBear, TCHAR *TeamMateCode,  double *distToMate, double *bearToMate);
double GetTeammateBearingFromRef(TCHAR *code );
double GetTeammateRangeFromRef(TCHAR *code );

#endif
