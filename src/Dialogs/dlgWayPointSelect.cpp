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
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointVisitor.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "DataField/Enum.hpp"
#include "LogFile.hpp"
#include "StringUtil.hpp"

#include <algorithm>

#include <assert.h>
#include <stdlib.h>

static GeoPoint g_location;

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

static Angle last_heading = Angle::native(fixed_zero);

static const TCHAR *const TypeFilter[] = {
  _T("*"), _T("Airport"), _T("Landable"),
  _T("Turnpoint"), _T("File 1"), _T("File 2"),
  NULL
};

enum type_filter {
  tfAll = 0,
  tfAirport,
  tfLandable,
  tfTurnpoint,
  tfFile1,
  tfFile2
};

enum {
  NAMEFILTERLEN = 10,
};

struct WayPointFilterData {
  TCHAR name[NAMEFILTERLEN + 1];

  int distance_index;

  int direction_index;

  type_filter type_index;

  bool defined() const {
    return !string_is_empty(name) || distance_index > 0 ||
      direction_index > 0 || type_index > 0;
  }
};

static WayPointFilterData filter_data;

/**
 * Structure to hold Waypoint sorting information
 */
struct WayPointSelectInfo {
  const Waypoint* way_point; /**< Pointer to actual waypoint (unprotected!) */
  fixed Distance; /**< Distance in user units from observer to waypoint */
  Angle Direction; /**< Bearing (deg true north) from observer to waypoint */
};

struct WaypointSelectInfoVector : public std::vector<WayPointSelectInfo> {
  void push_back(const Waypoint &way_point, const GeoPoint &Location) {
    WayPointSelectInfo info;

    info.way_point = &way_point;

    const GeoVector vec(Location, way_point.Location);

    info.Distance = vec.Distance;
    info.Direction = vec.Bearing;

    std::vector<WayPointSelectInfo>::push_back(info);
  }
};

static WaypointSelectInfoVector WayPointSelectInfo;

static TCHAR *
GetDirectionData(int DirectionFilterIdx)
{
  static TCHAR sTmp[12];

  if (DirectionFilterIdx == 0) {
    _stprintf(sTmp, _T("%c"), '*');
  } else if (DirectionFilterIdx == 1) {
    _stprintf(sTmp, _T("HDG(%u")_T(DEG)_T(")"),
              uround(XCSoarInterface::Basic().Heading.as_bearing().value_degrees()));
  } else
  _stprintf(sTmp, _T("%d")_T(DEG), DirectionFilter[DirectionFilterIdx]);

  return sTmp;
}

static void
InitializeDirection(bool bOnlyHeading)
{
  if (wpDirection) {  // initialize datafieldenum for Direction
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpDirection->GetDataField();
    if (!bOnlyHeading) {
      for (unsigned int i=0; i < sizeof(DirectionFilter) / sizeof(DirectionFilter[0]); i++) {
        dfe->addEnumText(GetDirectionData(i));
      }

      dfe->SetAsInteger(filter_data.direction_index);
    }
    dfe->replaceEnumText(1,GetDirectionData(1)); // update heading value to current heading
    wpDirection->RefreshDisplay();
  }
}

static void
PrepareData(void)
{
  TCHAR sTmp[15];

  filter_data.name[0] = _T('\0');

  wbName->SetCaption(_T("*"));

  if (wpDistance) {  // initialize datafieldenum for Distance
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpDistance->GetDataField();
    dfe->addEnumText(_T("*"));
    for (unsigned i = 1; i < sizeof(DistanceFilter) / sizeof(DistanceFilter[0]); i++) {
      _stprintf(sTmp, _T("%.0f%s"),
                (double)DistanceFilter[i],
                Units::GetDistanceName());
      dfe->addEnumText(sTmp);
    }
    dfe->SetAsInteger(filter_data.distance_index);
    wpDistance->RefreshDisplay();
  }

  InitializeDirection(false);

  if (wpType) {  // initialize datafieldenum for Type
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wpType->GetDataField();
    dfe->addEnumTexts(TypeFilter);
    dfe->SetAsInteger(filter_data.type_index);
    wpType->RefreshDisplay();
  }
}

