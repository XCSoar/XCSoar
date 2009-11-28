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

#include "AirspaceWarning.h"
#include "Airspace.h"
#include "AirspaceDatabase.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"
#include "simpleList.h"
#include "Thread/Mutex.hpp"

#include <stdlib.h>

static bool NewAirspaceWarnings = false;
static Mutex mutexAirspaceWarnings;

#define OUTSIDE_CHECK_INTERVAL 4

List<AirspaceWarningNotifier_t> AirspaceWarningNotifierList;

List<AirspaceInfo_c> AirspaceWarnings;

static bool UpdateAirspaceAckBrush(AirspaceInfo_c *Item, int Force){
  bool res=false;

  if (Force == 0){
    if (Item->IsCircle){
      if (airspace_database.AirspaceCircle != NULL) {
        AIRSPACE_CIRCLE &circle =
          airspace_database.AirspaceCircle[Item->AirspaceIndex];
        res = circle._NewWarnAckNoBrush;
        circle._NewWarnAckNoBrush =
          ((Item->WarnLevel > 0) && (Item->WarnLevel <= Item->Acknowledge))
          || (Item->Acknowledge==4);
      } else {
        res = false;
      }
    } else {
      if (airspace_database.AirspaceArea != NULL) {
        AIRSPACE_AREA &area =
          airspace_database.AirspaceArea[Item->AirspaceIndex];
        res = area._NewWarnAckNoBrush;
        area._NewWarnAckNoBrush =
          ((Item->WarnLevel > 0) && (Item->WarnLevel <= Item->Acknowledge))
          || (Item->Acknowledge==4);
      } else {
        res = false;
      }
    }
  } else {
    if (Item->IsCircle){
      if (airspace_database.AirspaceCircle != NULL) {
        AIRSPACE_CIRCLE &circle =
          airspace_database.AirspaceCircle[Item->AirspaceIndex];
        res = circle._NewWarnAckNoBrush;
        circle._NewWarnAckNoBrush = (Force == 1);
      } else {
        res = false;
      }
    } else {
      if (airspace_database.AirspaceArea != NULL) {
        AIRSPACE_AREA &area =
          airspace_database.AirspaceArea[Item->AirspaceIndex];
        res = area._NewWarnAckNoBrush;
        area._NewWarnAckNoBrush = (Force == 1);
      } else {
        res = false;
      }
    }
  }
  return(res);
}

void AirspaceWarnListAddNotifier(AirspaceWarningNotifier_t Notifier){
  AirspaceWarningNotifierList.push_front(Notifier);
}


void AirspaceWarnListRemoveNotifier(AirspaceWarningNotifier_t Notifier){
  List<AirspaceWarningNotifier_t>::Node* it = AirspaceWarningNotifierList.begin();
  while(it){
    if (it->data == Notifier){
      it = AirspaceWarningNotifierList.erase(it);
      continue;
    }
    it = it->next;
  }
}

bool AirspaceWarnGetItem(int Index, AirspaceInfo_c &Item){
  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next, Index-- ){
    if (Index == 0){
      Item = it->data;  // make a copy!
      return true;
    }
  }
  return(false);
}

int AirspaceWarnGetItemCount(void){
  int res=0;
  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
    res++;
  }
  return(res);
}


static void AirspaceWarnListDoNotify(AirspaceWarningNotifyAction_t Action,
                                     AirspaceInfo_c *AirSpace)
{
  for (List<AirspaceWarningNotifier_t>::Node*
         it = AirspaceWarningNotifierList.begin(); it; it = it->next ){
    (it->data)(Action, AirSpace);
  }
}


