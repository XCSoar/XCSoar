#if !defined(AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Parser.h"
#include <windows.h>
#include "mapshape.h"

#define NUMTHERMALBUCKETS 10
#define MAX_THERMAL_SOURCES 20


typedef struct _THERMAL_SOURCE_INFO
{
  POINT Screen;
  double Latitude;
  double Longitude;
  double GroundHeight;
  double LiftRate;
  bool Visible;
  double Time;
} THERMAL_SOURCE_INFO;


typedef struct _DERIVED_INFO
{
  double Vario;
  double LD;
  double CruiseLD;
  double VMacCready;
  double Average30s;
  double NettoAverage30s;
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
  bool TerrainValid;
  double Heading;
  double AltitudeAGL;
  int    Circling;
  int    FinalGlide;
  int    AutoMacCready;
  int    Flying;
  double NextAltitudeRequired;
  double NextAltitudeDifference;
  double FinalAltitudeRequired;
  double FinalAltitudeDifference;
  double TaskDistanceToGo;
  double TaskDistanceCovered;
  double TaskTimeToGo;
  double TaskStartTime;
  double TaskSpeed;
  double TaskSpeedInstantaneous;
  double TaskAltitudeRequired;
  double TaskAltitudeDifference;
  double TaskAltitudeDifference0; // difference with mc=0
  double TaskAltitudeRequiredFromStart;
  double LDFinish;
  double LDNext;
  double LegDistanceToGo;
  double LegDistanceCovered;
  double LegTimeToGo;
  double LegStartTime;
  double LegSpeed;
  double NextLatitude;
  double NextLongitude;
  double NextAltitude;
  double AATMaxDistance;
  double AATMinDistance;
  double AATTargetDistance;
  double AATTimeToGo;
  double AATMaxSpeed;
  double AATTargetSpeed;
  double AATMinSpeed;
  double PercentCircling;

  double TerrainWarningLongitude;
  double TerrainWarningLatitude;

  // JMW moved calculated waypoint info here

  double WaypointBearing;
  double WaypointDistance;
  double WaypointSpeed;

  // JMW thermal band data
  double MaxThermalHeight;
  int    ThermalProfileN[NUMTHERMALBUCKETS];
  double ThermalProfileW[NUMTHERMALBUCKETS];

  double NettoVario;

  // optimum speed to fly instantaneously
  double VOpt;

  // JMW estimated track bearing at next time step
  double NextTrackBearing;

  // whether Speed-To-Fly audio are valid or not
  bool STFMode;

  // JMW energy height excess to slow to best glide speed
  double EnergyHeight;

  double TurnRate;

  // reflects whether aircraft is in a start/finish/aat/turn sector
  bool IsInSector;
  bool IsInAirspace;

  // detects when glider is on ground for several seconds
  bool OnGround;

  double NavAltitude;
  bool ValidStart;
  double TaskStartSpeed;
  double TaskStartAltitude;
  bool ValidFinish;

  double LDvario;

  double ThermalEstimate_Longitude;
  double ThermalEstimate_Latitude;
  double ThermalEstimate_W;
  double ThermalEstimate_R;

  THERMAL_SOURCE_INFO ThermalSources[MAX_THERMAL_SOURCES];

  pointObj GlideFootPrint[NUMTERRAINSWEEPS+1];

  TCHAR OwnTeamCode[10];
  double TeammateBearing;
  double TeammateRange;
  double TeammateLatetude;
  double TeammateLongitude;
  double FlightTime;
} DERIVED_INFO;


int DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
int DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void OpenTerrain(void);
void CloseTerrain(void);
void AddSnailPoint(void);
double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic,
				DERIVED_INFO *Calculated, double *retlat,
				double *retlon,
				double maxsearchrange,
				bool *outofrange);

bool ClearAirspaceWarnings(bool ack, bool allday=false);
void ResumeAbortTask(int set = 0);
void RefreshTaskStatistics(void);
void  SetWindEstimate(double speed, double bearing, int quality=6);

double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
		      double bearing,
		      double GlideSlope);

void CloseCalculations(void);

double MacCreadyTimeLimit(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			  double bearing,
			  double timeremaining,
			  double hfinal);

void SaveCalculationsPersist(DERIVED_INFO *Calculated);
void InitCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

#endif