class FilterWaypointVisitor:
  public WaypointVisitor,
  private WayPointFilterData
{
  const GeoPoint location;
  const Angle heading;
  WaypointSelectInfoVector &vector;

private:
  static bool
  compare_type(const Waypoint &wp, int type_index)
  {
    switch (type_index) {
    case 0:
      return true;

    case 1:
      return wp.is_airport();

    case 2:
      return wp.is_landable();

    case 3:
      return wp.is_turnpoint();

    case 4:
    case 5:
      return wp.FileNum == type_index - 4;
    }

    /* not reachable */
    return false;
  }

  static bool
  compare_direction(const Waypoint &wp, int direction_index,
                    GeoPoint location, Angle heading)
  {
    if (direction_index <= 0)
      return true;

    int a = DirectionFilter[filter_data.direction_index];
    Angle angle = a == DirHDG
      ? last_heading = heading
      : Angle::degrees(fixed(a));

    const GeoVector vec(location, wp.Location);
    fixed DirectionErr = (vec.Bearing - angle).as_delta().magnitude_degrees();

    return DirectionErr < fixed(18);
  }

  static bool
  compare_name(const Waypoint &wp, const TCHAR *name)
  {
    return _tcsnicmp(wp.Name.c_str(), name, _tcslen(name)) == 0;
  }

public:
  FilterWaypointVisitor(const WayPointFilterData &filter,
                        GeoPoint _location, Angle _heading,
                        WaypointSelectInfoVector &_vector)
    :WayPointFilterData(filter), location(_location), heading(_heading),
     vector(_vector) {}

  void Visit(const Waypoint &wp) {
    if (compare_type(wp, type_index) &&
        (filter_data.distance_index == 0 || compare_name(wp, name)) &&
        compare_direction(wp, direction_index, location, heading))
      vector.push_back(wp, location);
  }
};

static bool
WaypointDistanceCompare(const struct WayPointSelectInfo &a,
                        const struct WayPointSelectInfo &b)
{
  return a.Distance < b.Distance;
}

static void
FillList(WaypointSelectInfoVector &dest, const Waypoints &src,
         GeoPoint location, Angle heading, const WayPointFilterData &filter)
{
  dest.clear();

  if (!filter.defined() && src.size() >= 500)
    return;

  FilterWaypointVisitor visitor(filter, location, heading, dest);

  if (filter.distance_index > 0)
    src.visit_within_radius(location, Units::ToSysDistance(
        DistanceFilter[filter.distance_index]), visitor);
  else
    src.visit_name_prefix(filter.name, visitor);

  if (filter.distance_index > 0 || filter.direction_index > 0)
    std::sort(dest.begin(), dest.end(), WaypointDistanceCompare);
}

static void
UpdateList()
{
  FillList(WayPointSelectInfo, way_points, g_location,
           XCSoarInterface::Basic().Heading, filter_data);

  wWayPointList->SetLength(std::max(1, (int)WayPointSelectInfo.size()));
  wWayPointList->SetOrigin(0);
  wWayPointList->SetCursorIndex(0);
  wWayPointList->invalidate();
}

static const TCHAR *
WaypointNameAllowedCharacters(const TCHAR *prefix)
{
  static TCHAR buffer[256];
  return way_points.suggest_name_prefix(prefix, buffer,
                                        sizeof(buffer) / sizeof(buffer[0]));
}

static void
OnFilterNameButton(gcc_unused WndButton &button)
{
  TCHAR newNameFilter[NAMEFILTERLEN + 1];
  _tcsncpy(newNameFilter, filter_data.name, NAMEFILTERLEN);
  dlgTextEntryShowModal(newNameFilter, NAMEFILTERLEN,
                        WaypointNameAllowedCharacters);

  int i = _tcslen(newNameFilter) - 1;
  while (i >= 0) {
    if (newNameFilter[i] != _T(' '))
      break;

    newNameFilter[i] = 0;
    i--;
  };

  _tcsncpy(filter_data.name, newNameFilter, NAMEFILTERLEN);

  if (wbName) {
    if (string_is_empty(filter_data.name))
      wbName->SetCaption(_T("*"));
    else
      wbName->SetCaption(filter_data.name);
  }

  UpdateList();
}

static void
OnFilterDistance(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
  case DataField::daInc:
  case DataField::daDec:
    filter_data.distance_index = Sender->GetAsInteger();
    UpdateList();
    break;
  }
}

static void
OnFilterDirection(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
  case DataField::daInc:
  case DataField::daDec:
    filter_data.direction_index = Sender->GetAsInteger();
    UpdateList();
    break;
  }
}

static void
OnFilterType(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
  case DataField::daInc:
  case DataField::daDec:
    filter_data.type_index = (type_filter)Sender->GetAsInteger();
    UpdateList();
    break;
  }
}

