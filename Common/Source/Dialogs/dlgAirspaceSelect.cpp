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

#include "Dialogs/Internal.hpp"
#include "Blackboard.hpp"
#include "Airspace.h"
#include "AirspaceDatabase.hpp"
#include "AirspaceWarning.h"
#include "Compatibility/string.h"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"
#include "DataField/Base.hpp"
#include "MapWindow.h"
#include "Components.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>
#include <stdlib.h>

typedef struct{
  int Index_Circle;
  int Index_Area;
  double Distance;
  double Direction;
  int    DirectionErr;
  int    Type;
  unsigned int FourChars;
} AirspaceSelectInfo_t;

static GEOPOINT Location;

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;

static TCHAR NameFilter[] = _T("*ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
static unsigned NameFilterIdx=0;

static double DistanceFilter[] = {0.0, 25.0, 50.0, 75.0, 100.0, 150.0,
                                  250.0, 500.0, 1000.0};
static unsigned DistanceFilterIdx=0;

#define DirHDG -1

static int DirectionFilter[] = {0, DirHDG, 360, 30, 60, 90, 120, 150,
                                180, 210, 240, 270, 300, 330};
static unsigned DirectionFilterIdx=0;
static int lastHeading=0;

static unsigned NumberOfAirspaces = 0;

static const TCHAR *TypeFilter[] = {_T("*"),
				    _T("Other"),
				    _T("Restricted areas"),
				    _T("Prohibited areas"),
				    _T("Danger areas"),
				    _T("Class A"),
				    _T("Class B"),
				    _T("Class C"),
				    _T("Class D"),
				    _T("No gliders"),
				    _T("CTR"),
				    _T("Wave"),
				    _T("AAT"),
				    _T("Class E"),
				    _T("Class F"),
};

static unsigned TypeFilterIdx=0;

static unsigned UpLimit=0;
static unsigned LowLimit=0;

static AirspaceSelectInfo_t *AirspaceSelectInfo=NULL;


static void OnAirspaceListEnter(WindowControl * Sender,
				WndListFrame::ListInfo_t *ListInfo){
  (void)Sender; (void)ListInfo;

  int ItemIndex = wAirspaceList->GetCursorIndex();
  if (ItemIndex != -1) {

    if (UpLimit > LowLimit
        && (ItemIndex >= 0)  // JMW fixed bug, was >0
        && ((unsigned)ItemIndex < (UpLimit - LowLimit))) {

      int index_circle = AirspaceSelectInfo[LowLimit+ItemIndex].Index_Circle;
      int index_area = AirspaceSelectInfo[LowLimit+ItemIndex].Index_Area;

      if ((index_circle>=0) || (index_area>=0)) {

        TCHAR *Name = NULL;
        if (index_circle>=0) {
          Name = airspace_database.AirspaceCircle[index_circle].Name;
        } else if (index_area>=0) {
          Name = airspace_database.AirspaceArea[index_area].Name;
        }
        if (Name) {
	  UINT answer;
          answer = MessageBoxX(Name,
			       gettext(_T("Acknowledge for day?")),
			       MB_YESNOCANCEL|MB_ICONQUESTION);
	  if (answer == IDYES) {
	    if (index_circle>=0) {
              AirspaceWarnListAdd(airspace_database, XCSoarInterface::Basic(),
                                  XCSoarInterface::Calculated(),
                                  XCSoarInterface::SettingsComputer(),
                                  XCSoarInterface::MapProjection(),
                                  false, true, index_circle, true);
            } else if (index_area>=0) {
              AirspaceWarnListAdd(airspace_database, XCSoarInterface::Basic(),
                                  XCSoarInterface::Calculated(),
                                  XCSoarInterface::SettingsComputer(),
                                  XCSoarInterface::MapProjection(),
                                  false, false, index_area, true);
            }
          } else if (answer == IDNO) {
	    // this will cancel a daily ack
	    if (index_circle>=0) {
              AirspaceWarnListAdd(airspace_database, XCSoarInterface::Basic(),
                                  XCSoarInterface::Calculated(),
                                  XCSoarInterface::SettingsComputer(),
                                  XCSoarInterface::MapProjection(),
                                  true, true, index_circle, true);
            } else if (index_area>=0) {
              AirspaceWarnListAdd(airspace_database, XCSoarInterface::Basic(),
                                  XCSoarInterface::Calculated(),
                                  XCSoarInterface::SettingsComputer(),
                                  XCSoarInterface::MapProjection(),
                                  true, false, index_area, true);
            }
	  }
        }
      }
    }
  } else {
    wf->SetModalResult(mrCancel);
  }
}



static int _cdecl AirspaceNameCompare(const void *elem1, const void *elem2 ){
  if (((AirspaceSelectInfo_t *)elem1)->FourChars < ((AirspaceSelectInfo_t *)elem2)->FourChars)
    return (-1);
  if (((AirspaceSelectInfo_t *)elem1)->FourChars > ((AirspaceSelectInfo_t *)elem2)->FourChars)
    return (+1);
  return (0);
}

static int _cdecl AirspaceDistanceCompare(const void *elem1, const void *elem2 ){
  if (((AirspaceSelectInfo_t *)elem1)->Distance < ((AirspaceSelectInfo_t *)elem2)->Distance)
    return (-1);
  if (((AirspaceSelectInfo_t *)elem1)->Distance > ((AirspaceSelectInfo_t *)elem2)->Distance)
    return (+1);
  return (0);
}

static int _cdecl AirspaceTypeCompare(const void *elem1, const void *elem2 ){
  if (((AirspaceSelectInfo_t *)elem1)->Type == (int)TypeFilterIdx - 1)
    return (-1);
  return (+1);
}


static int _cdecl AirspaceDirectionCompare(const void *elem1, const void *elem2 ){

  int a, a1, a2;

  a = DirectionFilter[DirectionFilterIdx];
  if (a == DirHDG){
    a = iround(XCSoarInterface::Calculated().Heading);
    lastHeading = a;
  }

  a1 = (int)(((AirspaceSelectInfo_t *)elem1)->Direction - a);
  a2 = (int)(((AirspaceSelectInfo_t *)elem2)->Direction - a);

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

  ((AirspaceSelectInfo_t *)elem1)->DirectionErr = a1;
  ((AirspaceSelectInfo_t *)elem2)->DirectionErr = a2;

  if (a1 < a2)
    return (-1);
  if (a1 > a2)
    return (+1);

  return (0);
}

static void PrepareData(void){

  TCHAR sTmp[5];

  if (airspace_database.NumberOfAirspaceAreas == 0 &&
      airspace_database.NumberOfAirspaceCircles == 0)
    return;

  AirspaceSelectInfo = (AirspaceSelectInfo_t*)
    malloc(sizeof(AirspaceSelectInfo_t) * NumberOfAirspaces);

  unsigned index=0;
  unsigned i;

  for (i = 0; i < airspace_database.NumberOfAirspaceCircles; i++) {
    const AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[i];

    AirspaceSelectInfo[index].Index_Circle = i;
    AirspaceSelectInfo[index].Index_Area = -1;

    AirspaceSelectInfo[index].Distance = DISTANCEMODIFY*
      airspace_database.CircleDistance(Location, i);

    DistanceBearing(Location,
                    circle.Location,
                    NULL, &AirspaceSelectInfo[index].Direction);

    _tcsncpy(sTmp, circle.Name, 4);
    sTmp[4] = '\0';
    _tcsupr(sTmp);

    AirspaceSelectInfo[index].FourChars =
                    (((DWORD)sTmp[0] & 0xff) << 24)
                  + (((DWORD)sTmp[1] & 0xff) << 16)
                  + (((DWORD)sTmp[2] & 0xff) << 8)
                  + (((DWORD)sTmp[3] & 0xff) );

    AirspaceSelectInfo[index].Type = circle.Type;

    index++;
  }

  for (i = 0; i < airspace_database.NumberOfAirspaceAreas; i++) {
    const AIRSPACE_AREA &area = airspace_database.AirspaceArea[i];
    MapWindow &map_window = XCSoarInterface::main_window.map;

    AirspaceSelectInfo[index].Index_Circle = -1;
    AirspaceSelectInfo[index].Index_Area = i;

    AirspaceSelectInfo[index].Distance = DISTANCEMODIFY*
      airspace_database.RangeArea(Location, i,
                                  &AirspaceSelectInfo[index].Direction,
                                  map_window);

    _tcsncpy(sTmp, area.Name, 4);
    sTmp[4] = '\0';
    _tcsupr(sTmp);

    AirspaceSelectInfo[index].FourChars =
                    (((DWORD)sTmp[0] & 0xff) << 24)
                  + (((DWORD)sTmp[1] & 0xff) << 16)
                  + (((DWORD)sTmp[2] & 0xff) << 8)
                  + (((DWORD)sTmp[3] & 0xff) );

    AirspaceSelectInfo[index].Type = area.Type;

    index++;
  }

  qsort(AirspaceSelectInfo, UpLimit,
      sizeof(AirspaceSelectInfo_t), AirspaceNameCompare);

}


static void UpdateList(void){

//  TCHAR sTmp[128];
  unsigned i;
  bool distancemode = false;

  UpLimit= NumberOfAirspaces;
  LowLimit =0;

  if (TypeFilterIdx>0) {

    qsort(AirspaceSelectInfo, NumberOfAirspaces,
        sizeof(AirspaceSelectInfo_t), AirspaceTypeCompare);
    for (i = 0; i < NumberOfAirspaces; i++) {
      if (!(AirspaceSelectInfo[i].Type == (int)TypeFilterIdx - 1)){
        UpLimit = i;
        break;
      }
    }
  }

  if (DistanceFilterIdx != 0){
    distancemode = true;
    qsort(AirspaceSelectInfo, UpLimit,
        sizeof(AirspaceSelectInfo_t), AirspaceDistanceCompare);
    for (i = 0; i < UpLimit; i++){
      if (AirspaceSelectInfo[i].Distance > DistanceFilter[DistanceFilterIdx]){
        UpLimit = i;
        break;
      }
    }
  }

  if (DirectionFilterIdx != 0){
    distancemode = true;
    qsort(AirspaceSelectInfo, UpLimit,
        sizeof(AirspaceSelectInfo_t), AirspaceDirectionCompare);
    for (i=0; i<UpLimit; i++){
      if (AirspaceSelectInfo[i].DirectionErr > 18){
        UpLimit = i;
        break;
      }
    }
  }

  if (NameFilterIdx != 0){
    TCHAR sTmp[8];
    LowLimit = UpLimit;
    qsort(AirspaceSelectInfo, UpLimit,
        sizeof(AirspaceSelectInfo_t), AirspaceNameCompare);
    sTmp[0] = NameFilter[NameFilterIdx];
    sTmp[1] = '\0';
    sTmp[2] = '\0';
    _tcsupr(sTmp);
    for (i=0; i<UpLimit; i++){
      if ((BYTE)(AirspaceSelectInfo[i].FourChars >> 24) >= (sTmp[0]&0xff)){
        LowLimit = i;
        break;
      }
    }

    for (; i<UpLimit; i++){
      if ((BYTE)(AirspaceSelectInfo[i].FourChars >> 24) != (sTmp[0]&0xff)){
        UpLimit = i;
        break;
      }
    }
  }

  if (!distancemode) {
    qsort(&AirspaceSelectInfo[LowLimit], UpLimit-LowLimit,
	  sizeof(AirspaceSelectInfo_t), AirspaceNameCompare);
  } else {
    qsort(&AirspaceSelectInfo[LowLimit], UpLimit-LowLimit,
	  sizeof(AirspaceSelectInfo_t), AirspaceDistanceCompare);
  }

  wAirspaceList->ResetList();
  wAirspaceList->invalidate();
}


static WndProperty *wpName;
static WndProperty *wpDistance;
static WndProperty *wpDirection;


static void FilterMode(bool direction) {
  if (direction) {
    DistanceFilterIdx=0;
    DirectionFilterIdx=0;
    if (wpDistance) {
      wpDistance->GetDataField()->Set(_T("*"));
      wpDistance->RefreshDisplay();
    }
    if (wpDirection) {
      wpDirection->GetDataField()->Set(_T("*"));
      wpDirection->RefreshDisplay();
    }
  } else {
    NameFilterIdx=0;
    if (wpName) {
      wpName->GetDataField()->Set(_T("**"));
      wpName->RefreshDisplay();
    }
  }
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
      FilterMode(true);
      UpdateList();
    break;
    case DataField::daDec:
      if (NameFilterIdx == 0)
        NameFilterIdx = sizeof(NameFilter)/sizeof(NameFilter[0])-1;
      else
        NameFilterIdx--;
      FilterMode(true);
      UpdateList();
    break;
  }

  _stprintf(sTmp, _T("%c*"), NameFilter[NameFilterIdx]);
  Sender->Set(sTmp);

}



