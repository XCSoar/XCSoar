#if !defined(AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "parser.h"
#include <windows.h>

#define NUMTHERMALBUCKETS 10

typedef struct _DERIVED_INFO
{
  double Vario;
  double LD;
  double CruiseLD;
  double VMcReady;
  double Average30s;
  double BestCruiseTrack;
  double AverageThermal;
  double ThermalGain;
  double LastThermalAverage;
  double LastThermalGain;
  double LastThermalTime;
  double ClimbStartLat;
  double ClimbStartLong;
  double ClimbStartAlt;
  double ClimbStartTime;
  double CruiseStartLat;
  double CruiseStartLong;
  double CruiseStartAlt;
  double CruiseStartTime;
  double WindSpeed;
  double WindBearing;
  double Bearing;
  double TerrainAlt;
  double AltitudeAGL;
  int    Circling;
  int    FinalGlide;
  int    AutoMcReady;
  double NextAltitudeRequired;
  double NextAltitudeDifference;
  double FinalAltitudeRequired;
  double FinalAltitudeDifference;
  double TaskDistanceToGo;
  double TaskDistanceCovered;
  double TaskStartTime;
  double TaskSpeed;
  double TaskAltitudeRequired;
  double TaskAltitudeDifference;
  double LDFinish;
  double LegDistanceToGo;
  double LegDistanceCovered;
  double LegStartTime;
  double LegSpeed;
  double NextLattitude;
  double NextLongditde;
  double NextAltitude;
  double AATMaxDistance;
  double AATMinDistance;
  double AATTimeToGo;
  double AATMaxSpeed;
  double AATMinSpeed;
  double PercentCircling;

  double TerrainWarningLongditude;
  double TerrainWarningLattitude;

  // JMW thermal band data
  double MaxThermalHeight;
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  double ThermalProfileW[NUMTHERMALBUCKETS];

} DERIVED_INFO;

#include "RasterTerrain.h"


int DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
int DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void OpenTerrain(void);
void CloseTerrain(void);
void AddSnailPoint(void);
double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic,
				DERIVED_INFO *Calculated, double *retlat,
				double *retlon);

bool ClearAirspaceWarnings();

#endif
