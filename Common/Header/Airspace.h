/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#if !defined(XCSOAR_AIRSPACE_H)
#define XCSOAR_AIRSPACE_H

#include "Sizes.h"
#include "Screen/shapelib/mapshape.h"
#include "GeoPoint.hpp"

#include <windef.h>
#include <tchar.h>

struct SETTINGS_COMPUTER;
class AirspaceDatabase;

struct AIRSPACE_ACK
{
  bool AcknowledgedToday;
  double AcknowledgementTime;
};

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

struct AIRSPACE_ALT
{
  double Altitude;
  double FL;
  double AGL;
  AirspaceAltBase_t Base;
};

struct AIRSPACE_AREA
{
  TCHAR Name[NAME_SIZE + 1];
  int Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;
  unsigned FirstPoint;
  unsigned NumPoints;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
  GEOPOINT minBound;
  GEOPOINT maxBound;
  rectObj bounds;
  AIRSPACE_ACK Ack;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
  bool FarVisible;
};

#define AIRSPACE_POINT GEOPOINT
// quick hack...

struct AIRSPACE_CIRCLE
{
  TCHAR Name[NAME_SIZE + 1];
  int Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;
  GEOPOINT Location;
  double Radius;
  POINT Screen;
  int ScreenR;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
  AIRSPACE_ACK Ack;
  rectObj bounds;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
  bool FarVisible;
};

// Airspace Database
extern AirspaceDatabase airspace_database;

extern POINT *AirspaceScreenPoint;

void DeleteAirspace();

void ReadAirspace(void);
int FindAirspaceCircle(const GEOPOINT &location,
		       bool visibleonly=true);
int FindAirspaceArea(const GEOPOINT &location,
		     bool visibleonly=true);

bool
CheckAirspaceAltitude(double Base, double Top,
                      const SETTINGS_COMPUTER &settings);

void CloseAirspace(void);

void SortAirspace(void);

bool InsideAirspaceCircle(const GEOPOINT &location,
			  const int i);

bool InsideAirspaceArea(const GEOPOINT &location,
			const int i);

void ScanAirspaceLine(const GEOPOINT *locs,
                      const double *heights,
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X]);


void AirspaceQnhChangeNotify(double newQNH);

//*******************************************************************************
// experimental: new dialog based warning system

class AirspaceInfo_c{

public:

  int    TimeOut;             // in systicks
  int    InsideAckTimeOut;    // downgrade auto ACK timer
  int    Sequence;            // Sequence nummer is equal for real and predicted calculation
  int    hDistance;           // horizontal distance in m
  int    vDistance;           // vertical distance in m
  int    Bearing;             // in deg
  DWORD  PredictedEntryTime;  // in ms
  int    Acknowledge;         // 0=not Acked, 1=Acked til closer, 2=Acked til leave, 3= Acked whole day
  bool   Inside;              // true if inside
  bool   Predicted;           // true if predicted inside, menas close and entry expected
  bool   IsCircle;            // true if Airspace is a circle
  int    AirspaceIndex;       // index of airspace
  int    SortKey;             // SortKey
  int    LastListIndex;       // Last index in List, used to sort items with same sort criteria
  int    ID;                  // Unique ID
  int    WarnLevel;           // WarnLevel 0 far away, 1 prdicted entry, 2 predicted entry and close, 3 inside

};

typedef enum {asaNull,
              asaItemAdded,
              asaItemChanged,
              asaClearAll,
              asaItemRemoved,
              asaWarnLevelIncreased,
              asaProcessEnd,
              asaProcessBegin} AirspaceWarningNotifyAction_t;

typedef void (*AirspaceWarningNotifier_t)(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace) ;

void AirspaceWarnListAddNotifier(AirspaceWarningNotifier_t Notifier);
void AirspaceWarnListRemoveNotifier(AirspaceWarningNotifier_t Notifier);
bool AirspaceWarnGetItem(int Index, AirspaceInfo_c &Item);
int AirspaceWarnGetItemCount(void);
int dlgAirspaceWarningInit(void);
int dlgAirspaceWarningDeInit(void);
void AirspaceWarnListClear(void);
void AirspaceWarnDoAck(int ID, int Ack);
int AirspaceWarnFindIndexByID(int ID);

bool ValidAirspace(void);

class MapWindowProjection;

double RangeAirspaceCircle(const GEOPOINT &location,
			   const int i);

double RangeAirspaceArea(const GEOPOINT &location,
			 const int i, double *bearing,
			 const MapWindowProjection &map_projection);

void FindNearestAirspace(const GEOPOINT &location,
                         const SETTINGS_COMPUTER &settings,
                         const MapWindowProjection &map_projection,
                         double *nearestdistance,
			 double *nearestbearing,
			 int *foundcircle,
			 int *foundarea,
			 double *height=NULL);


#endif
