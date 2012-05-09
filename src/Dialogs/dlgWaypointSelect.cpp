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
#include "Compatibility/path.h"
#include "Waypoint/LastUsed.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointVisitor.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "Form/DataField/Enum.hpp"
#include "LogFile.hpp"
#include "Util/StringUtil.hpp"
#include "Engine/Task/Tasks/BaseTask/OrderedTaskPoint.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/FAITriangleValidator.hpp"
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
static FAITrianglePointValidator *triangle_validator = NULL;
static WndForm *dialog = NULL;
static ListControl *waypoint_list_control = NULL;
static WndButton *name_button;
static WndProperty *distance_filter;
static WndProperty *direction_filter;
static WndProperty *type_filter;

static const fixed distance_filter_items[] = {
  fixed_zero, fixed(25.0), fixed(50.0),
  fixed(75.0), fixed(100.0), fixed(150.0),
  fixed(250.0), fixed(500.0), fixed(1000.0),
};

#define HEADING_DIRECTION -1
static int direction_filter_items[] = {
  0, HEADING_DIRECTION, 360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
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

enum TypeFilter {
  TF_ALL = 0,
  TF_AIRPORT,
  TF_LANDABLE,
  TF_TURNPOINT,
  TF_START,
  TF_FINISH,
  TF_FAI_TRIANGLE_LEFT,
  TF_FAI_TRIANGLE_RIGHT,
  TF_FILE_1,
  TF_FILE_2,
  TF_LAST_USED,
};

enum {
  NAME_FILTER_LENGTH = 10,
};

struct WaypointFilterData
{
  TCHAR name[NAME_FILTER_LENGTH + 1];

  int distance_index;
  int direction_index;
  TypeFilter type_index;

  bool IsDefined() const {
    return !StringIsEmpty(name) || distance_index > 0 ||
      direction_index > 0 || type_index != TF_ALL;
  }
};

static WaypointFilterData filter_data;
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
  if (direction_filter) {
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
}

/**
 * Gets a path from the profile and return its base name only.
 */
gcc_pure
static const TCHAR *
GetProfilePathBase(const TCHAR *key)
{
  const TCHAR *p = Profile::Get(key);
  if (p == NULL)
    return NULL;

  if (DIR_SEPARATOR != '\\') {
    const TCHAR *backslash = _tcsrchr(p, '\\');
    if (backslash != NULL)
      p = backslash + 1;
  }

  return BaseName(p);
}

static void
PrepareData()
{
  filter_data.name[0] = _T('\0');

  name_button->SetCaption(_T("*"));

  // initialize datafieldenum for Distance
  if (distance_filter) {
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
  }

  InitializeDirection(false);

  // initialize datafieldenum for Type
  if (type_filter) {
    DataFieldEnum* data_field = (DataFieldEnum*)type_filter->GetDataField();
    data_field->addEnumTexts(type_filter_items);

    const TCHAR *p = GetProfilePathBase(szProfileWaypointFile);
    if (p != NULL)
      data_field->replaceEnumText(TF_FILE_1, p);

    p = GetProfilePathBase(szProfileAdditionalWaypointFile);
    if (p != NULL)
      data_field->replaceEnumText(TF_FILE_2, p);

    data_field->SetAsInteger(filter_data.type_index);
    type_filter->RefreshDisplay();
  }
}

/**
 * Helper class to filter waypoints in list based on whether the filter is set
 * to Right or Left FAI Triangle.
 */
class FAITrianglePointValidator
{
  OrderedTask *task;
  unsigned t_index;
  unsigned t_size;
  fixed leg1;
  fixed leg2;
  fixed leg3;

  /** min distance for any FAI Leg -- derived from circular FAI sector radius */
  const fixed min_fai_leg;
  /** min angle allowable in a FAI Triangle 31.5 degrees */
  const fixed min_fai_angle;
  /** max angle allowable in a FAI Triangle 113.2 degrees */
  const fixed max_fai_angle;
  bool fai_triangle_point_invalid;

public:
  FAITrianglePointValidator(OrderedTask *ordered_task,
                            const unsigned ordered_task_index) :
                            task(ordered_task),
                            t_index(ordered_task_index),
                            t_size(0),
                            leg1(fixed_zero),
                            leg2(fixed_zero),
                            leg3(fixed_zero),
                            min_fai_leg(fixed(2000)),
                            min_fai_angle(fixed(31.5)),
                            max_fai_angle(fixed(114)),
                            fai_triangle_point_invalid(false)
  {
    PrepareFAITest(ordered_task, ordered_task_index);
  }


  bool
  TestFAITriangle(const fixed d1, const fixed d2, const fixed d3)
  {
    if ((d1 < min_fai_leg) || (d2 < min_fai_leg) || (d3 < min_fai_leg))
      return false;

    return FAITriangleValidator::TestDistances(d1, d2, d3);
  }

  /**
   * Perform fast check to exclude point as from further consideration
   * based on min/max possible values for any FAI triangle
   * @param p0 point 1 of angle
   * @param p1 point 2 of angle
   * @param p2 point 3 of angle
   * @param right.  = 1 if angle is for right triangle, -1 if left triangle.
   * @returns False if angle from three points is out of possible range for
   * an FAI triangle.
   */
  bool
  IsFAIAngle(const GeoPoint &p0, const GeoPoint &p1, const GeoPoint &p2,
             const fixed right)
  {
    const Angle a01 = p0.Bearing(p1);
    const Angle a21 = p2.Bearing(p1);
    const fixed diff = (a01 - a21).AsDelta().Degrees();

    if (positive(right))
      return (diff > min_fai_angle) && (diff < max_fai_angle);
    else
      return (diff < fixed(-1) * min_fai_angle) && (diff > fixed(-1)
              * max_fai_angle);
  }

  /** Test whether wp could be a point in an FAI triangle based on the other
   * points in the task and the current ordered task index
   * Tests angle ranges first to reduce more costly calculation of distances
   * @param wp Point being tested
   * @param right = 1 if triangle turns are to right, -1 if turns are to left
   * @return True if point would be valid in an FAI Triangle
   */
  bool
  IsFAITrianglePoint(const Waypoint& wp, const fixed right)
  {
    if (fai_triangle_point_invalid)
      return false;

    if (!task)
      return true;

    if (t_size == 0)
      return true;

    const GeoPoint &p = wp.location;
    // replace start
    if (t_index == 0) {
      assert(t_size <= 4 && t_size > 0);

      switch (t_size) {
      case 1:
        return true;

      case 2:
        return p.Distance(task->get_tp(1)->GetLocation()) > min_fai_leg;

      default: // size == 3 or 4
        if (!IsFAIAngle(p, task->get_tp(1)->GetLocation(),
                        task->get_tp(2)->GetLocation(), right))
          return false;
        if (t_size == 3) {
          return TestFAITriangle(p.Distance(task->get_tp(1)->GetLocation()),
                                 leg2,
                                 task->get_tp(2)->GetLocation().Distance(p));
        } else if (t_size == 4) {
          return (wp == task->get_tp(3)->GetWaypoint()) &&
                 TestFAITriangle(p.Distance(task->get_tp(1)->GetLocation()),
                                 leg2,
                                 leg3);
        }
      }
    }
    // append or replace point #1
    if (t_index == 1) {
      assert(t_size > 0);

      if (t_size <= 2)
        return p.Distance(task->get_tp(0)->GetLocation()) > min_fai_leg;

      // size == 3 or 4
      if (!IsFAIAngle(task->get_tp(0)->GetLocation(),
                      p,
                      task->get_tp(2)->GetLocation(), right))
        return false;

      if (t_size == 3) {
        return TestFAITriangle(p.Distance(task->get_tp(0)->GetLocation()),
                               p.Distance(task->get_tp(2)->GetLocation()),
                               task->get_tp(2)->GetLocation().
                                  Distance(task->get_tp(0)->GetLocation()));
      } else if (t_size == 4) {
        return TestFAITriangle(p.Distance(task->get_tp(0)->GetLocation()),
                               p.Distance(task->get_tp(2)->GetLocation()),
                               leg3);
      }
    }
    // append or replace point #2
    if (t_index == 2) {
      assert(t_size >= 2);
      if (!IsFAIAngle(task->get_tp(0)->GetLocation(),
                      task->get_tp(1)->GetLocation(),
                      p, right))
        return false;

      if (t_size < 4) { // no finish point yet
        return TestFAITriangle(leg1,
                               p.Distance(task->get_tp(1)->GetLocation()),
                               p.Distance(task->get_tp(0)->GetLocation()));

      } else { // already finish point(#3) exists
        return (task->get_tp(0)->GetWaypoint() == task->get_tp(3)->GetWaypoint()) &&
                TestFAITriangle(leg1,
                                p.Distance(task->get_tp(1)->GetLocation()),
                                p.Distance(task->get_tp(0)->GetLocation()));
      }
    }
    // append or replace finish
    if (t_index == 3) {
      assert (t_size == 3 || t_size == 4);
      return (wp == task->get_tp(0)->GetWaypoint()) &&
              TestFAITriangle(leg1,
                              leg2,
                              p.Distance(task->get_tp(2)->GetLocation()));
    }
    return true;
  }

private:

  void
  PrepareFAITest(OrderedTask *ordered_task, const unsigned ordered_task_index)
  {
    task = ordered_task;
    t_index = ordered_task_index;

    fai_triangle_point_invalid = false;

    if (ordered_task) {
      t_size = task->TaskSize();
      leg1 = t_size > 1
        ? task->GetTaskPoint(1).GetVectorPlanned().distance : fixed_zero;
      leg2 = t_size > 2
        ? task->GetTaskPoint(2).GetVectorPlanned().distance : fixed_zero;
      leg3 = t_size > 3
        ? task->GetTaskPoint(3).GetVectorPlanned().distance : fixed_zero;
    } else {
      leg1 = leg2 = leg3 = fixed_zero;
      t_size = 0;
      t_index = 0;
    }

    if (t_size > 4)
      fai_triangle_point_invalid = true;

    if (t_index > 3)
      fai_triangle_point_invalid = true;
  }
};

class FilterWaypointVisitor:
  public WaypointVisitor,
  private WaypointFilterData
{
  const GeoPoint location;
  const Angle heading;
  WaypointList &waypoint_list;

private:
  static bool
  CompareType(const Waypoint &waypoint, TypeFilter type)
  {
    switch (type) {
    case TF_ALL:
      return true;

    case TF_AIRPORT:
      return waypoint.IsAirport();

    case TF_LANDABLE:
      return waypoint.IsLandable();

    case TF_TURNPOINT:
      return waypoint.IsTurnpoint();

    case TF_START:
      return waypoint.IsStartpoint();

    case TF_FINISH:
      return waypoint.IsFinishpoint();

    case TF_FAI_TRIANGLE_LEFT:
      return triangle_validator->IsFAITrianglePoint(waypoint, fixed(-1));

    case TF_FAI_TRIANGLE_RIGHT:
      return triangle_validator->IsFAITrianglePoint(waypoint, fixed_one);

    case TF_FILE_1:
      return waypoint.file_num == 1;

    case TF_FILE_2:
      return waypoint.file_num == 2;

    case TF_LAST_USED:
      return false;
    }

    /* not reachable */
    return false;
  }

  static bool
  CompareDirection(const Waypoint &waypoint, int direction_index,
                    GeoPoint location, Angle heading)
  {
    if (direction_index <= 0)
      return true;

    int a = direction_filter_items[filter_data.direction_index];
    Angle angle = (a == HEADING_DIRECTION) ? heading : Angle::Degrees(fixed(a));

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
  FilterWaypointVisitor(const WaypointFilterData &filter,
                        GeoPoint _location, Angle _heading,
                        WaypointList &_waypoint_list)
    :WaypointFilterData(filter), location(_location), heading(_heading),
     waypoint_list(_waypoint_list) {}

  void Visit(const Waypoint &waypoint) {
    if (CompareType(waypoint, type_index) &&
        (filter_data.distance_index == 0 || CompareName(waypoint, name)) &&
        CompareDirection(waypoint, direction_index, location, heading))
      waypoint_list.push_back(WaypointListItem(waypoint));
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
         GeoPoint location, Angle heading, const WaypointFilterData &filter)
{
  list.clear();

  if (!filter.IsDefined() && src.size() >= 500)
    return;

  FilterWaypointVisitor visitor(filter, location, heading, list);

  if (filter.distance_index > 0)
    src.VisitWithinRange(location, Units::ToSysDistance(
        distance_filter_items[filter.distance_index]), visitor);
  else
    src.VisitNamePrefix(filter.name, visitor);

  if (filter.distance_index > 0 || filter.direction_index > 0)
    std::sort(list.begin(), list.end(), WaypointDistanceCompare(location));
}

static void
FillLastUsedList(WaypointList &list,
                 const WaypointIDList &last_used_ids,
                 const Waypoints &waypoints)
{
  list.clear();

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
  if (filter_data.type_index == TF_LAST_USED)
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
  TCHAR new_name_filter[NAME_FILTER_LENGTH + 1];
  CopyString(new_name_filter, filter_data.name, NAME_FILTER_LENGTH + 1);
  dlgTextEntryShowModal(*(SingleWindow *)button.GetRootOwner(),
                        new_name_filter, NAME_FILTER_LENGTH, _("Waypoint name"),
                        WaypointNameAllowedCharacters);

  int i = _tcslen(new_name_filter) - 1;
  while (i >= 0) {
    if (new_name_filter[i] != _T(' '))
      break;

    new_name_filter[i] = 0;
    i--;
  }

  CopyString(filter_data.name, new_name_filter, NAME_FILTER_LENGTH + 1);

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
      direction_filter->RefreshDisplay();
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
    new_index = TF_ALL;
    break;

  case VK_APP2:
    new_index = TF_LANDABLE;
    break;

  case VK_APP3:
    new_index = TF_TURNPOINT;
    break;

  default:
    return false;
  }

  if (filter_data.type_index != new_index) {
    filter_data.type_index = new_index;
    UpdateList();
    type_filter->GetDataField()->SetAsInteger(filter_data.type_index);
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
                  OrderedTask *ordered_task,
                  const unsigned ordered_task_index)
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
  direction_filter = (WndProperty*)dialog->FindByName(_T("prpFltDirection"));
  type_filter = (WndProperty *)dialog->FindByName(_T("prpFltType"));

  location = _location;
  triangle_validator =
      new FAITrianglePointValidator(ordered_task, ordered_task_index);
  last_heading = CommonInterface::Calculated().heading;

  PrepareData();
  UpdateList();

  dialog->SetTimerNotify(OnTimerNotify);

  if (dialog->ShowModal() != mrOK) {
    delete dialog;
    delete triangle_validator;
    return NULL;
  }

  unsigned index = waypoint_list_control->GetCursorIndex();

  delete dialog;
  delete triangle_validator;

  const Waypoint* retval = NULL;

  if (index < waypoint_list.size())
    retval = waypoint_list[index].waypoint;

  return retval;
}