static void
AirspaceWarnListCalcDistance(const NMEA_INFO *Basic,
                             const DERIVED_INFO *Calculated,
                             bool IsCircle, int AsIdx,
                             int *hDistance, int *Bearing, int *vDistance,
                             const MapWindowProjection &map_projection)
{
  int vDistanceBase;
  int vDistanceTop;
  int alt;
  int agl;

  if (Basic->BaroAltitudeAvailable) {
    alt = (int)Basic->BaroAltitude;
  } else {
    alt = (int)Basic->Altitude;
  }
  agl = (int)Calculated->AltitudeAGL;

  if (IsCircle){
    const AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[AsIdx];

    *hDistance = (int)RangeAirspaceCircle(Basic->Location,
                                          AsIdx);
    if (*hDistance < 0)
      *hDistance = 0;
    if (circle.Base.Base != abAGL) {
      vDistanceBase = alt - (int)circle.Base.Altitude;
    } else {
      vDistanceBase = agl - (int)circle.Base.AGL;
    }
    if (circle.Top.Base != abAGL) {
      vDistanceTop  = alt - (int)circle.Top.Altitude;
    } else {
      vDistanceTop  = agl - (int)circle.Top.AGL;
    }
    // EntryTime = ToDo
  } else {
    const AIRSPACE_AREA &area = airspace_database.AirspaceArea[AsIdx];

    if (!InsideAirspaceArea(Basic->Location, AsIdx)){
      // WARNING: RangeAirspaceArea dont return negative values if
      // inside aera -> but RangeAirspaceCircle does!
      double fBearing;
      *hDistance = (int)RangeAirspaceArea(Basic->Location,
                                          AsIdx, &fBearing,
					  map_projection);
      *Bearing = (int)fBearing;
    } else {
      *hDistance = 0;
    }
    if (area.Base.Base != abAGL) {
      vDistanceBase = alt - (int)area.Base.Altitude;
    } else {
      vDistanceBase = agl - (int)area.Base.AGL;
    }
    if (area.Top.Base != abAGL) {
      vDistanceTop  = alt - (int)area.Top.Altitude;
    } else {
      vDistanceTop  = agl - (int)area.Top.AGL;
    }
    // EntryTime = ToDo
  }

  if (vDistanceBase > 0 && vDistanceTop < 0) {
    *vDistance  = 0;
  }
  else if (-vDistanceBase > vDistanceTop)
    *vDistance = vDistanceBase;
  else
    *vDistance = vDistanceTop;
}

static bool calcWarnLevel(AirspaceInfo_c *asi){

  int LastWarnLevel;

  if (asi == NULL)
    return(false);

  int dh = asi->hDistance;
  int dv = abs(asi->vDistance);

  LastWarnLevel = asi->WarnLevel;

  if (!asi->Inside && asi->Predicted){
    if ((dh < 500) && (dv < 100)) { // JMW hardwired!
      asi->WarnLevel = 2;
    } else
      asi->WarnLevel = 1;
  } else
  if (asi->Inside){
    asi->WarnLevel = 3;
  }

  asi->SortKey = dh + dv*20;

  UpdateAirspaceAckBrush(asi, 0);

  return((asi->WarnLevel > asi->Acknowledge)
	 && (asi->WarnLevel > LastWarnLevel));

}

