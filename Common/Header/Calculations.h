#if !defined(AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "parser.h"
#include <windows.h>

typedef struct _DERIVED_INFO
{
  double Vario;
  double LD;
	double CruiseLD;
  double Average30s;
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
} DERIVED_INFO;

typedef struct _TERRAIN_INFO
{
  double Left;
	double Right;
	double Top;
	double Bottom;
	double StepSize;
	long Rows;
	long Columns;
} TERRAIN_INFO;

int DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void OpenTerrain(void);
void CloseTerrain(void);
double GetTerrainHeight(double Lattitude, double Longditude);


#endif