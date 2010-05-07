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
#include "Math/Earth.hpp"
#include "Screen/Layout.hpp"
#include "Compatibility/string.h"
#include "Math/FastMath.h"
#include "DataField/Base.hpp"
#include "Waypoint/WaypointSorter.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "DataField/Enum.hpp"
#include "LogFile.hpp"

#include <assert.h>
#include <stdlib.h>

static TCHAR sNameFilter[NAMEFILTERLEN+1];


static GEOPOINT Location;

static WndForm *wf=NULL;
static WndListFrame *wWayPointList=NULL;
static WndButton *wbName;
static WndProperty *wpDistance;
static WndProperty *wpDirection;
static WndProperty *wpType;

static const fixed DistanceFilter[] = {
  fixed_zero, fixed(25.0), fixed(50.0),
  fixed(75.0), fixed(100.0), fixed(150.0),
  fixed(250.0), fixed(500.0), fixed(1000.0),
};

#define DirHDG -1
static int DirectionFilter[] = {0, DirHDG, 360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330};

static int lastHeading=0;

static const TCHAR *TypeFilter[] = {_T("*"), _T("Airport"), _T("Landable"),
				    _T("Turnpoint"), _T("File 1"), _T("File 2")};

static TCHAR * GetDirectionData(int DirectionFilterIdx);

static void
OnWaypointListEnter(unsigned i)
{
  wf->SetModalResult(mrOK);
}

static WaypointSelectInfoVector WayPointSelectInfo;

static WaypointSorter* waypoint_sorter;
static unsigned UpLimit = 0;


static void InitializeDirection(bool bOnlyHeading) {
  if (wpDirection) {  // initialize datafieldenum for Direction
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpDirection->GetDataField();
    if (!bOnlyHeading) {
      for (unsigned int i=0; i < sizeof(DirectionFilter) / sizeof(DirectionFilter[0]); i++) {
        dfe->addEnumText(GetDirectionData(i));
      }
      dfe->SetAsInteger(0);
    }
    dfe->replaceEnumText(1,GetDirectionData(1)); // update heading value to current heading
    wpDirection->RefreshDisplay();
  }
}

static void SetNameCaptionFlushLeft(const TCHAR * sIn) { // sets button with enough spaces to appear flush left

  //ToDo: RLD make the text sflush left instead of centered on button
  wbName->SetCaption(sIn);
}

static void PrepareData(void){

  TCHAR sTmp[15];

  sNameFilter[0]='\0';
  SetNameCaptionFlushLeft(_T("*"));

  if (wpDistance) {  // initialize datafieldenum for Distance
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpDistance->GetDataField();
    for (unsigned i=0; i < sizeof(DistanceFilter) / sizeof(DistanceFilter[0]); i++) {
      if (i== 0) {
        sTmp[0]='*';
        sTmp[1]='\0';
      }
      else {
        _stprintf(sTmp, TEXT("%.0f%s"),
                (double)DistanceFilter[i],
                Units::GetDistanceName());
      }
      dfe->addEnumText(sTmp);
    }
    dfe->SetAsInteger(0);
    wpDistance->RefreshDisplay();
  }

  InitializeDirection(false);

  if (wpType) {  // initialize datafieldenum for Type
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpType->GetDataField();
    for (unsigned i=0; i < sizeof(TypeFilter) / sizeof(TypeFilter[0]); i++) {
      _stprintf(sTmp, TEXT("%s"), TypeFilter[i]);
      dfe->addEnumText(sTmp);
    }
    dfe->SetAsInteger(0);
    wpType->RefreshDisplay();
  }


}


