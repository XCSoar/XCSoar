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
  double RelativeNorth;
  double RelativeEast;
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


class NMEAParser {
 public:
  NMEAParser();
  static void UpdateMonitor(void);
  static BOOL ParseNMEAString(int portnum,
			      TCHAR *String, NMEA_INFO *GPS_INFO);
  static int FindVegaPort(void);
  static void Reset(void);

  BOOL ParseNMEAString_Internal(TCHAR *String, NMEA_INFO *GPS_INFO);
  bool gpsValid;
  bool hasVega;
  int nSatellites;

  bool activeGPS;

  static BOOL GpsUpdated;
  static BOOL VarioUpdated;

 public:
  static void TestRoutine(NMEA_INFO *GPS_INFO);

  // these routines can be used by other parsers.
  static BOOL NMEAChecksum(TCHAR *String);
  static void ExtractParameter(TCHAR *Source, 
			       TCHAR *Destination, 
			       int DesiredFieldNumber);

 private:

  BOOL GLL(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL GGA(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL RMC(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL RMB(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL RMA(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL RMZ(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL WP0(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL WP1(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL WP2(TCHAR *String, NMEA_INFO *GPS_INFO);
  
  // Additional sentances added by JMW
  BOOL PJV01(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PBB50(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PBJVA(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PBJVH(TCHAR *String, NMEA_INFO *GPS_INFO);
  
  BOOL PDVDS(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDVDV(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDAPL(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDAAV(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDVSC(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDSWC(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PDVVT(TCHAR *String, NMEA_INFO *GPS_INFO);
  
  // FLARM sentances
  BOOL PFLAU(TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL PFLAA(TCHAR *String, NMEA_INFO *GPS_INFO);
};

void FLARM_RefreshSlots(NMEA_INFO *GPS_INFO);

extern bool EnableLogNMEA;
void LogNMEA(TCHAR* text);

#endif
