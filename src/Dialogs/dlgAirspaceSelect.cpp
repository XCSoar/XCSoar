/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"
#include "DataField/String.hpp"
#include "Components.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Busy.hpp"
#include "Compiler.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static WndForm *wf=NULL;
static WndListFrame *wAirspaceList=NULL;

static TCHAR NameFilter[] = _T("*ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
static unsigned NameFilterIdx=0;

static const fixed DistanceFilter[] = {
  fixed_zero, fixed(25.0), fixed(50.0),
  fixed(75.0), fixed(100.0), fixed(150.0),
  fixed(250.0), fixed(500.0), fixed(1000.0),
};

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

static AirspaceSelectInfoVector AirspaceSelectInfo;

static AirspaceSorter* airspace_sorter;

static void
OnAirspaceListEnter(unsigned i)
{
  if (AirspaceSelectInfo.empty()) {
    assert(i == 0);
    return;
  }

  assert(i < AirspaceSelectInfo.size());

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
      a = uround(CommonInterface::Calculated().Heading.value_degrees());
      lastHeading = a;
    }
    airspace_sorter->filter_direction(AirspaceSelectInfo, 
                                      Angle::degrees(fixed(a)));
  }
  if (sort_distance) {
    airspace_sorter->sort_distance(AirspaceSelectInfo);
  }
  if (NameFilterIdx) {
    airspace_sorter->filter_name(AirspaceSelectInfo, (NameFilter[NameFilterIdx])&0xff);
  }

  wAirspaceList->SetLength(max((size_t)1, AirspaceSelectInfo.size()));
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
      DataFieldString *df = (DataFieldString *)wpDistance->GetDataField();
      df->Set(_T("*"));
      wpDistance->RefreshDisplay();
    }
    if (wpDirection) {
      DataFieldString *df = (DataFieldString *)wpDirection->GetDataField();
      df->Set(_T("*"));
      wpDirection->RefreshDisplay();
    }
  } else {
    NameFilterIdx=0;
    if (wpName) {
      DataFieldString *df = (DataFieldString *)wpName->GetDataField();
      df->Set(_T("**"));
      wpName->RefreshDisplay();
    }
  }
}


static void OnFilterName(DataField *_Sender, DataField::DataAccessKind_t Mode){
  DataFieldString *Sender = (DataFieldString *)_Sender;

  TCHAR sTmp[12];

  switch(Mode){
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

  case DataField::daSpecial:
    return;
  }
  _stprintf(sTmp, _T("%c*"), NameFilter[NameFilterIdx]);
  Sender->Set(sTmp);

}



static void OnFilterDistance(DataField *_Sender,
                             DataField::DataAccessKind_t Mode) {
  DataFieldString *Sender = (DataFieldString *)_Sender;
  TCHAR sTmp[12];

  switch(Mode){
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

  case DataField::daSpecial:
    return;
  }

  if (DistanceFilterIdx == 0)
    _stprintf(sTmp, _T("%c"), '*');
  else
    _stprintf(sTmp, _T("%.0f%s"),
              (double)DistanceFilter[DistanceFilterIdx],
              Units::GetDistanceName());
  Sender->Set(sTmp);
}


static void SetDirectionData(DataFieldString *Sender){

  TCHAR sTmp[12];

  if (Sender == NULL){
    Sender = (DataFieldString *)wpDirection->GetDataField();
  }

  if (DirectionFilterIdx == 0)
    _stprintf(sTmp, _T("%c"), '*');
  else if (DirectionFilterIdx == 1){
    int a = iround(CommonInterface::Calculated().Heading.value_degrees());
    if (a <=0)
      a += 360;
    _stprintf(sTmp, _T("HDG(%d")_T(DEG)_T(")"), a);
  }else
    _stprintf(sTmp, _T("%d")_T(DEG), DirectionFilter[DirectionFilterIdx]);

  Sender->Set(sTmp);

}

static void OnFilterDirection(DataField *_Sender,
                              DataField::DataAccessKind_t Mode){
  DataFieldString *Sender = (DataFieldString *)_Sender;

  switch(Mode){
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

  case DataField::daSpecial:
    return;
  }

  SetDirectionData(Sender);

}

static void OnFilterType(DataField *_Sender,
                         DataField::DataAccessKind_t Mode) {
  DataFieldString *Sender = (DataFieldString *)_Sender;

  switch(Mode){
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

  case DataField::daSpecial:
    return;
  }

  Sender->Set(TypeFilter[TypeFilterIdx]);
}

static void
OnPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  if (AirspaceSelectInfo.empty()) {
    assert(i == 0);

    canvas.text(rc.left + Layout::FastScale(2),
                rc.top + Layout::FastScale(2), _("No Match!"));
    return;
  }

  assert(i < AirspaceSelectInfo.size());

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
              airspace.get_type_text(true));
    
  TCHAR sTmp[12];

  // right justified after airspace type
  _stprintf(sTmp, _T("%d%s"),
            (int)AirspaceSelectInfo[i].Distance,
            Units::GetDistanceName());
  x2 = w0 - w3 - canvas.text_width(sTmp);
  canvas.text(rc.left + x2, rc.top + Layout::FastScale(2), sTmp);
    
  // right justified after distance
  _stprintf(sTmp, _T("%d")_T(DEG),  
            (int)AirspaceSelectInfo[i].Direction.value_degrees());
  x3 = w0 - canvas.text_width(sTmp);
  canvas.text(rc.left + x3, rc.top + Layout::FastScale(2), sTmp);
}


static void
OnWPSCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  if (DirectionFilterIdx == 1){
    int a;
    a = (lastHeading - iround(CommonInterface::Calculated().Heading.value_degrees()));
    if (abs(a) > 0){
      UpdateList();
      SetDirectionData(NULL);
      wpDirection->RefreshDisplay();
    }
  }
}

#ifdef GNAV

static bool
FormKeyDown(WndForm &Sender, unsigned key_code){

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

#endif /* GNAV */

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnFilterName),
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(NULL)
};

static void
PrepareAirspaceSelectDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  !Layout::landscape ? _T("IDR_XML_AIRSPACESELECT_L") :
                                       _T("IDR_XML_AIRSPACESELECT"));
  assert(wf != NULL);

#ifdef GNAV
  wf->SetKeyDownNotify(FormKeyDown);
#endif

  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnWPSCloseClicked);

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceList"));
  assert(wAirspaceList != NULL);
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
  wAirspaceList->SetPaintItemCallback(OnPaintListItem);

  wpName = (WndProperty*)wf->FindByName(_T("prpFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));

  wf->SetTimerNotify(OnTimerNotify);
}

void
dlgAirspaceSelect()
{
  PrepareAirspaceSelectDialog();

  GeoPoint Location = XCSoarInterface::Basic().Location;
  AirspaceSorter g_airspace_sorter(airspace_database, Location,
                                   Units::ToUserDistance(fixed_one));
  airspace_sorter = &g_airspace_sorter;

  UpdateList();

  wf->ShowModal();
  delete wf;
}

