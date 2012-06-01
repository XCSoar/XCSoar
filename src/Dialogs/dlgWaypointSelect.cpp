/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Math/Earth.hpp"
#include "Screen/Layout.hpp"
#include "Compatibility/string.h"
#include "Math/FastMath.h"
#include "Form/DataField/Base.hpp"
#include "Profile/Profile.hpp"
#include "OS/PathName.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointVisitor.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "Form/DataField/Enum.hpp"
#include "LogFile.hpp"
#include "Util/StringUtil.hpp"
#include "Task/FAITrianglePointValidator.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Util/Macros.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"

#include <algorithm>
#include <list>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

class FAITrianglePointValidator;

static GeoPoint location;
static WndForm *dialog = NULL;
static ListControl *waypoint_list_control = NULL;
static WndButton *name_button;
static WndProperty *distance_filter;
static WndProperty *direction_filter;
static WndProperty *type_filter;

static OrderedTask *ordered_task;
static unsigned ordered_task_index;

static const fixed distance_filter_items[] = {
  fixed_zero, fixed(25.0), fixed(50.0),
  fixed(75.0), fixed(100.0), fixed(150.0),
  fixed(250.0), fixed(500.0), fixed(1000.0),
};

static int direction_filter_items[] = {
  -1, -1, 0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
};

static Angle last_heading = Angle::Zero();

/**
 * used for single-letter name search with Left/Right keys
 */
static int name_filter_index = -1;

static const TCHAR *const type_filter_items[] = {
  _T("*"), _T("Airport"), _T("Landable"),
  _T("Turnpoint"), 
  _T("Start"), 
  _T("Finish"), 
  _T("Left FAI Triangle"),
  _T("Right FAI Triangle"),
  _T("File 1"), _T("File 2"),
  _T("Recently Used"),
  NULL
};

enum class TypeFilter: uint8_t {
  ALL = 0,
  AIRPORT,
  LANDABLE,
  TURNPOINT,
  START,
  FINISH,
  FAI_TRIANGLE_LEFT,
  FAI_TRIANGLE_RIGHT,
  FILE_1,
  FILE_2,
  LAST_USED,
};

struct WaypointListFilter
{
  enum {
    NAME_LENGTH = 10,
  };

  TCHAR name[NAME_LENGTH + 1];

  fixed distance;
  Angle direction;
  TypeFilter type_index;
};

struct WaypointListDialogState
{
  TCHAR name[WaypointListFilter::NAME_LENGTH + 1];

  int distance_index;
  int direction_index;
  TypeFilter type_index;

  bool IsDefined() const {
    return !StringIsEmpty(name) || distance_index > 0 ||
      direction_index > 0 || type_index != TypeFilter::ALL;
  }

  void ToFilter(WaypointListFilter &filter, Angle heading) const {
    _tcscpy(filter.name, name);
    filter.distance = Units::ToSysDistance(distance_filter_items[distance_index]);
    filter.type_index = type_index;

    if (direction_index != 1)
      filter.direction = Angle::Degrees(
          fixed(direction_filter_items[direction_index]));
    else
      filter.direction = heading;
  }
};

static WaypointListDialogState filter_data;
static WaypointList waypoint_list;

static TCHAR *
GetDirectionData(TCHAR *buffer, size_t size, int direction_filter_index)
{
  if (direction_filter_index == 0)
    _tcscpy(buffer, _T("*"));
  else if (direction_filter_index == 1) {
    TCHAR bearing[8];
    FormatBearing(bearing, ARRAY_SIZE(bearing), last_heading);
    _stprintf(buffer, _T("HDG(%s)"), bearing);
  } else
    FormatBearing(buffer, size, direction_filter_items[direction_filter_index]);

  return buffer;
}

static void
InitializeDirection(bool only_heading)
{
  // initialize datafieldenum for Direction
  TCHAR buffer[12];

  DataFieldEnum* data_field = (DataFieldEnum*)direction_filter->GetDataField();
  if (!only_heading) {
    for (unsigned int i = 0; i < ARRAY_SIZE(direction_filter_items); i++)
      data_field->addEnumText(GetDirectionData(buffer, ARRAY_SIZE(buffer), i));

    data_field->SetAsInteger(filter_data.direction_index);
  }
  // update heading value to current heading
  data_field->replaceEnumText(1,GetDirectionData(buffer, ARRAY_SIZE(buffer), 1));
  direction_filter->RefreshDisplay();
}

