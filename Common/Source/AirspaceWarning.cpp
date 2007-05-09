/*
Copyright_License {
  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005
  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>
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
#include "stdafx.h"
#include "Calculations.h"
#include "Externs.h"
#include "Dialogs.h"
#include "Utils.h"
#include "Airspace.h"
#include "device.h"


#include "simpleList.h"

static bool NewAirspaceWarnings = false;
static CRITICAL_SECTION  csAirspaceWarnings;

#define OUTSIDE_CHECK_INTERVAL 4


List<AirspaceWarningNotifier_t> AirspaceWarningNotifierList;

List<AirspaceInfo_c> AirspaceWarnings;

static void LockList(void){
  EnterCriticalSection(&csAirspaceWarnings);
}

static void UnLockList(void){
  LeaveCriticalSection(&csAirspaceWarnings);
}

static bool UpdateAirspaceAckBrush(AirspaceInfo_c *Item, int Force){
  bool res=false;

  if (Force == 0){
    if (Item->IsCircle){
      if (AirspaceCircle) {
        res = AirspaceCircle[Item->AirspaceIndex]._NewWarnAckNoBrush;
        AirspaceCircle[Item->AirspaceIndex]._NewWarnAckNoBrush =
          (Item->WarnLevel > 0) && (Item->WarnLevel <= Item->Acknowledge);
      } else {
        res = false;
      }
    } else {
      if (AirspaceArea) {
        res = AirspaceArea[Item->AirspaceIndex]._NewWarnAckNoBrush;
        AirspaceArea[Item->AirspaceIndex]._NewWarnAckNoBrush =
          (Item->WarnLevel > 0) && (Item->WarnLevel <= Item->Acknowledge);
      } else {
        res = false;
      }
    }
  } else {
    if (Item->IsCircle){
      if (AirspaceCircle) {
        res = AirspaceCircle[Item->AirspaceIndex]._NewWarnAckNoBrush;
        AirspaceCircle[Item->AirspaceIndex]._NewWarnAckNoBrush = (Force == 1);
      } else {
        res = false;
      }
    } else {
      if (AirspaceArea) {
        res = AirspaceArea[Item->AirspaceIndex]._NewWarnAckNoBrush;
        AirspaceArea[Item->AirspaceIndex]._NewWarnAckNoBrush = (Force == 1);
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
  bool res = false;

  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next, Index-- ){
      if (Index == 0){
	Item = it->data;  // make a copy!
	res = true;
	break;
      }
    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }
  return(res);
}

int AirspaceWarnGetItemCount(void){
  int res=0;
  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
      res++;
    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }
  return(res);
}



double RangeAirspaceCircle(const double &longitude, const double &latitude, int i);
double RangeAirspaceArea(const double &longitude, const double &latitude, int i, double *bearing);


static void AirspaceWarnListDoNotify(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace){
  for (List<AirspaceWarningNotifier_t>::Node* it = AirspaceWarningNotifierList.begin(); it; it = it->next ){
    (it->data)(Action, AirSpace);
  }
}


static void AirspaceWarnListCalcDistance(NMEA_INFO *Basic, bool IsCircle, int AsIdx, int *hDistance, int *Bearing, int *vDistance){

  int vDistanceBase;
  int vDistanceTop;
  int alt;

  if (Basic->BaroAltitudeAvailable) {
    alt = (int)Basic->BaroAltitude;
  } else {
    alt = (int)Basic->Altitude;
  }

  if (IsCircle){
    *hDistance = (int)RangeAirspaceCircle(Basic->Longitude, 
                                          Basic->Latitude, 
                                          AsIdx);
    if (*hDistance < 0)
      *hDistance = 0;
    vDistanceBase = (int)AirspaceCircle[AsIdx].Base.Altitude - alt;
    vDistanceTop  = (int)AirspaceCircle[AsIdx].Top.Altitude - alt;
    // EntryTime = ToDo
  } else {
    if (!InsideAirspaceArea(Basic->Longitude, Basic->Latitude, AsIdx)){
      // WARNING: RangeAirspaceArea dont return negative values if insige aera -> but RangeAirspaceCircle does!
      double fBearing;
      *hDistance = (int)RangeAirspaceArea(Basic->Longitude, Basic->Latitude, AsIdx, &fBearing);
      *Bearing = (int)fBearing;
    } else {
      *hDistance = 0;
    }
    vDistanceBase = alt - (int)AirspaceArea[AsIdx].Base.Altitude;
    vDistanceTop  = alt - (int)AirspaceArea[AsIdx].Top.Altitude;
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
    if ((dh < 500) && (dv < 100)) {
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

void AirspaceWarnListAdd(NMEA_INFO *Basic, int Sequence, bool Predicted, 
			 bool IsCircle, int AsIdx){

  int   hDistance = 0;
  int   vDistance = 0;
  int   Bearing = 0;
  DWORD EntryTime = 0;
  bool  FoundInList = false;

  if (Predicted){  // ToDo calculate predicted data
    AirspaceWarnListCalcDistance(Basic, IsCircle, AsIdx, &hDistance, 
				 &Bearing, &vDistance);
  }
  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
  for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
    if ((it->data.IsCircle == IsCircle) 
	&& (it->data.AirspaceIndex == AsIdx)){   // check if already in list
      
      if ((it->data.Sequence == Sequence)){       
	// still updated in real pos calculation
      } else {
	it->data.Sequence = Sequence;
	it->data.TimeOut = 3;
	it->data.hDistance = hDistance;
	it->data.vDistance = vDistance;
	it->data.Bearing = Bearing;
	it->data.PredictedEntryTime = EntryTime;
	it->data.Inside = !Predicted;
	it->data.Predicted = Predicted;
	
	if (calcWarnLevel(&it->data))
	  AirspaceWarnListDoNotify(asaWarnLevelIncreased, &it->data);
	else
	  AirspaceWarnListDoNotify(asaItemChanged, &it->data);
	
      }
      
      it->data.InsideAckTimeOut = 20 / OUTSIDE_CHECK_INTERVAL;
      it->data.TimeOut = OUTSIDE_CHECK_INTERVAL;
      
      FoundInList = true;
      break;
      
    }
  }
      
  if (!FoundInList){
    
    AirspaceInfo_c asi; // not in list, add new
    
    asi.TimeOut = OUTSIDE_CHECK_INTERVAL;
    asi.InsideAckTimeOut = 20 / OUTSIDE_CHECK_INTERVAL;
    asi.Sequence = Sequence;
    
    asi.hDistance = hDistance;
    asi.vDistance = vDistance;
    asi.Bearing = Bearing;
    asi.PredictedEntryTime = EntryTime;  // ETE, ToDo
    
    asi.Acknowledge = 0;
    asi.Inside = !Predicted;
    asi.Predicted = Predicted;
    asi.IsCircle = IsCircle;
    asi.AirspaceIndex = AsIdx;
    asi.SortKey = 0;
    
    calcWarnLevel(&asi);
    
    asi.ID = AsIdx + (IsCircle ? 10000 : 0);
    
    AirspaceWarnings.push_front(asi);
    UpdateAirspaceAckBrush(&asi, 0);
    
    NewAirspaceWarnings = true;
    
    AirspaceWarnListDoNotify(asaItemAdded, &AirspaceWarnings.begin()->data);
  }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnLockList();
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

  int i, idx=0;
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

void AirspaceWarnListProcess(NMEA_INFO *Basic){

  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif

    for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it;){
      
      it->data.TimeOut--;                // age the entry
      
      if (it->data.TimeOut < 0){
	
        it->data.Predicted = false;
        it->data.Inside = false;
        it->data.WarnLevel = 0;

        int hDistance = 0;
        int vDistance = 0;
        int Bearing = 0;

        AirspaceWarnListCalcDistance(Basic, 
				     it->data.IsCircle, 
				     it->data.AirspaceIndex, 
				     &hDistance, &Bearing, &vDistance);

        it->data.hDistance = hDistance;                    // for all: update data
        it->data.vDistance = vDistance;
        it->data.Bearing = Bearing;
        it->data.TimeOut = OUTSIDE_CHECK_INTERVAL;         // retrigger checktimeout

        if (calcWarnLevel(&it->data))
          AirspaceWarnListDoNotify(asaWarnLevelIncreased, &it->data);

        UpdateAirspaceAckBrush(&it->data, 0);

        if (it->data.Acknowledge == 4){                 // whole day achnowledged
          if (it->data.SortKey > 25000) {
            it->data.TimeOut = 60;
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
            it->data.InsideAckTimeOut = 20 / OUTSIDE_CHECK_INTERVAL; 
	    // 20sec outside check interval prevent down ACK on circling
          }
        }

        AirspaceWarnListDoNotify(asaItemChanged, &it->data);

      }


      // SortTheList();

      /*

      // just for debugging

      TCHAR szMessageBuffer[1024];

      if (it->data.IsCircle){

        int i = it->data.AirspaceIndex;

        wsprintf(szMessageBuffer, TEXT("%20s %d %d %5d %5d"), AirspaceCircle[i].Name, it->data.Inside, it->data.Predicted, (int)it->data.hDistance, (int)it->data.vDistance);

      } else {

        int i = it->data.AirspaceIndex;

        wsprintf(szMessageBuffer, TEXT("%20s %d %d %5d %5d"), AirspaceArea[i].Name, it->data.Inside, it->data.Predicted, (int)it->data.hDistance, (int)it->data.vDistance);

      }

      devPutVoice(NULL, szMessageBuffer);
      */

      it = it->next;

    }

    AirspaceWarnListSort();

#ifdef DEBUG_AIRSPACE
    char text[100];
    sprintf(text,"%d # airspace\n", AirspaceWarnGetItemCount());
    DebugStore(text);
#endif

    AirspaceWarnListDoNotify(asaProcessEnd, NULL);

#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }

}

void AirspaceWarnDoAck(int ID, int Ack){
  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
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
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }
}


void AirspaceWarnListClear(void){
  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
      UpdateAirspaceAckBrush(&it->data, -1);
    }
    AirspaceWarnings.clear();
    AirspaceWarnListDoNotify(asaClearAll ,NULL);
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }
}


void AirspaceWarnListInit(void){
  InitializeCriticalSection(&csAirspaceWarnings);
}

void AirspaceWarnListDeInit(void){
  DeleteCriticalSection(&csAirspaceWarnings);
}

int AirspaceWarnFindIndexByID(int ID){
  int idx=0;
  int res = -1;
  LockList();
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    for (List<AirspaceInfo_c>::Node* it = AirspaceWarnings.begin(); it; it = it->next ){
      if (it->data.ID == ID){
        res = idx;
        break;
      }
      idx++;
    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
  {
    UnLockList();
  }
  return(res);
}

