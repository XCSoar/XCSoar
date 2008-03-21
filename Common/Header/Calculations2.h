#ifndef CALCULATIONS2_H
#define CALCULATIONS2_H
#include "Calculations.h"

void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void AddSnailPoint(void);

double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic,
				DERIVED_INFO *Calculated, double *retlat,
				double *retlon,
				double maxsearchrange,
				bool *outofrange);

double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
		      double bearing,
		      double GlideSlope);

double EffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
double EffectiveCruiseEfficiency(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

double MacCreadyTimeLimit(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			  double bearing,
			  double timeremaining,
			  double hfinal);

void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void CalculateTeammateBearingRange(NMEA_INFO *Basic, DERIVED_INFO *Calculated) ;

void CalibrationInit(void);
void CalibrationUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void CalibrationSave(void);

#endif
