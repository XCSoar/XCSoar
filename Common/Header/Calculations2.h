#ifndef CALCULATIONS2_H
#define CALCULATIONS2_H
#include "Calculations.h"

void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void AddSnailPoint(void);

double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
		      const double bearing,
		      const double GlideSlope);

double EffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
double EffectiveCruiseEfficiency(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

double MacCreadyTimeLimit(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			  const double bearing,
			  const double timeremaining,
			  const double hfinal);

void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void CalculateTeammateBearingRange(NMEA_INFO *Basic, DERIVED_INFO *Calculated) ;

void CalibrationInit(void);
void CalibrationUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void CalibrationSave(void);

#endif
