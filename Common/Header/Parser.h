#if !defined(AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Sizes.h"

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
  double RelativeNorth;
  double RelativeEast;
  double RelativeAltitude;
  long ID;
  TCHAR Name[10];
  unsigned short IDType;
  unsigned short AlarmLevel;
  double Time_Fix;
  unsigned short Type;
#ifdef FLARM_AVERAGE
  double Average30s;
#endif
} FLARM_TRAFFIC;


typedef struct _SWITCH_INFO
{
  bool AirbrakeLocked;
  bool FlapPositive;
  bool FlapNeutral;
  bool FlapNegative;
  bool GearExtended;
  bool Acknowledge;
  bool Repeat;
  bool SpeedCommand;
  bool UserSwitchUp;
  bool UserSwitchMiddle;
  bool UserSwitchDown;
  bool VarioCircling;
  bool FlapLanding;
  // bool Stall;
} SWITCH_INFO;


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
  int Hour;
  int Minute;
  int Second;
  int Month;
  int Day;
  int Year;
  int NAVWarning;
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
  int SatelliteIDs[MAXSATELLITES];

  double SupplyBatteryVoltage;

  SWITCH_INFO SwitchState;

  BOOL MovementDetected;

  double StallRatio;

} NMEA_INFO;


class NMEAParser {
 public:
  NMEAParser();
  static void UpdateMonitor(void);
  static BOOL ParseNMEAString(int portnum,
			      TCHAR *String, NMEA_INFO *GPS_INFO);
  static void Reset(void);
  static bool PortIsFlarm(int portnum);
  void _Reset(void);

  BOOL ParseNMEAString_Internal(TCHAR *String, NMEA_INFO *GPS_INFO);
  bool gpsValid;
  int nSatellites;

  bool activeGPS;
  bool isFlarm;

  static int StartDay;

 public:
  static void TestRoutine(NMEA_INFO *GPS_INFO);

  // these routines can be used by other parsers.
  static double ParseAltitude(TCHAR *, const TCHAR *);
  static size_t ValidateAndExtract(const TCHAR *src, TCHAR *dst, size_t dstsz, TCHAR **arr, size_t arrsz);
  static size_t ExtractParameters(const TCHAR *src, TCHAR *dst, TCHAR **arr, size_t sz);
  static BOOL NMEAChecksum(const TCHAR *String);

  static void ExtractParameter(const TCHAR *Source,
			       TCHAR *Destination,
			       int DesiredFieldNumber);

 private:
  BOOL GSAAvailable;
  BOOL GGAAvailable;
  BOOL RMZAvailable;
  BOOL RMAAvailable;
  double RMZAltitude;
  double RMAAltitude;
  double LastTime;

  bool TimeHasAdvanced(double ThisTime, NMEA_INFO *GPS_INFO);
  static double TimeModify(double FixTime, NMEA_INFO* info);

  BOOL GLL(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL GGA(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL GSA(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL RMC(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL RMB(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL RMA(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL RMZ(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  BOOL WP0(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL WP1(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL WP2(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);

  // Additional sentances
  BOOL PTAS1(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);  // RMN: Tasman instruments.  TAS, Vario, QNE-altitude

  // FLARM sentances
  BOOL PFLAU(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
  BOOL PFLAA(TCHAR *String, TCHAR **, size_t, NMEA_INFO *GPS_INFO);
};

void FLARM_RefreshSlots(NMEA_INFO *GPS_INFO);

extern bool EnableLogNMEA;
void LogNMEA(TCHAR* text);

#endif