static void OnFilterDistance(DataField *Sender,
                             DataField::DataAccessKind_t Mode) {

  TCHAR sTmp[12];

  switch(Mode){
    case DataField::daGet:
      Sender->Set(_T("25"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      DistanceFilterIdx++;
      if (DistanceFilterIdx > sizeof(DistanceFilter)/sizeof(DistanceFilter[0])-1)
        DistanceFilterIdx = 0;
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      if (DistanceFilterIdx == 0)
        DistanceFilterIdx = sizeof(DistanceFilter)/sizeof(DistanceFilter[0])-1;
      else
        DistanceFilterIdx--;
      FilterMode(false);
      UpdateList();
    break;
  }

  if (DistanceFilterIdx == 0)
    _stprintf(sTmp, _T("%c"), '*');
  else
    _stprintf(sTmp, _T("%.0f%s"),
              DistanceFilter[DistanceFilterIdx],
              Units::GetDistanceName());
  Sender->Set(sTmp);
}


static void SetDirectionData(DataField *Sender){

  TCHAR sTmp[12];

  if (Sender == NULL){
    Sender = wpDirection->GetDataField();
  }

  if (DirectionFilterIdx == 0)
    _stprintf(sTmp, _T("%c"), '*');
  else if (DirectionFilterIdx == 1){
    int a = iround(XCSoarInterface::Calculated().Heading);
    if (a <=0)
      a += 360;
    _stprintf(sTmp, _T("HDG(%d")_T(DEG)_T(")"), a);
  }else
    _stprintf(sTmp, _T("%d")_T(DEG), DirectionFilter[DirectionFilterIdx]);

  Sender->Set(sTmp);

}

static void OnFilterDirection(DataField *Sender,
                              DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      Sender->Set(_T("*"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      DirectionFilterIdx++;
      if (DirectionFilterIdx > sizeof(DirectionFilter)/sizeof(DirectionFilter[0])-1)
        DirectionFilterIdx = 0;
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      if (DirectionFilterIdx == 0)
        DirectionFilterIdx = sizeof(DirectionFilter)/sizeof(DirectionFilter[0])-1;
      else
        DirectionFilterIdx--;
      FilterMode(false);
      UpdateList();
    break;
  }

  SetDirectionData(Sender);

}

static void OnFilterType(DataField *Sender,
                         DataField::DataAccessKind_t Mode) {

  TCHAR sTmp[20];

  switch(Mode){
    case DataField::daGet:
      Sender->Set(_T("*"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
    break;
    case DataField::daInc:
      TypeFilterIdx++;
      if (TypeFilterIdx > sizeof(TypeFilter)/sizeof(TypeFilter[0])-1)
        TypeFilterIdx = 0;
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      if (TypeFilterIdx == 0)
        TypeFilterIdx = sizeof(TypeFilter)/sizeof(TypeFilter[0])-1;
      else
        TypeFilterIdx--;
      FilterMode(false);
      UpdateList();
    break;
  }

  _stprintf(sTmp, _T("%s"), TypeFilter[TypeFilterIdx]);

  Sender->Set(sTmp);

}

static void
OnPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  int n = UpLimit - LowLimit;
  TCHAR sTmp[12];

  if ((int)i < n) {
    i += LowLimit;

// Sleep(100);
    TCHAR *Name = 0;
    if (AirspaceSelectInfo[i].Index_Circle>=0) {
      Name = airspace_database.AirspaceCircle[AirspaceSelectInfo[i].Index_Circle].Name;
    }
    if (AirspaceSelectInfo[i].Index_Area>=0) {
      Name = airspace_database.AirspaceArea[AirspaceSelectInfo[i].Index_Area].Name;
    }
    if (Name) {

      int w0, w1, w2, w3, x1, x2, x3;
      w0 = Layout::FastScale(Layout::landscape ? 202 : 225);
      w1 = canvas.text_width(_T("XXX"));
      w2 = canvas.text_width(_T(" 000km"));
      w3 = canvas.text_width(_T(" 000")_T(DEG));

      x1 = w0-w1-w2-w3;

      canvas.text_clipped(rc.left + Layout::FastScale(2),
                          rc.top + Layout::FastScale(2),
                          x1 - Layout::FastScale(5), Name);

      sTmp[0] = '\0';
      sTmp[1] = '\0';
      sTmp[2] = '\0';

      switch(AirspaceSelectInfo[i].Type) {
      case CLASSA:
        _tcscpy(sTmp, gettext(_T("A")));
        break;
      case CLASSB:
        _tcscpy(sTmp, gettext(_T("B")));
        break;
      case CLASSC:
        _tcscpy(sTmp, gettext(_T("C")));
        break;
      case CLASSD:
        _tcscpy(sTmp, gettext(_T("D")));
        break;
      case CLASSE:
        _tcscpy(sTmp, gettext(_T("E")));
        break;
      case CLASSF:
        _tcscpy(sTmp, gettext(_T("F")));
        break;
      case PROHIBITED:
        _tcscpy(sTmp, gettext(_T("Prb")));
        break;
      case DANGER:
        _tcscpy(sTmp, gettext(_T("Dgr")));
        break;
      case RESTRICT:
        _tcscpy(sTmp, gettext(_T("Res")));
        break;
      case CTR:
        _tcscpy(sTmp, gettext(_T("CTR")));
        break;
      case NOGLIDER:
        _tcscpy(sTmp, gettext(_T("NoGl")));
        break;
      case WAVE:
        _tcscpy(sTmp, gettext(_T("Wav")));
        break;
      case OTHER:
        _tcscpy(sTmp, gettext(_T("?")));
        break;
      case AATASK:
        _tcscpy(sTmp, gettext(_T("AAT")));
        break;
      default:
        break;
      }

      // left justified

      canvas.text(rc.left + x1, rc.top + Layout::FastScale(2), sTmp);

      // right justified after airspace type
      _stprintf(sTmp, _T("%.0f%s"),
                AirspaceSelectInfo[i].Distance,
                Units::GetDistanceName());
      x2 = w0 - w3 - canvas.text_width(sTmp);
      canvas.text(rc.left + x2, rc.top + Layout::FastScale(2), sTmp);

      // right justified after distance
      _stprintf(sTmp, _T("%d")_T(DEG),  iround(AirspaceSelectInfo[i].Direction));
      x3 = w0 - canvas.text_width(sTmp);
      canvas.text(rc.left + x3, rc.top + Layout::FastScale(2), sTmp);
    } else {
      // should never get here!
    }
  } else {
    if (i == 0){
      _stprintf(sTmp, _T("%s"), gettext(_T("No Match!")));
      canvas.text(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), sTmp);
    }
  }
}

// DrawListIndex = number of things to draw
// ItemIndex = current selected item


static void OnWpListInfo(WindowControl * Sender,
                         WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
	if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = UpLimit-LowLimit;
  }
}


static void OnWPSCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancel);
}

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  if (DirectionFilterIdx == 1){
    int a;
    a = (lastHeading - iround(XCSoarInterface::Calculated().Heading));
    if (abs(a) > 0){
      UpdateList();
      SetDirectionData(NULL);
      wpDirection->RefreshDisplay();
    }
  }
  return 0;
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code){

  WndProperty* wp;
  unsigned NewIndex = TypeFilterIdx;

  wp = ((WndProperty *)wf->FindByName(_T("prpFltType")));

  switch(key_code) {
    case VK_F1:
      NewIndex = 0;
    break;
    case VK_F2:
      NewIndex = 2;
    break;
    case VK_F3:
      NewIndex = 3;
    break;

  default:
    return false;
  }

  if (TypeFilterIdx != NewIndex){
    TypeFilterIdx = NewIndex;
    FilterMode(false);
    UpdateList();
    wp->GetDataField()->SetAsString(TypeFilter[TypeFilterIdx]);
    wp->RefreshDisplay();
  }

  return true;
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnFilterName),
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(OnWpListInfo),
  DeclareCallBackEntry(NULL)
};


void dlgAirspaceSelect(void) {

  XCSoarInterface::StartHourglassCursor();

  UpLimit = 0;
  LowLimit = 0;

  NumberOfAirspaces = airspace_database.NumberOfAirspaceCircles +
    airspace_database.NumberOfAirspaceAreas;

  Location = XCSoarInterface::Basic().Location;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgAirspaceSelect_L.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACESELECT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgAirspaceSelect.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_AIRSPACESELECT"));
  }

  if (!wf) return;

  assert(wf!=NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->
   FindByName(_T("cmdClose")))->
    SetOnClickNotify(OnWPSCloseClicked);

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceList"));
  assert(wAirspaceList!=NULL);
  wAirspaceList->SetBorderKind(BORDERLEFT);
  wAirspaceList->SetEnterCallback(OnAirspaceListEnter);
  wAirspaceList->SetPaintItemCallback(OnPaintListItem);

  wpName = (WndProperty*)wf->FindByName(_T("prpFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));

  PrepareData();
  UpdateList();

  wf->SetTimerNotify(OnTimerNotify);

  XCSoarInterface::StopHourglassCursor();
  wf->ShowModal();

  free(AirspaceSelectInfo);

  delete wf;

  wf = NULL;

  return;

}

