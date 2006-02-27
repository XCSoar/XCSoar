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
#include <Aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"

typedef struct{
  int Index;
  double Distance;
  double Direction;
  int DirectionErr;
  unsigned int FourChars;
}WayPointSelectInfo_t;

static WndForm *wf=NULL;
static WndListFrame *wWayPointList=NULL;
static WndOwnerDrawFrame *wWayPointListEntry = NULL;

static TCHAR NameFilter[] = TEXT("*ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
static int NameFilterIdx=0;

static double DistanceFilter[] = {0.0, 25.0, 50.0, 75.0, 100.0, 150.0, 250.0, 500.0, 1000.0};
static int DistanceFilterIdx=0;

static int DirectionFilter[] = {0, 360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330};
static int DirectionFilterIdx=0;

static int UpLimit=0;
static int LowLimit=0;

static int ItemIndex = -1;

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  switch(wParam & 0xffff){
    case VK_RETURN:
      if (ItemIndex != -1)
        wf->SetModalResult(mrOK);
      else
        wf->SetModalResult(mrCancle);
    return(0);
  }
  return(1);
}

static WayPointSelectInfo_t *WayPointSelectInfo=NULL;

static int _cdecl WaypointNameCompare(const void *elem1, const void *elem2 ){
  if (((WayPointSelectInfo_t *)elem1)->FourChars < ((WayPointSelectInfo_t *)elem2)->FourChars)
    return (-1);
  if (((WayPointSelectInfo_t *)elem1)->FourChars > ((WayPointSelectInfo_t *)elem2)->FourChars)
    return (+1);
  return (0);
}

static int _cdecl WaypointDistanceCompare(const void *elem1, const void *elem2 ){
  if (((WayPointSelectInfo_t *)elem1)->Distance < ((WayPointSelectInfo_t *)elem2)->Distance)
    return (-1);
  if (((WayPointSelectInfo_t *)elem1)->Distance > ((WayPointSelectInfo_t *)elem2)->Distance)
    return (+1);
  return (0);
}

static int _cdecl WaypointDirectionCompare(const void *elem1, const void *elem2 ){

  int a1, a2;

  a1 = (int)(((WayPointSelectInfo_t *)elem1)->Direction - DirectionFilter[DirectionFilterIdx]);
  a2 = (int)(((WayPointSelectInfo_t *)elem2)->Direction - DirectionFilter[DirectionFilterIdx]);

  if (a1 > 180)
    a1 -=360;

  if (a1 < -180)
    a1 +=360;

  if (a2 > 180)
    a2 -=360;

  if (a2 < -180)
    a2 +=360;

  a1 = abs(a1);
  a2 = abs(a2);

  ((WayPointSelectInfo_t *)elem1)->DirectionErr = a1;
  ((WayPointSelectInfo_t *)elem2)->DirectionErr = a2;

  if (a1 < a2)
    return (-1);
  if (a1 > a2)
    return (+1);

  return (0);
}

static void PrepareData(void){

  TCHAR sTmp[5];

  WayPointSelectInfo = (WayPointSelectInfo_t*)malloc(sizeof(WayPointSelectInfo_t) * NumberOfWayPoints);

  for (int i=0; i<(int)NumberOfWayPoints; i++){
    WayPointSelectInfo[i].Index = i;
    WayPointSelectInfo[i].Distance
                    = Distance(GPS_INFO.Latitude,
                        GPS_INFO.Longitude,
                        WayPointList[i].Latitude,
                        WayPointList[i].Longitude)*DISTANCEMODIFY;

    WayPointSelectInfo[i].Direction
                    = Bearing(GPS_INFO.Latitude,
                      GPS_INFO.Longitude,
                      WayPointList[i].Latitude,
                      WayPointList[i].Longitude);

    _tcsncpy(sTmp, WayPointList[i].Name, 4);
    sTmp[4] = '\0';
    _tcsupr(sTmp);

    WayPointSelectInfo[i].FourChars =
                    (((DWORD)sTmp[0] & 0xff) << 24)
                  + (((DWORD)sTmp[1] & 0xff) << 16)
                  + (((DWORD)sTmp[2] & 0xff) << 8)
                  + (((DWORD)sTmp[3] & 0xff) );
  }

  qsort(WayPointSelectInfo, UpLimit,
      sizeof(WayPointSelectInfo_t), WaypointNameCompare);

}


