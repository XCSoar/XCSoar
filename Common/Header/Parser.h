#if !defined(AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "sizes.h"

#define NAUTICALMILESTOMETRES (double)1851.96
#define KNOTSTOMETRESSECONDS (double)0.5144

#define TOKNOTS (double)1.944
#define TOMPH   (double)2.237
#define TOKPH   (double)3.6

#define TONAUTICALMILES (double)0.00053996
#define TOMILES         (double)0.0006214
#define TOKILOMETER     (double)0.001
#define TOFEET          (double)3.281
#define TOMETER         (double)1.0

typedef struct _FLARM_TRAFFIC
{
  double Latitude;
  double Longitude;
  double TrackBearing;
  double Speed;
  double Altitude;
  double TurnRate;
  double ClimbRate;
  TCHAR ID[7];
  unsigned short IDType;
  unsigned short AlarmLevel;
  double Time_Fix;
  unsigned short Type;

} FLARM_TRAFFIC;


typedef struct _NMEA_INFO
{

  double Latitude;
  double Longitude;
  double TrackBearing;
  double Speed;
  double Altitude;
  //  TCHAR  WaypointID[WAY_POINT_ID_SIZE + 1];
  //  double WaypointBearing;
  //  double WaypointDistance;
  //  double WaypointSpeed; IGNORED NOW
  double CrossTrackError;
  double Time;
  int		Month;
  int		Day;
  int		Year;
  int   NAVWarning;
  double IndicatedAirspeed;
  double TrueAirspeed;
  double BaroAltitude;
  double MacReady;
  BOOL BaroAltitudeAvailable;
  BOOL ExternalWindAvailalbe;
  double ExternalWindSpeed;
  double ExternalWindDirection;
  BOOL VarioAvailable;
  BOOL NettoVarioAvailable;
  BOOL AirspeedAvailable;
  double Vario;
  double NettoVario;
  double IAS;
  double Ballast;
  double Bugs;
  double Gload;
  BOOL AccelerationAvailable;
  double AccelX;
  double AccelZ;
  int SatellitesUsed;
  BOOL TemperatureAvailable;
  double OutsideAirTemperature;
  BOOL HumidityAvailable;
  double RelativeHumidity;

  unsigned short FLARM_RX;
  unsigned short FLARM_TX;
  unsigned short FLARM_GPS;
  unsigned short FLARM_AlarmLevel;
  BOOL FLARM_Available;
  FLARM_TRAFFIC FLARM_Traffic[FLARM_MAX_TRAFFIC];

  double SupplyBatteryVoltage;

} NMEA_INFO;

int ParseNMEAString(TCHAR *String, NMEA_INFO *GPS_INFO);
BOOL NMEAChecksum(TCHAR *String);
void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);

extern bool EnableLogNMEA;
void LogNMEA(TCHAR* text);

#endif
