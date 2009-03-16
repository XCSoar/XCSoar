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
  bool   TerrainValid;
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

  // Turn rate in global coordinates
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
  double TakeOffTime;

  double AverageClimbRate[200];
  long AverageClimbRateN[200];

  double HomeDistance;

  double ZoomDistance;
  double TaskSpeedAchieved;
  double TrueAirspeedEstimated;

  double timeCruising;
  double timeCircling;

  double MinAltitude;
  double MaxHeightGain;

  // Turn rate in wind coordinates
  double GPSVario;
  double TurnRateWind;
  double BankAngle;
  double PitchAngle;
  double GPSVarioTE;
  double MacCreadyRisk;
  double TaskTimeToGoTurningNow;
  double TotalHeightClimb;
  double DistanceVario;
  double GliderSinkRate;
  double Gload;
  double Essing;
  double TerrainBase; // lowest height within glide range
  double TermikLigaPoints;
  double GRFinish;	// VENTA- Glide Ratio to final destination, 3/2/09
  double Experimental;

  // JMW note, new items should go at the bottom of this struct!
} DERIVED_INFO;


int DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
int DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


bool ClearAirspaceWarnings(const bool ack, const bool allday=false);
void RefreshTaskStatistics(void);
void  SetWindEstimate(const double speed, const double bearing, const int quality=6);

void LoadCalculationsPersist(DERIVED_INFO *Calculated);
void SaveCalculationsPersist(DERIVED_INFO *Calculated);
void DeleteCalculationsPersist(void);

void CloseCalculations(void);
void InitCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      const bool doadvance, const bool doannounce);

bool  InAATTurnSector(const double longitude, const double latitude, const int thepoint);

void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

double FAIFinishHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int wp);
int getFinalWaypoint(void);
bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin);
bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin);

double FinalGlideThroughTerrain(const double bearing, NMEA_INFO *Basic,
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon,
                                const double maxrange,
				bool *outofrange,
				double *TerrainBase = NULL);

#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

int FindFlarmSlot(const int flarmId);
int FindFlarmSlot(const TCHAR *flarmCN);
bool IsFlarmTargetCNInRange(void);

#endif