static void
PrepareData()
{
  filter_data.name[0] = _T('\0');

  name_button->SetCaption(_T("*"));

  // initialize datafieldenum for Distance
  DataFieldEnum* data_field = (DataFieldEnum*)distance_filter->GetDataField();
  data_field->addEnumText(_T("*"));

  TCHAR buffer[15];
  for (unsigned i = 1; i < ARRAY_SIZE(distance_filter_items); i++) {
    _stprintf(buffer, _T("%.0f%s"), (double)distance_filter_items[i],
              Units::GetDistanceName());
    data_field->addEnumText(buffer);
  }

  data_field->SetAsInteger(filter_data.distance_index);
  distance_filter->RefreshDisplay();

  InitializeDirection(false);

  // initialize datafieldenum for Type
  data_field = (DataFieldEnum*)type_filter->GetDataField();
  data_field->addEnumTexts(type_filter_items);

  const TCHAR *p = Profile::GetPathBase(szProfileWaypointFile);
  if (p != NULL)
    data_field->replaceEnumText((unsigned)TypeFilter::FILE_1, p);

  p = Profile::GetPathBase(szProfileAdditionalWaypointFile);
  if (p != NULL)
    data_field->replaceEnumText((unsigned)TypeFilter::FILE_2, p);

  data_field->SetAsInteger((int)filter_data.type_index);
  type_filter->RefreshDisplay();
}

class WaypointListBuilder:
  public WaypointVisitor,
  private WaypointListFilter
{
  const GeoPoint location;
  WaypointList &waypoint_list;
  FAITrianglePointValidator triangle_validator;

private:
  static bool
  CompareType(const Waypoint &waypoint, TypeFilter type,
              const FAITrianglePointValidator &triangle_validator)
  {
    switch (type) {
    case TypeFilter::ALL:
      return true;

    case TypeFilter::AIRPORT:
      return waypoint.IsAirport();

    case TypeFilter::LANDABLE:
      return waypoint.IsLandable();

    case TypeFilter::TURNPOINT:
      return waypoint.IsTurnpoint();

    case TypeFilter::START:
      return waypoint.IsStartpoint();

    case TypeFilter::FINISH:
      return waypoint.IsFinishpoint();

    case TypeFilter::FAI_TRIANGLE_LEFT:
      return triangle_validator.IsFAITrianglePoint(waypoint, false);

    case TypeFilter::FAI_TRIANGLE_RIGHT:
      return triangle_validator.IsFAITrianglePoint(waypoint, true);

    case TypeFilter::FILE_1:
      return waypoint.file_num == 1;

    case TypeFilter::FILE_2:
      return waypoint.file_num == 2;

    case TypeFilter::LAST_USED:
      return false;
    }

    /* not reachable */
    return false;
  }

  static bool
  CompareDirection(const Waypoint &waypoint, Angle angle, GeoPoint location)
  {
    if (negative(angle.Native()))
      return true;

    const GeoVector vec(location, waypoint.location);
    fixed direction_error = (vec.bearing - angle).AsDelta().AbsoluteDegrees();

    return direction_error < fixed(18);
  }

  static bool
  CompareName(const Waypoint &waypoint, const TCHAR *name)
  {
    return _tcsnicmp(waypoint.name.c_str(), name, _tcslen(name)) == 0;
  }

public:
  WaypointListBuilder(const WaypointListFilter &filter,
                        GeoPoint _location, WaypointList &_waypoint_list,
                        OrderedTask *ordered_task, unsigned ordered_task_index)
    :WaypointListFilter(filter), location(_location),
     waypoint_list(_waypoint_list),
     triangle_validator(ordered_task, ordered_task_index) {}

  void Add(const Waypoint &waypoint) {
    waypoint_list.push_back(WaypointListItem(waypoint));
  }

  void AddFiltered(const Waypoint &waypoint) {
    if (CompareType(waypoint, type_index, triangle_validator) &&
        (!positive(distance) || CompareName(waypoint, name)) &&
        CompareDirection(waypoint, direction, location))
      Add(waypoint);
  }

  void AddFiltered(const Waypoints &waypoints) {
    if (positive(distance))
      waypoints.VisitWithinRange(location, distance, *this);
    else
      waypoints.VisitNamePrefix(name, *this);
  }

  void Visit(const Waypoint &waypoint) {
    AddFiltered(waypoint);
  }
};

class WaypointDistanceCompare
{
  const GeoPoint &location;

public:
  WaypointDistanceCompare(const GeoPoint &_location):location(_location) {}

  bool operator()(const WaypointListItem &a,
                  const WaypointListItem &b) const {
    return a.GetVector(location).distance < b.GetVector(location).distance;
  }
};