void
AirspaceWarnListAdd(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated,
                    const SETTINGS_COMPUTER *settings,
                    const MapWindowProjection &map_projection,
                    bool Predicted, bool IsCircle, int AsIdx,
                    bool ackDay)
{
  static int  Sequence = 0;

  if (!Predicted){
    if (++Sequence > 1000)
      Sequence = 1;
  }

  int   hDistance = 0;
  int   vDistance = 0;
  int   Bearing = 0;
  DWORD EntryTime = 0;
  bool  FoundInList = false;

  if (Predicted){  // ToDo calculate predicted data
    AirspaceWarnListCalcDistance(Basic, Calculated, IsCircle, AsIdx, &hDistance,
				 &Bearing, &vDistance,
                                 map_projection);
  }
  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin();
       it; it = it->next ){
    if ((it->data.IsCircle == IsCircle)
        && (it->data.AirspaceIndex == AsIdx)){ // check if already in list

      if ((it->data.Sequence == Sequence) && (it->data.Sequence>=0)
          && !ackDay){
        // still updated in real pos calculation
        // JMW: need to be able to override if ack day
      } else {

        it->data.Sequence = Sequence;
        it->data.TimeOut = 3;
        it->data.hDistance = hDistance;
        it->data.vDistance = vDistance;
        it->data.Bearing = Bearing;
        it->data.PredictedEntryTime = EntryTime;

        if (ackDay) {
          if (!Predicted) {
            it->data.Acknowledge = 4;
          } else {
            // code for cancel daily ack
            if (it->data.Acknowledge == 4) {
              it->data.Acknowledge = 0;
            }
          }
          it->data.Inside = 0;
          it->data.Predicted = 0;
        } else {
          if (!Predicted) {
            it->data.Inside = true;
          }
          it->data.Predicted = Predicted;
        }

        if (calcWarnLevel(&it->data))
          AirspaceWarnListDoNotify(asaWarnLevelIncreased, &it->data);
        else
          AirspaceWarnListDoNotify(asaItemChanged, &it->data);

      }

      it->data.InsideAckTimeOut = settings->AcknowledgementTime / OUTSIDE_CHECK_INTERVAL;
      it->data.TimeOut = OUTSIDE_CHECK_INTERVAL;

      FoundInList = true;
      break;

    }
  }

  if (!FoundInList && !(Predicted && ackDay)) {
    // JMW Predicted & ackDay means cancel a daily ack

    AirspaceInfo_c asi; // not in list, add new

    asi.TimeOut = OUTSIDE_CHECK_INTERVAL;

    asi.InsideAckTimeOut = settings->AcknowledgementTime / OUTSIDE_CHECK_INTERVAL;

    asi.Sequence = Sequence;

    asi.hDistance = hDistance;
    asi.vDistance = vDistance;
    asi.Bearing = Bearing;
    asi.PredictedEntryTime = EntryTime;  // ETE, ToDo

    if (ackDay) {
      asi.Acknowledge = 4;
      asi.Inside = 0;
      asi.Predicted = 0;
    } else {
      asi.Acknowledge = 0;
      asi.Inside = !Predicted;
      asi.Predicted = Predicted;
    }
    asi.IsCircle = IsCircle;
    asi.AirspaceIndex = AsIdx;
    asi.SortKey = 0;
    asi.LastListIndex = 0; // JMW initialise
    asi.WarnLevel = 0; // JMW initialise

    calcWarnLevel(&asi);

    asi.ID = AsIdx + (IsCircle ? 10000 : 0);

    AirspaceWarnings.push_front(asi);
    UpdateAirspaceAckBrush(&asi, 0);

    NewAirspaceWarnings = true;

    AirspaceWarnListDoNotify(asaItemAdded, &AirspaceWarnings.begin()->data);
  }
}

static int _cdecl cmp(const void *a, const void *b){

  int adAL, bdAL;

  adAL = ((AirspaceInfo_c *)a)->WarnLevel - ((AirspaceInfo_c *)a)->Acknowledge;
  bdAL = ((AirspaceInfo_c *)b)->WarnLevel - ((AirspaceInfo_c *)b)->Acknowledge;

  if (adAL < bdAL)
    return(1);
  if (adAL > bdAL)
    return(-1);
  if (((AirspaceInfo_c *)a)->LastListIndex > ((AirspaceInfo_c *)b)->LastListIndex)
    return(1);
  return(-1);

}

void AirspaceWarnListSort(void){

  unsigned i, idx=0;
  AirspaceInfo_c l[20];
  List<AirspaceInfo_c>::Node* it;

  for (it = AirspaceWarnings.begin(); it; it = it->next){
    it->data.LastListIndex = idx;
    l[idx] = it->data;
    idx++;
    if (idx >= (sizeof(l)/(sizeof(l[0]))))
      return;
  }

  if (idx < 1)
    return;

  qsort(&l[0], idx, sizeof(l[0]), cmp);

  it = AirspaceWarnings.begin();

  for (i=0; i<idx; i++){
    it->data = l[i];
    it = it->next;
  }

}


