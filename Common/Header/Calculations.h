/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#if !defined(AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_CALCULATIONS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include "NMEA/Info.h"
#include <windows.h>
#include "Screen/shapelib/mapshape.h"

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
  int	 AverageLD;
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
  double NextAltitudeRequired0; // mc=0
  double NextAltitudeDifference;
  double NextAltitudeDifference0; // difference with mc=0
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
  double NextAltitudeAGL;
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
  double HomeRadial;


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
  double GRFinish;	// GRadient to final destination, 090203
			// Note: we don't need GRNext since this value is used when going to a landing
			// point, which is always a final glide.

  double Experimental;
  // JMW note, new items should go at the bottom of this struct before experimental!
} DERIVED_INFO;

extern int AutoMcMode;
extern bool EnableCalibration;
extern bool ForceFinalGlide;
extern bool AutoForceFinalGlide;

BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
BOOL DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void SearchBestAlternate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int thepoint); // VENTA3


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

void BallastDump();

#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

int FindFlarmSlot(const int flarmId);
int FindFlarmSlot(const TCHAR *flarmCN);
bool IsFlarmTargetCNInRange(void);
void AlertBestAlternate(short soundmode);

#endif