static void
FillList(WaypointList &list, const Waypoints &src,
         GeoPoint location, Angle heading, const WaypointListDialogState &state)
{
  if (!state.IsDefined() && src.size() >= 500)
    return;

  WaypointListFilter filter;
  state.ToFilter(filter, heading);

  WaypointListBuilder builder(filter, location, list,
                              ordered_task, ordered_task_index);
  builder.AddFiltered(src);

  if (positive(filter.distance) || !negative(filter.direction.Native()))
    std::sort(list.begin(), list.end(), WaypointDistanceCompare(location));
}

static void
FillLastUsedList(WaypointList &list,
                 const WaypointIDList &last_used_ids,
                 const Waypoints &waypoints)
{
  for (auto it = last_used_ids.rbegin(); it != last_used_ids.rend(); it++) {
    const Waypoint* waypoint = waypoints.LookupId(*it);
    if (waypoint == NULL)
      continue;

    list.push_back(WaypointListItem(*waypoint));
  }
}

static void
UpdateList()
{
  waypoint_list.clear();

  if (filter_data.type_index == TypeFilter::LAST_USED)
    FillLastUsedList(waypoint_list, LastUsedWaypoints::GetList(),
                     way_points);
  else
    FillList(waypoint_list, way_points, location, last_heading,
             filter_data);

  waypoint_list_control->SetLength(std::max(1, (int)waypoint_list.size()));
  waypoint_list_control->SetOrigin(0);
  waypoint_list_control->SetCursorIndex(0);
  waypoint_list_control->Invalidate();
}

static const TCHAR *
WaypointNameAllowedCharacters(const TCHAR *prefix)
{
  static TCHAR buffer[256];
  return way_points.SuggestNamePrefix(prefix, buffer, ARRAY_SIZE(buffer));
}

static void
NameButtonUpdateChar()
{
  const TCHAR *name_filter = WaypointNameAllowedCharacters(_T(""));
  if (name_filter_index == -1) {
    filter_data.name[0] = '\0';
    name_button->SetCaption(_T("*"));
  } else {
    filter_data.name[0] = name_filter[name_filter_index];
    filter_data.name[1] = '\0';
    name_button->SetCaption(filter_data.name);
  }

  UpdateList();
}

static void
OnFilterNameButtonRight(gcc_unused WndButton &button)
{
  const TCHAR * name_filter = WaypointNameAllowedCharacters(_T(""));
  name_filter_index++;
  if (name_filter_index > (int)(_tcslen(name_filter) - 2))
    name_filter_index = -1;

  NameButtonUpdateChar();
}

static void
OnFilterNameButtonLeft(gcc_unused WndButton &button)
{
  const TCHAR * name_filter = WaypointNameAllowedCharacters(_T(""));
  if (name_filter_index == -1)
    name_filter_index = (int)(_tcslen(name_filter)-1);
  else
    name_filter_index--;

  NameButtonUpdateChar();
}

static void
OnFilterNameButton(gcc_unused WndButton &button)
{
  TCHAR new_name_filter[WaypointListFilter::NAME_LENGTH + 1];
  CopyString(new_name_filter, filter_data.name,
             WaypointListFilter::NAME_LENGTH + 1);

  dlgTextEntryShowModal(*(SingleWindow *)button.GetRootOwner(), new_name_filter,
                        WaypointListFilter::NAME_LENGTH, _("Waypoint name"),
                        WaypointNameAllowedCharacters);

  int i = _tcslen(new_name_filter) - 1;
  while (i >= 0) {
    if (new_name_filter[i] != _T(' '))
      break;

    new_name_filter[i] = 0;
    i--;
  }

  CopyString(filter_data.name, new_name_filter,
             WaypointListFilter::NAME_LENGTH + 1);

  if (name_button) {
    if (StringIsEmpty(filter_data.name))
      name_button->SetCaption(_T("*"));
    else
      name_button->SetCaption(filter_data.name);
  }

  UpdateList();
}