static void UpdateList(void){

//  TCHAR sTmp[128];
  int i;

  ItemIndex = 0;

  UpLimit=NumberOfWayPoints;
  LowLimit =0;

  if (DistanceFilterIdx != 0){

    qsort(WayPointSelectInfo, NumberOfWayPoints,
        sizeof(WayPointSelectInfo_t), WaypointDistanceCompare);


    for (i=0; i<(int)NumberOfWayPoints; i++){
      if (WayPointSelectInfo[i].Distance > DistanceFilter[DistanceFilterIdx]){

        UpLimit = i;

        break;

      }

    }

  }


  if (DirectionFilterIdx != 0){


    qsort(WayPointSelectInfo, UpLimit,
        sizeof(WayPointSelectInfo_t), WaypointDirectionCompare);


    for (i=0; i<UpLimit; i++){

      if (WayPointSelectInfo[i].DirectionErr > 18){

        UpLimit = i;

        break;

      }

    }


  }


  if (NameFilterIdx != 0){

    TCHAR sTmp[8];


    qsort(WayPointSelectInfo, UpLimit,

        sizeof(WayPointSelectInfo_t), WaypointNameCompare);


    sTmp[0] = NameFilter[NameFilterIdx];

    sTmp[2] = '\0';

    _tcsupr(sTmp);


    for (i=0; i<UpLimit; i++){

      if ((BYTE)(WayPointSelectInfo[i].FourChars >> 24) >= (sTmp[0]&0xff)){

        LowLimit = i;

        break;

      }

    }

    for (; i<UpLimit; i++){

      if ((BYTE)(WayPointSelectInfo[i].FourChars >> 24) != (sTmp[0]&0xff)){

        UpLimit = i;

        break;

      }

    }

  }


  qsort(&WayPointSelectInfo[LowLimit], UpLimit-LowLimit,

      sizeof(WayPointSelectInfo_t), WaypointNameCompare);


  wWayPointList->ResetList();

  wWayPointList->Redraw();


}




static void OnFilterName(DataField *Sender, DataField::DataAccessKind_t Mode){

  TCHAR sTmp[12];

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      NameFilterIdx++;
      if (NameFilterIdx > sizeof(NameFilter)/sizeof(NameFilter[0])-2)
        NameFilterIdx = 1;
      UpdateList();
    break;
    case DataField::daDec:
      NameFilterIdx--;
      if (NameFilterIdx < 0)
        NameFilterIdx = sizeof(NameFilter)/sizeof(NameFilter[0])-1;
      UpdateList();
    break;
  }

  _stprintf(sTmp, TEXT("%c*"), NameFilter[NameFilterIdx]);
  Sender->Set(sTmp);

}



