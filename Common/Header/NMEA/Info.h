#ifndef XCSOAR_NMEA_INFO_H
#define XCSOAR_NMEA_INFO_H

#include "FLARM/Traffic.h"
#include "Sizes.h"

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
  bool BaroAltitudeAvailable;
  bool ExternalWindAvailalbe;
  double ExternalWindSpeed;
  double ExternalWindDirection;
  bool VarioAvailable;
  bool NettoVarioAvailable;
  bool AirspeedAvailable;
  double Vario;
  double NettoVario;
  double Ballast;
  double Bugs;
  double Gload;
  bool AccelerationAvailable;
  double AccelX;
  double AccelZ;
  int SatellitesUsed;
  bool TemperatureAvailable;
  double OutsideAirTemperature;
  bool HumidityAvailable;
  double RelativeHumidity;

  unsigned short FLARM_RX;
  unsigned short FLARM_TX;
  unsigned short FLARM_GPS;
  unsigned short FLARM_AlarmLevel;
  bool FLARM_Available;
  FLARM_TRAFFIC FLARM_Traffic[FLARM_MAX_TRAFFIC];
  int SatelliteIDs[MAXSATELLITES];

  double SupplyBatteryVoltage;

  SWITCH_INFO SwitchState;

  bool MovementDetected;

  double StallRatio;

} NMEA_INFO;

#endif
