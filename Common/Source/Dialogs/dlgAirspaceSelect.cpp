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
#include "Airspace/AirspaceSorter.hpp"
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

static AirspaceSelectInfoVector AirspaceSelectInfo;

static AirspaceSorter* airspace_sorter;

static void
OnAirspaceListEnter(unsigned i)
{
  if (!UpLimit || i >= UpLimit || i>= AirspaceSelectInfo.size())
    return;

  dlgAirspaceDetails(*AirspaceSelectInfo[i].airspace);
}


static void UpdateList(void)
{
  AirspaceSelectInfo = airspace_sorter->get_list();

  if (TypeFilterIdx) {
    airspace_sorter->filter_class(AirspaceSelectInfo, (AirspaceClass_t)(TypeFilterIdx-1));
  }
  
  bool sort_distance = false;
  if (DistanceFilterIdx) {
    sort_distance = true;
    airspace_sorter->filter_distance(AirspaceSelectInfo, DistanceFilter[DistanceFilterIdx]);
  } 
  if (DirectionFilterIdx) {
    sort_distance = true;
    int a = DirectionFilter[DirectionFilterIdx];
    if (a == DirHDG) {
      a = iround(XCSoarInterface::Basic().Heading);
      lastHeading = a;
    }
    airspace_sorter->filter_direction(AirspaceSelectInfo, a);
  }
  if (sort_distance) {
    airspace_sorter->sort_distance(AirspaceSelectInfo);
  }
  if (NameFilterIdx) {
    airspace_sorter->filter_name(AirspaceSelectInfo, (NameFilter[NameFilterIdx])&0xff);
  }

  UpLimit = AirspaceSelectInfo.size();
  wAirspaceList->SetLength(UpLimit);
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
    int a = iround(XCSoarInterface::Basic().Heading);
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
  if ((i >= UpLimit) || (i>=AirspaceSelectInfo.size())) {
    if (!i) {
      canvas.text(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), gettext(_T("No Match!")));
    }
    return;
  }

  const AbstractAirspace &airspace = *AirspaceSelectInfo[i].airspace;
    
  int w0, w1, w2, w3, x1, x2, x3;
  w0 = rc.right - rc.left - Layout::FastScale(4);
  w1 = canvas.text_width(_T("XXX"));
  w2 = canvas.text_width(_T(" 000km"));
  w3 = canvas.text_width(_T(" 000")_T(DEG));
  
  x1 = w0-w1-w2-w3;
    
  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2),
                      x1 - Layout::FastScale(5), airspace.get_name_text(false).c_str());
    
  // left justified
  canvas.text(rc.left + x1, rc.top + Layout::FastScale(2), 
              airspace.get_type_text(true).c_str());
    
  TCHAR sTmp[12];

  // right justified after airspace type
  _stprintf(sTmp, _T("%d%s"),
            AirspaceSelectInfo[i].Distance.as_int(),
            Units::GetDistanceName());
  x2 = w0 - w3 - canvas.text_width(sTmp);
  canvas.text(rc.left + x2, rc.top + Layout::FastScale(2), sTmp);
    
  // right justified after distance
  _stprintf(sTmp, _T("%d")_T(DEG),  
            AirspaceSelectInfo[i].Direction.as_int());
  x3 = w0 - canvas.text_width(sTmp);
  canvas.text(rc.left + x3, rc.top + Layout::FastScale(2), sTmp);
}


static void OnWPSCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancel);
}

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  if (DirectionFilterIdx == 1){
    int a;
    a = (lastHeading - iround(XCSoarInterface::Basic().Heading));
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
  DeclareCallBackEntry(NULL)
};


void dlgAirspaceSelect(void) {

  XCSoarInterface::StartHourglassCursor();

  UpLimit = 0;
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
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
  wAirspaceList->SetPaintItemCallback(OnPaintListItem);

  wpName = (WndProperty*)wf->FindByName(_T("prpFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));

  AirspaceSorter g_airspace_sorter(airspace_database, Location, DISTANCEMODIFY);
  airspace_sorter = &g_airspace_sorter;

  UpdateList();

  wf->SetTimerNotify(OnTimerNotify);

  XCSoarInterface::StopHourglassCursor();
  wf->ShowModal();

  delete wf;

  wf = NULL;

  return;
}