static void
OnFilterDistance(DataField *sender, DataField::DataAccessMode mode)
{
  switch (mode) {
  case DataField::daChange:
    filter_data.distance_index = sender->GetAsInteger();
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnFilterDirection(DataField *sender, DataField::DataAccessMode mode)
{
  switch (mode) {
  case DataField::daChange:
    filter_data.direction_index = sender->GetAsInteger();
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnFilterType(DataField *sender, DataField::DataAccessMode mode)
{
  switch (mode) {
  case DataField::daChange:
    filter_data.type_index = (TypeFilter)sender->GetAsInteger();
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  if (waypoint_list.empty()) {
    assert(i == 0);

    const UPixelScalar line_height = rc.bottom - rc.top;
    const Font &name_font =
      *UIGlobals::GetDialogLook().list.font;
    canvas.SetTextColor(COLOR_BLACK);
    canvas.Select(name_font);
    canvas.text(rc.left + line_height + Layout::FastScale(2),
                rc.top + line_height / 2 - name_font.GetHeight() / 2,
                filter_data.IsDefined() || way_points.IsEmpty() ?
                _("No Match!") : _("Choose a filter or click here"));
    return;
  }

  assert(i < waypoint_list.size());

  const struct WaypointListItem &info = waypoint_list[i];

  WaypointListRenderer::Draw(canvas, rc, *info.waypoint,
                             info.GetVector(location),
                             UIGlobals::GetDialogLook(),
                             UIGlobals::GetMapLook().waypoint,
                             CommonInterface::GetMapSettings().waypoint);
}

static void
OnWaypointListEnter(gcc_unused unsigned i)
{
  if (waypoint_list.size() > 0)
    dialog->SetModalResult(mrOK);
  else
    OnFilterNameButton(*name_button);
}

static void
OnSelectClicked(gcc_unused WndButton &button)
{
  OnWaypointListEnter(0);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

static void
OnTimerNotify(gcc_unused WndForm &sender)
{
  if (filter_data.direction_index == 1 && !XCSoarInterface::Calculated().circling) {
    Angle a = last_heading - CommonInterface::Calculated().heading;
    if (a.AsDelta().AbsoluteDegrees() >= fixed(60)) {
      last_heading = CommonInterface::Calculated().heading;
      UpdateList();
      InitializeDirection(true);
    }
  }
}

#ifdef GNAV

static bool
FormKeyDown(WndForm &sender, unsigned key_code)
{
  TypeFilter new_index = filter_data.type_index;

  switch (key_code) {
  case VK_APP1:
    new_index = TypeFilter::ALL;
    break;

  case VK_APP2:
    new_index = TypeFilter::LANDABLE;
    break;

  case VK_APP3:
    new_index = TypeFilter::TURNPOINT;
    break;

  default:
    return false;
  }

  if (filter_data.type_index != new_index) {
    filter_data.type_index = new_index;
    UpdateList();
    type_filter->GetDataField()->SetAsInteger((int)filter_data.type_index);
    type_filter->RefreshDisplay();
  }

  return true;
}

#endif /* GNAV */

static gcc_constexpr_data CallBackTableEntry callback_table[] = {
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(OnFilterNameButton),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSelectClicked),
  DeclareCallBackEntry(NULL)
};

const Waypoint*
dlgWaypointSelect(SingleWindow &parent, const GeoPoint &_location,
                  OrderedTask *_ordered_task,
                  const unsigned _ordered_task_index)
{
  dialog = LoadDialog(callback_table, parent, Layout::landscape ?
      _T("IDR_XML_WAYPOINTSELECT_L") : _T("IDR_XML_WAYPOINTSELECT"));
  assert(dialog != NULL);

#ifdef GNAV
  dialog->SetKeyDownNotify(FormKeyDown);
#endif

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  waypoint_list_control = (ListControl*)dialog->FindByName(_T("frmWaypointList"));
  assert(waypoint_list_control != NULL);
  waypoint_list_control->SetActivateCallback(OnWaypointListEnter);
  waypoint_list_control->SetPaintItemCallback(OnPaintListItem);
  waypoint_list_control->SetItemHeight(WaypointListRenderer::GetHeight(dialog_look));

  name_button = (WndButton*)dialog->FindByName(_T("cmdFltName"));
  name_button->SetOnLeftNotify(OnFilterNameButtonLeft);
  name_button->SetOnRightNotify(OnFilterNameButtonRight);

  distance_filter = (WndProperty*)dialog->FindByName(_T("prpFltDistance"));
  assert(distance_filter != NULL);
  direction_filter = (WndProperty*)dialog->FindByName(_T("prpFltDirection"));
  assert(direction_filter != NULL);
  type_filter = (WndProperty *)dialog->FindByName(_T("prpFltType"));
  assert(type_filter != NULL);

  location = _location;
  ordered_task = _ordered_task;
  ordered_task_index = _ordered_task_index;
  last_heading = CommonInterface::Calculated().heading;

  PrepareData();
  UpdateList();

  dialog->SetTimerNotify(OnTimerNotify);

  if (dialog->ShowModal() != mrOK) {
    delete dialog;
    return NULL;
  }

  unsigned index = waypoint_list_control->GetCursorIndex();

  delete dialog;

  const Waypoint* retval = NULL;

  if (index < waypoint_list.size())
    retval = waypoint_list[index].waypoint;

  return retval;
}
