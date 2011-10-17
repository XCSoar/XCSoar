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

#include "Dialogs/Airspace.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/Dialogs.h"
#include "Blackboard.hpp"
#include "Airspace/AirspaceSorter.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"
#include "DataField/String.hpp"
#include "DataField/Enum.hpp"
#include "Components.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Busy.hpp"
#include "Compiler.h"
#include "Util/Macros.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Special enum integer value for "filter disabled".
 */
static gcc_constexpr_data unsigned WILDCARD = 0x7fff;

static WndForm *wf=NULL;
static WndProperty *wpName;
static WndProperty *wpDistance;
static WndProperty *wpDirection;
static WndListFrame *wAirspaceList=NULL;

static fixed distance_filter;

static unsigned direction_filter = WILDCARD;
static Angle last_heading;

static gcc_constexpr_data StaticEnumChoice type_filter_list[] = {
  { WILDCARD, _T("*") },
  { OTHER, _T("Other") },
  { RESTRICT, _T("Restricted areas") },
  { PROHIBITED, _T("Prohibited areas") },
  { DANGER, _T("Danger areas") },
  { CLASSA, _T("Class A") },
  { CLASSB, _T("Class B") },
  { CLASSC, _T("Class C") },
  { CLASSD, _T("Class D") },
  { NOGLIDER, _T("No gliders") },
  { CTR, _T("CTR") },
  { WAVE, _T("Wave") },
  { CLASSE, _T("Class E") },
  { CLASSF, _T("Class F") },
  { 0 }
};

static unsigned TypeFilterIdx = WILDCARD;

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

  if (TypeFilterIdx != WILDCARD)
    airspace_sorter->filter_class(AirspaceSelectInfo, (AirspaceClass)TypeFilterIdx);
  
  bool sort_distance = false;
  if (positive(distance_filter)) {
    sort_distance = true;
    airspace_sorter->filter_distance(AirspaceSelectInfo, distance_filter);
  } 
  if (direction_filter != WILDCARD) {
    sort_distance = true;
    Angle a = direction_filter == 0
      ? CommonInterface::Calculated().heading
      : Angle::Degrees(fixed(direction_filter));
    airspace_sorter->filter_direction(AirspaceSelectInfo, a);
  }
  if (sort_distance) {
    airspace_sorter->sort_distance(AirspaceSelectInfo);
  }

  const TCHAR *name_filter = wpName->GetDataField()->GetAsString();
  if (!string_is_empty(name_filter))
    airspace_sorter->FilterNamePrefix(AirspaceSelectInfo, name_filter);

  wAirspaceList->SetLength(max((size_t)1, AirspaceSelectInfo.size()));
  wAirspaceList->invalidate();
}

static void FilterMode(bool direction) {
  if (direction) {
    distance_filter = fixed_minus_one;
    direction_filter = WILDCARD;
    if (wpDistance) {
      DataFieldEnum &df = *(DataFieldEnum *)wpDistance->GetDataField();
      df.Set(WILDCARD);
      wpDistance->RefreshDisplay();
    }
    if (wpDirection) {
      DataFieldEnum &df = *(DataFieldEnum *)wpDirection->GetDataField();
      df.Set(WILDCARD);
      wpDirection->RefreshDisplay();
    }
  } else {
    if (wpName) {
      DataFieldString *df = (DataFieldString *)wpName->GetDataField();
      df->Set(_T(""));
      wpName->RefreshDisplay();
    }
  }
}


static void OnFilterName(DataField *_Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daChange:
    FilterMode(true);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}