static void
PaintWaypoint(Canvas &canvas, const RECT rc,
              const struct WayPointSelectInfo &info)
{
  const Waypoint &way_point = *info.way_point;

  int w0, w1, w2, w3, x;
  w0 = rc.right - rc.left - Layout::FastScale(4);
  w1 = canvas.text_width(_T("XXX"));
  w2 = canvas.text_width(_T(" 000km"));
  w3 = canvas.text_width(_T(" 000")_T(DEG));

  x = w0 - w1 - w2 - w3;

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2),
                      x - Layout::FastScale(5), way_point.Name.c_str());

  TCHAR buffer[12];
  buffer[0] = '\0';
  buffer[1] = '\0';
  buffer[2] = '\0';

  if (way_point.Flags.Home)
    buffer[0] = 'H';
  else if (way_point.Flags.Airport)
    buffer[0] = 'A';
  else if (way_point.Flags.LandPoint)
    buffer[0] = 'L';

  if (way_point.Flags.TurnPoint) {
    if (buffer[0] == '\0')
      buffer[0] = 'T';
    else
      buffer[1] = 'T';
  }

  // left justified
  canvas.text(rc.left + x, rc.top + Layout::FastScale(2), buffer);

  // right justified after waypoint flags
  Units::FormatUserDistance(info.Distance, buffer,
                            sizeof(buffer) / sizeof(buffer[0]));
  x = w0 - w3 - canvas.text_width(buffer);
  canvas.text(rc.left + x, rc.top + Layout::FastScale(2), buffer);

  // right justified after distance
  _stprintf(buffer, _T("%u")_T(DEG), uround(info.Direction.value_degrees()));
  x = w0 - canvas.text_width(buffer);
  canvas.text(rc.left + x, rc.top + Layout::FastScale(2), buffer);
}

static void
OnPaintListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  if (WayPointSelectInfo.empty()) {
    assert(i == 0);

    canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
                filter_data.defined() || way_points.empty()
                ? _("No Match!")
                : _("Choose a filter or click here"));
    return;
  }

  assert(i < WayPointSelectInfo.size());

  PaintWaypoint(canvas, rc, WayPointSelectInfo[i]);
}

static void
OnWaypointListEnter(unsigned i)
{
  if (WayPointSelectInfo.size() > 0)
    wf->SetModalResult(mrOK);
  else
    OnFilterNameButton(*wbName);
}

static void
OnWPSSelectClicked(gcc_unused WndButton &button)
{
  OnWaypointListEnter(0);
}

static void
OnWPSCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static void
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  if (filter_data.direction_index == 1) {
    Angle a = last_heading - XCSoarInterface::Basic().Heading;
    if (a.as_delta().magnitude_degrees() >= fixed_one) {
      UpdateList();
      InitializeDirection(true);
      wpDirection->RefreshDisplay();
    }
  }
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  type_filter NewIndex = filter_data.type_index;

  switch (key_code) {
  case VK_F1:
    NewIndex = tfAll;
    break;

  case VK_F2:
    NewIndex = tfLandable;
    break;

  case VK_F3:
    NewIndex = tfTurnpoint;
    break;

  default:
    return false;
  }

  if (filter_data.type_index != NewIndex) {
    filter_data.type_index = NewIndex;
    UpdateList();
    wpType->GetDataField()->SetAsInteger(filter_data.type_index);
    wpType->RefreshDisplay();
  }

  return true;
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(NULL)
};

const Waypoint*
dlgWayPointSelect(SingleWindow &parent, const GeoPoint &location)
{
  wf = LoadDialog(CallBackTable, parent, Layout::landscape ?
      _T("IDR_XML_WAYPOINTSELECT_L") : _T("IDR_XML_WAYPOINTSELECT"));

  if (!wf)
    return NULL;

  assert(wf != NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnWPSCloseClicked);

  ((WndButton *)wf->FindByName(_T("cmdSelect")))->
      SetOnClickNotify(OnWPSSelectClicked);

  ((WndButton *)wf->FindByName(_T("cmdFltName")))->
      SetOnClickNotify(OnFilterNameButton);

  wWayPointList = (WndListFrame*)wf->FindByName(_T("frmWayPointList"));
  assert(wWayPointList != NULL);
  wWayPointList->SetActivateCallback(OnWaypointListEnter);
  wWayPointList->SetPaintItemCallback(OnPaintListItem);

  wbName = (WndButton*)wf->FindByName(_T("cmdFltName"));
  wpDistance = (WndProperty*)wf->FindByName(_T("prpFltDistance"));
  wpDirection = (WndProperty*)wf->FindByName(_T("prpFltDirection"));
  wpType = (WndProperty *)wf->FindByName(_T("prpFltType"));

  g_location = location;
  PrepareData();
  UpdateList();

  wf->SetTimerNotify(OnTimerNotify);

  if (wf->ShowModal() != mrOK) {
    delete wf;
    return NULL;
  }

  unsigned ItemIndex = wWayPointList->GetCursorIndex();
  if (ItemIndex >= WayPointSelectInfo.size())
    return NULL;

  delete wf;
  return WayPointSelectInfo[ItemIndex].way_point;
}