static void UpdateList(void)
{
  WayPointSelectInfo = waypoint_sorter->get_list();

  switch( wpType->GetDataField()->GetAsInteger() ) {
  case 1: 
    waypoint_sorter->filter_airport(WayPointSelectInfo);
    break;
  case 2:
    waypoint_sorter->filter_landable(WayPointSelectInfo);
    break;
  case 3: 
    waypoint_sorter->filter_turnpoint(WayPointSelectInfo);
    break;
  case 4:
  case 5:
    waypoint_sorter->filter_file(WayPointSelectInfo, wpType->GetDataField()->GetAsInteger()-4);
    break;
  default:
    break;
  }

  bool sort_distance = false;
  if (wpDistance->GetDataField()->GetAsInteger()) {
    sort_distance = true;
    waypoint_sorter->filter_distance(WayPointSelectInfo, DistanceFilter[wpDistance->GetDataField()->GetAsInteger()]);
  } 

  if (wpDirection->GetDataField()->GetAsInteger()) {
    sort_distance = true;
    int a = DirectionFilter[wpDirection->GetDataField()->GetAsInteger()];
    if (a == DirHDG) {
      a = iround(XCSoarInterface::Basic().Heading.value());
      lastHeading = a;
    }
    waypoint_sorter->filter_direction(WayPointSelectInfo, fixed(a));
  }

  if (sort_distance) {
    waypoint_sorter->sort_distance(WayPointSelectInfo);
  }
  if (_tcslen(sNameFilter) > 0) {
    waypoint_sorter->filter_name(WayPointSelectInfo, sNameFilter);
  }

  UpLimit = WayPointSelectInfo.size();
  wWayPointList->SetLength(UpLimit);
  wWayPointList->invalidate();
}

static void FilterMode(bool direction) {
  if (direction) {

    if (wpDistance) {
      wpDistance->GetDataField()->SetDetachGUI(true);
      wpDistance->GetDataField()->SetAsInteger(0);  // "*"
      wpDistance->GetDataField()->SetDetachGUI(false);
      wpDistance->RefreshDisplay();
    }
    if (wpDirection) {
      wpDirection->GetDataField()->SetDetachGUI(true);
      wpDirection->GetDataField()->SetAsInteger(0);
      wpDirection->GetDataField()->SetDetachGUI(false);
      wpDirection->RefreshDisplay();
    }
  }
}


static void
OnFilterNameButton(gcc_unused WndButton &button){


  TCHAR newNameFilter[NAMEFILTERLEN+1];
  _tcsncpy(newNameFilter, sNameFilter, NAMEFILTERLEN);
  dlgTextEntryShowModal(newNameFilter, NAMEFILTERLEN);

  int i= _tcslen(newNameFilter)-1;
  while (i>=0) {
    if (newNameFilter[i]!=_T(' '))
    {
    break;
    }
    newNameFilter[i]=0;
    i--;
  };

  _tcsncpy(sNameFilter, newNameFilter, NAMEFILTERLEN);

  if (wbName) {

    if (sNameFilter[0]=='\0') {
      SetNameCaptionFlushLeft(TEXT("*"));
    }
    else {
      SetNameCaptionFlushLeft(sNameFilter);
    }
  }
  FilterMode(true);
  UpdateList();

}


static void
OnFilterDistance(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daInc:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      FilterMode(false);
      UpdateList();
    break;
  }
}

static TCHAR *
GetDirectionData(int DirectionFilterIdx){

  static TCHAR sTmp[12];

  if (DirectionFilterIdx == 0)
    _stprintf(sTmp, TEXT("%c"), '*');
  else if (DirectionFilterIdx == 1){
    int a = iround(XCSoarInterface::Basic().Heading.AngleLimit360().value());
    _stprintf(sTmp, TEXT("HDG(%d")TEXT(DEG)TEXT(")"), a);
  }else
    _stprintf(sTmp, TEXT("%d")TEXT(DEG), DirectionFilter[DirectionFilterIdx]);

  return sTmp;

}

static void
OnFilterDirection(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      Sender->Set(TEXT("*"));
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daInc:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      FilterMode(false);
      UpdateList();
    break;
  }
}

static void
OnFilterType(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    break;
    case DataField::daChange:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daInc:
      FilterMode(false);
      UpdateList();
    break;
    case DataField::daDec:
      FilterMode(false);
      UpdateList();
    break;
  }
}