static void OnFilterDistance(DataField *_Sender,
                             DataField::DataAccessKind_t Mode) {
  DataFieldString *Sender = (DataFieldString *)_Sender;

  switch(Mode){
    case DataField::daChange:
    distance_filter = (unsigned)Sender->GetAsInteger() != WILDCARD
      ? fixed(Sender->GetAsInteger())
      : fixed_minus_one;
    FilterMode(false);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void OnFilterDirection(DataField *_Sender,
                              DataField::DataAccessKind_t Mode){
  DataFieldEnum &df = *(DataFieldEnum *)_Sender;

  switch(Mode){
    case DataField::daChange:
    direction_filter = df.GetAsInteger();
    FilterMode(false);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void OnFilterType(DataField *Sender,
                         DataField::DataAccessKind_t Mode) {
  switch(Mode){
    case DataField::daChange:
      TypeFilterIdx = Sender->GetAsInteger();
      FilterMode(false);
      UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
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
                      x1 - Layout::FastScale(5), airspace.GetNameText().c_str());
    
  // left justified
  canvas.text(rc.left + x1, rc.top + Layout::FastScale(2), 
              airspace.GetTypeText(true));
    
  TCHAR sTmp[12];

  // right justified after airspace type
  _stprintf(sTmp, _T("%d%s"),
            (int)AirspaceSelectInfo[i].Distance,
            Units::GetDistanceName());
  x2 = w0 - w3 - canvas.text_width(sTmp);
  canvas.text(rc.left + x2, rc.top + Layout::FastScale(2), sTmp);
    
  // right justified after distance
  _stprintf(sTmp, _T("%d")_T(DEG),  
            (int)AirspaceSelectInfo[i].Direction.Degrees());
  x3 = w0 - canvas.text_width(sTmp);
  canvas.text(rc.left + x3, rc.top + Layout::FastScale(2), sTmp);
}


static void
CloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

gcc_pure
static const TCHAR *
GetHeadingString(TCHAR *buffer)
{
  _stprintf(buffer, _T("%s (%u")_T(DEG)_T(")"), _("Heading"),
            uround(CommonInterface::Calculated().heading.AsBearing().Degrees()));
  return buffer;
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  if (direction_filter == 0 && !CommonInterface::Calculated().circling) {
    Angle a = last_heading - CommonInterface::Calculated().heading;
    if (a.AsDelta().AbsoluteDegrees() >= fixed(10)) {
      last_heading = CommonInterface::Calculated().heading;

      UpdateList();

      DataFieldEnum &df = *(DataFieldEnum *)wpDirection->GetDataField();
      TCHAR buffer[64];
      df.replaceEnumText(0, GetHeadingString(buffer));
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
      NewIndex = WILDCARD;
    break;
    case VK_F2:
      NewIndex = RESTRICT;
    break;
    case VK_F3:
      NewIndex = PROHIBITED;
    break;

  default:
    return false;
  }

  if (TypeFilterIdx != NewIndex){
    wp->GetDataField()->SetAsInteger(NewIndex);
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
  DeclareCallBackEntry(CloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
FillDistanceEnum(DataFieldEnum &df)
{
  df.AddChoice(0, _T("*"));

  static gcc_constexpr_data unsigned distances[] = {
    25, 50, 75, 100, 150, 250, 500, 1000
  };

  TCHAR buffer[64];
  const TCHAR *unit = Units::GetDistanceName();
  for (unsigned i = 0; i < ARRAY_SIZE(distances); ++i) {
    _stprintf(buffer, _T("%u %s"), distances[i], unit);
    df.AddChoice(distances[i], buffer);
  }

  df.Set(0);
}

static void
FillDirectionEnum(DataFieldEnum &df)
{
  TCHAR buffer[64];

  df.AddChoice(WILDCARD, _T("*"));
  df.AddChoice(0, GetHeadingString(buffer));

  static gcc_constexpr_data unsigned directions[] = {
    360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
  };

  for (unsigned i = 0; i < ARRAY_SIZE(directions); ++i) {
    _stprintf(buffer, _T("%d")_T(DEG), directions[i]);
    df.AddChoice(directions[i], buffer);
  }

  df.Set(WILDCARD);
}

static void
PrepareAirspaceSelectDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_AIRSPACESELECT_L") :
                                      _T("IDR_XML_AIRSPACESELECT"));
  assert(wf != NULL);

#ifdef GNAV
  wf->SetKeyDownNotify(FormKeyDown);
#endif

  wAirspaceList = (WndListFrame*)wf->FindByName(_T("frmAirspaceList"));
  assert(wAirspaceList != NULL);
  wAirspaceList->SetActivateCallback(OnAirspaceListEnter);
  wAirspaceList->SetPaintItemCallback(OnPaintListItem);

  wpName = (WndProperty*)wf->FindByName(_T("prpFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  FillDistanceEnum(*(DataFieldEnum *)wpDistance->GetDataField());
  wpDistance->RefreshDisplay();

  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));
  FillDirectionEnum(*(DataFieldEnum *)wpDirection->GetDataField());
  wpDirection->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpFltType"), type_filter_list, WILDCARD);

  wf->SetTimerNotify(OnTimerNotify);
}

void
dlgAirspaceSelect()
{
  PrepareAirspaceSelectDialog();

  GeoPoint Location = XCSoarInterface::Basic().location;
  AirspaceSorter g_airspace_sorter(airspace_database, Location,
                                   Units::ToUserDistance(fixed_one));
  airspace_sorter = &g_airspace_sorter;

  UpdateList();

  wf->ShowModal();
  delete wf;
}

