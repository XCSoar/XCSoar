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
#ifndef AIRSPACE_WARNING_H
#define AIRSPACE_WARNING_H

#include <windef.h>

struct NMEA_INFO;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;

class AirspaceDatabase;
class MapWindowProjection;

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

extern void
AirspaceWarnListAdd(AirspaceDatabase &airspace_database,
                    const NMEA_INFO &casic, const DERIVED_INFO &calculated,
                    const SETTINGS_COMPUTER &settings,
                    const MapWindowProjection &map_projection,
                    bool Predicted, bool IsCircle, int AsIdx,
                    bool ackDay=false);

extern void
AirspaceWarnListProcess(AirspaceDatabase &airspace_database,
                        const NMEA_INFO &casic, const DERIVED_INFO &calculated,
                        const SETTINGS_COMPUTER &settings,
                        const MapWindowProjection &map_projection);

void
AirspaceWarnDoAck(AirspaceDatabase &airspace_database, int ID, int Ack);

void
AirspaceWarnListClear(AirspaceDatabase &airspace_database);

int AirspaceWarnFindIndexByID(int ID);

bool
ClearAirspaceWarnings(AirspaceDatabase &airspace_database,
                      bool ack, bool allday=false);

int dlgAirspaceWarningInit(void);
int dlgAirspaceWarningDeInit(void);

#endif
