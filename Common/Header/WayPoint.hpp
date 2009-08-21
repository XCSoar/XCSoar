#ifndef XCSOAR_WAY_POINT_HPP
#define XCSOAR_WAY_POINT_HPP

#include "Sizes.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tchar.h>

#define AIRPORT				0x01
#define TURNPOINT			0x02
#define LANDPOINT			0x04
#define HOME					0x08
#define START					0x10
#define FINISH				0x20
#define RESTRICTED		0x40
#define WAYPOINTFLAG	0x80

// VENTA3 note> probably it would be a good idea to separate static WP data to dynamic values,
// by moving things like Reachable, AltArival , etc to WPCALC
// Currently at 5.2.2 the whole structure is saved into the task file, so everytime we
// change the struct all old taks files become invalid... (there's a bug, btw, in this case)

typedef struct _WAYPOINT_INFO
{
  int Number;
  double Latitude;
  double Longitude;
  double Altitude;
  int Flags;
  TCHAR Name[NAME_SIZE + 1];
  TCHAR Comment[COMMENT_SIZE + 1];
  POINT	Screen;
  int Zoom;
  BOOL Reachable;
  double AltArivalAGL;
  BOOL Visible;
  bool InTask;
  TCHAR *Details;
  bool FarVisible;
  int FileNum; // which file it is in, or -1 to delete
} WAYPOINT;

// VENTA3
// This struct is separated from _WAYPOINT_INFO and will not be used in task files.
// It is managed by the same functions that manage WayPointList, only add variables here
// and use them like  WayPointCalc[n].Distance  for example.
typedef struct _WAYPOINT_CALCULATED
{
//  long timeslot;
  double GR;       // GR from current position
  short VGR;       // Visual GR
  double Distance; // distance from current position
  double Bearing;  // used for radial
  double AltReqd;  // comes free from CalculateWaypointArrivalAltitude
  double AltArriv; // Arrival Altitude
  bool Preferred;  // Flag to be used by Preferred quick selection WP page (todo) and
		   // by BestAlternate
} WPCALC;

extern WAYPOINT *WayPointList;
extern WPCALC   *WayPointCalc; // VENTA3 additional calculated infos on WPs
extern unsigned int NumberOfWayPoints;
extern int WaypointsOutOfRange;

#endif