static void
OnPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  TCHAR sTmp[12];

  if (i < UpLimit) {

    const Waypoint &way_point = *WayPointSelectInfo[i].way_point;

    int w0, w1, w2, w3, x1, x2, x3;
    w0 = rc.right - rc.left - Layout::FastScale(4);
    w1 = canvas.text_width(_T("XXX"));
    w2 = canvas.text_width(_T(" 000km"));
    w3 = canvas.text_width(_T(" 000")_T(DEG));

    x1 = w0-w1-w2-w3;

    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2),
                        x1 - Layout::FastScale(5),
                        way_point.Name.c_str());

    sTmp[0] = '\0';
    sTmp[1] = '\0';
    sTmp[2] = '\0';

    if (way_point.Flags.Home){
      sTmp[0] = 'H';
    }else
    if (way_point.Flags.Airport){
      sTmp[0] = 'A';
    }else
    if (way_point.Flags.LandPoint){
      sTmp[0] = 'L';
    }

    if (way_point.Flags.TurnPoint) {
      if (sTmp[0] == '\0')
        sTmp[0] = 'T';
      else
        sTmp[1] = 'T';
    }

    // left justified
    canvas.text(rc.left + x1, rc.top + Layout::FastScale(2), sTmp);

    // right justified after waypoint flags
    _stprintf(sTmp, _T("%.0f%s"),
              (double)WayPointSelectInfo[i].Distance,
              Units::GetDistanceName());
    x2 = w0-w3-canvas.text_width(sTmp);
    canvas.text(rc.left + x2, rc.top + Layout::FastScale(2), sTmp);

    // right justified after distance
    _stprintf(sTmp, _T("%d")_T(DEG),
	      iround(WayPointSelectInfo[i].Direction.value()));
    x3 = w0-canvas.text_width(sTmp);
    canvas.text(rc.left + x3, rc.top + Layout::FastScale(2), sTmp);
  } else {
    if (i == 0){
      _stprintf(sTmp, _T("%s"), gettext(_T("No Match!")));
      canvas.text(rc.left + Layout::FastScale(2),
                  rc.top + Layout::FastScale(2), sTmp);
    }
  }

}
static void
OnWPSSelectClicked(gcc_unused WndButton &button){
  OnWaypointListEnter(0);
}

static void
OnWPSCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  if (wpDirection->GetDataField()->GetAsInteger() == 1){
    int a;
    a = (lastHeading - iround(XCSoarInterface::Basic().Heading.value()));
    if (abs(a) > 0){
      UpdateList();
      InitializeDirection(true);
      wpDirection->RefreshDisplay();
    }
  }
  return 0;
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  int NewIndex = wpType->GetDataField()->GetAsInteger();;

  switch(key_code){
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

  if (wpType->GetDataField()->GetAsInteger() != NewIndex){
    FilterMode(false);
    UpdateList();
    wpType->GetDataField()->SetAsInteger(NewIndex);
    wpType->RefreshDisplay();
  }

  return true;
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(NULL)
};

const Waypoint* 
dlgWayPointSelect(SingleWindow &parent,
                  const GEOPOINT &location,
                  const int type, const int FilterNear)
{
  UpLimit = 0;

  Location = location;

  if (type > -1){
    wpDistance->GetDataField()->SetAsInteger(type);
  }
  if (FilterNear){
    wpDistance->GetDataField()->SetAsInteger(1);
  }

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgWayPointSelect_L.xml"),
                        parent,
                        _T("IDR_XML_WAYPOINTSELECT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgWayPointSelect.xml"),
                        parent,
                        _T("IDR_XML_WAYPOINTSELECT"));
  }

  if (!wf) return NULL;

  assert(wf!=NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->
   FindByName(_T("cmdClose")))->
    SetOnClickNotify(OnWPSCloseClicked);

  ((WndButton *)wf->
   FindByName(_T("cmdSelect")))->
    SetOnClickNotify(OnWPSSelectClicked);

  ((WndButton *)wf->
   FindByName(_T("cmdFltName")))->
    SetOnClickNotify(OnFilterNameButton);

  wWayPointList = (WndListFrame*)wf->FindByName(_T("frmWayPointList"));
  assert(wWayPointList!=NULL);
  wWayPointList->SetActivateCallback(OnWaypointListEnter);
  wWayPointList->SetPaintItemCallback(OnPaintListItem);

  wbName = (WndButton*)wf->FindByName(_T("cmdFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));
  wpType = ((WndProperty *)wf->FindByName(TEXT("prpFltType")));

  WaypointSorter g_waypoint_sorter(way_points, location, fixed(Units::ToUserDistance(1)));
  waypoint_sorter = &g_waypoint_sorter;
  PrepareData();
  UpdateList();

  wf->SetTimerNotify(OnTimerNotify);

  const Waypoint* wp_selected = NULL;

  if (wf->ShowModal() == mrOK) {
    unsigned ItemIndex = wWayPointList->GetCursorIndex();
    if (ItemIndex < UpLimit) {
      wp_selected = WayPointSelectInfo[ItemIndex].way_point;
    }
  }

  delete wf;

  wf = NULL;

  return wp_selected;
}