static void OnFilterDistance(DataField *Sender, DataField::DataAccessKind_t Mode){

  TCHAR sTmp[12];

  switch(Mode){
    case DataField::daGet:
      Sender->Set(TEXT("25"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      DistanceFilterIdx++;
      if (DistanceFilterIdx > sizeof(DistanceFilter)/sizeof(DistanceFilter[0])-1)
        DistanceFilterIdx = 0;
      UpdateList();
    break;
    case DataField::daDec:
      DistanceFilterIdx--;
      if (DistanceFilterIdx < 0)
        DistanceFilterIdx = sizeof(DistanceFilter)/sizeof(DistanceFilter[0])-1;
      UpdateList();
    break;
  }

  if (DistanceFilterIdx == 0)
    _stprintf(sTmp, TEXT("%c"), '*');
  else                      // todo user unit
    _stprintf(sTmp, TEXT("%.0fkm"), DistanceFilter[DistanceFilterIdx]);
  Sender->Set(sTmp);

}

static void OnFilterDirection(DataField *Sender, DataField::DataAccessKind_t Mode){

  TCHAR sTmp[12];

  switch(Mode){
    case DataField::daGet:
      Sender->Set(TEXT("*"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      DirectionFilterIdx++;
      if (DirectionFilterIdx > sizeof(DirectionFilter)/sizeof(DirectionFilter[0])-1)
        DirectionFilterIdx = 0;
      UpdateList();
    break;
    case DataField::daDec:
      DirectionFilterIdx--;
      if (DirectionFilterIdx < 0)
        DirectionFilterIdx = sizeof(DirectionFilter)/sizeof(DirectionFilter[0])-1;
      UpdateList();
    break;
  }

  if (DirectionFilterIdx == 0)
    _stprintf(sTmp, TEXT("%c"), '*');
  else
    _stprintf(sTmp, TEXT("%d°"), DirectionFilter[DirectionFilterIdx]);
  Sender->Set(sTmp);

}

static int DrawListIndex=0;

void OnPaintListItem(WindowControl * Sender, HDC hDC){

  int n = UpLimit - LowLimit;
  TCHAR sTmp[12];

  if (DrawListIndex < n){

    int i = LowLimit + DrawListIndex;

    ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      WayPointList[WayPointSelectInfo[i].Index].Name,
      _tcslen(WayPointList[WayPointSelectInfo[i].Index].Name), NULL);

    if (WayPointList[WayPointSelectInfo[i].Index].Flags & HOME){
      sTmp[0] = 'H';
    }else
    if (WayPointList[WayPointSelectInfo[i].Index].Flags & AIRPORT){
      sTmp[0] = 'A';
    }else
    if (WayPointList[WayPointSelectInfo[i].Index].Flags & LANDPOINT){
      sTmp[0] = 'L';
    }else
      sTmp[0] = 'T';

    ExtTextOut(hDC, 135*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      sTmp, 1, NULL);
                           //todo user unit
    _stprintf(sTmp, TEXT("%.0fkm"), WayPointSelectInfo[i].Distance);
    ExtTextOut(hDC, 146*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);

    _stprintf(sTmp, TEXT("%d°"),  iround(WayPointSelectInfo[i].Direction));
    ExtTextOut(hDC, 185*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
      ETO_OPAQUE, NULL,
      sTmp, _tcslen(sTmp), NULL);

  } else {
    if (DrawListIndex == 0){
      _stprintf(sTmp, TEXT("%s"), TEXT("No Match!"));
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
        ETO_OPAQUE, NULL,
        sTmp, _tcslen(sTmp), NULL);
    }
  }

}

// DrawListIndex = number of things to draw
// ItemIndex = current selected item


void OnWpListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = UpLimit-LowLimit;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
    ItemIndex = ListInfo->ItemIndex+ListInfo->ScrollIndex;
  }
}

static void OnCloseClicked(WindowControl * Sender){
  ItemIndex = -1;
  wf->SetModalResult(mrCancle);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(FormKeyDown),
  DeclearCallBackEntry(OnFilterName),
  DeclearCallBackEntry(OnFilterDistance),
  DeclearCallBackEntry(OnFilterDirection),
  DeclearCallBackEntry(OnPaintListItem),
  DeclearCallBackEntry(OnWpListInfo),
  DeclearCallBackEntry(NULL)
};

int dlgWayPointSelect(void){

  UpLimit = 0;
  LowLimit = 0;
  ItemIndex = -1;

  wf = dlgLoadFromXML(CallBackTable, "\\NOR Flash\\dlgWayPointSelect.xml", hWndMainWindow);

  if (!wf) return -1;

  ASSERT(wf!=NULL);
  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->
   FindByName(TEXT("cmdClose")))->
    SetOnClickNotify(OnCloseClicked);

  wWayPointList = (WndListFrame*)wf->FindByName(TEXT("frmWayPointList"));
  ASSERT(wWayPointList!=NULL);
  wWayPointList->SetBorderKind(BORDERLEFT);

  wWayPointListEntry = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmWayPointListEntry"));
  ASSERT(wWayPointListEntry!=NULL);
  wWayPointListEntry->SetCanFocus(true);

  PrepareData();
  UpdateList();

  if ((wf->ShowModal() == mrOK) && (UpLimit - LowLimit > 0) &&
      (ItemIndex >= 0)  // JMW fixed bug, was >0
      && (ItemIndex < (UpLimit - LowLimit))) {
    ItemIndex = WayPointSelectInfo[LowLimit + ItemIndex].Index;
  }else
    ItemIndex = -1;

  free(WayPointSelectInfo);

  delete wf;

  wf = NULL;

  return(ItemIndex);

}