void
AirspaceWarnListProcess(const NMEA_INFO *Basic,
                        const DERIVED_INFO *Calculated,
                        const SETTINGS_COMPUTER *settings,
                        const MapWindowProjection &map_projection)
{

  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it;){

    it->data.TimeOut--;                // age the entry

    if (it->data.TimeOut < 0){

      it->data.Predicted = false;
      it->data.Inside = false;
      it->data.WarnLevel = 0;

      int hDistance = 0;
      int vDistance = 0;
      int Bearing = 0;

      AirspaceWarnListCalcDistance(Basic, Calculated,
                                   it->data.IsCircle,
                                   it->data.AirspaceIndex,
                                   &hDistance, &Bearing, &vDistance,
                                   map_projection);

      it->data.hDistance = hDistance; // for all: update data
      it->data.vDistance = vDistance;
      it->data.Bearing = Bearing;
      it->data.TimeOut = OUTSIDE_CHECK_INTERVAL; // retrigger checktimeout

      if (calcWarnLevel(&it->data))
        AirspaceWarnListDoNotify(asaWarnLevelIncreased, &it->data);

      UpdateAirspaceAckBrush(&it->data, 0);

      if (it->data.Acknowledge == 4){ // whole day achnowledged
        if (it->data.SortKey > 25000) {
          it->data.TimeOut = 60; // JMW hardwired why?
        }
        continue;
      }

      if ((hDistance > 2500) || (abs(vDistance) > 250)){
        // far away remove from warning list
        AirspaceInfo_c asi = it->data;

        it = AirspaceWarnings.erase(it);
        UpdateAirspaceAckBrush(&asi, -1);
        AirspaceWarnListDoNotify(asaItemRemoved, &asi);

        continue;

      } else {
        if ((hDistance > 500) || (abs(vDistance) > 100)){
          // close clear inside ack and auto ACK til closer
          if (it->data.Acknowledge > 2)
            if (--(it->data.InsideAckTimeOut) < 0){      // timeout the
              it->data.Acknowledge = 1;
            }

        } else { // very close, just update ack timer
          it->data.InsideAckTimeOut = settings->AcknowledgementTime
            / OUTSIDE_CHECK_INTERVAL;
          // 20sec outside check interval prevent down ACK on circling
        }
      }

      AirspaceWarnListDoNotify(asaItemChanged, &it->data);

    }


    // SortTheList();


    it = it->next;

  }

  AirspaceWarnListSort();

#ifdef DEBUG_AIRSPACE
  DebugStore("%d # airspace\n", AirspaceWarnGetItemCount());
#endif

  AirspaceWarnListDoNotify(asaProcessEnd, NULL);
}


void AirspaceWarnDoAck(int ID, int Ack){
  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
    if (it->data.ID == ID){

      if (Ack < it->data.Acknowledge)   // force data refresh on down ack
        it->data.TimeOut=0;

      if (Ack==-1)                      // ack current warnlevel
        it->data.Acknowledge = it->data.WarnLevel;
      else                              // ack defined warnlevel
        it->data.Acknowledge = Ack;

      UpdateAirspaceAckBrush(&it->data, 0);

      AirspaceWarnListDoNotify(asaItemChanged, &it->data);

    }
  }
}


void AirspaceWarnListClear(void){
  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
    UpdateAirspaceAckBrush(&it->data, -1);
  }
  AirspaceWarnings.clear();
  AirspaceWarnListDoNotify(asaClearAll ,NULL);
}


int AirspaceWarnFindIndexByID(int ID){
  int idx=0;
  if (ID<0) {
    return -1;
  }

  ScopeLock protect(mutexAirspaceWarnings);
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
    if (it->data.ID == ID){
      return idx;
    }
    idx++;
  }
  return(-1);
}

#include "Message.h"
#include "Interface.hpp"
extern bool GlobalClearAirspaceWarnings;
// JMW this code needs a better home

bool ClearAirspaceWarnings(const bool acknowledge, const bool ack_all_day) {
  if (!acknowledge)
    return false;

  GlobalClearAirspaceWarnings = true;
  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceCircles; i++) {
    AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[i];

    if (circle.WarningLevel > 0) {
      circle.Ack.AcknowledgementTime = XCSoarInterface::Basic().Time;
      if (ack_all_day)
        circle.Ack.AcknowledgedToday = true;
      circle.WarningLevel = 0;
    }
  }

  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceAreas; i++) {
    AIRSPACE_AREA &area = airspace_database.AirspaceArea[i];

    if (area.WarningLevel > 0) {
      area.Ack.AcknowledgementTime = XCSoarInterface::Basic().Time;
      if (ack_all_day)
        area.Ack.AcknowledgedToday = true;

      area.WarningLevel = 0;
    }
  }

  return Message::Acknowledge(Message::MSG_AIRSPACE);
}
