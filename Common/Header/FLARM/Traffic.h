#ifndef XCSOAR_FLARM_TRAFFIC_H
#define XCSOAR_FLARM_TRAFFIC_H

#include <tchar.h>

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

#endif
