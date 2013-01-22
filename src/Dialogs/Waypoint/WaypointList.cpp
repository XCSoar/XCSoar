/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WaypointDialogs.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Math/FastMath.h"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/List.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Base.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Profile/Profile.hpp"
#include "OS/PathName.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/WaypointListBuilder.hpp"
#include "Waypoint/WaypointFilter.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "Form/DataField/Enum.hpp"
#include "LogFile.hpp"
#include "Util/StringUtil.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Util/Macros.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Blackboard/ScopeGPSListener.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <list>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

class FAITrianglePointValidator;

static GeoPoint location;
static WndForm *dialog = NULL;
static ListControl *waypoint_list_control = NULL;
static WndProperty *name_control;
static WndProperty *distance_filter;
static WndProperty *direction_filter;
static WndProperty *type_filter;

static OrderedTask *ordered_task;
static unsigned ordered_task_index;

static constexpr unsigned distance_filter_items[] = {
  0, 25, 50, 75, 100, 150, 250, 500, 1000
};

static constexpr int direction_filter_items[] = {
  -1, -1, 0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
};

static Angle last_heading = Angle::Zero();

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

struct WaypointListDialogState
{
  StaticString<WaypointFilter::NAME_LENGTH + 1> name;

  int distance_index;
  int direction_index;
  TypeFilter type_index;

  bool IsDefined() const {
    return !name.empty() || distance_index > 0 ||
      direction_index > 0 || type_index != TypeFilter::ALL;
  }

  void ToFilter(WaypointFilter &filter, Angle heading) const {
    filter.name = name;
    filter.distance =
      Units::ToSysDistance(fixed(distance_filter_items[distance_index]));
    filter.type_index = type_index;

    if (direction_index != 1)
      filter.direction = Angle::Degrees(
          fixed(direction_filter_items[direction_index]));
    else
      filter.direction = heading;
  }
};

class WaypointListDialog : public ListItemRenderer,
                           public ListCursorHandler {
public:
  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;
};

static WaypointListDialogState dialog_state;
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

    data_field->SetAsInteger(dialog_state.direction_index);
  } else
    // update heading value to current heading
    data_field->replaceEnumText(1, GetDirectionData(buffer, ARRAY_SIZE(buffer), 1));

  direction_filter->RefreshDisplay();
}

static void
PrepareData()
{
  dialog_state.name.clear();

  // initialize datafieldenum for Distance
  DataFieldEnum* data_field = (DataFieldEnum*)distance_filter->GetDataField();
  data_field->addEnumText(_T("*"));

  TCHAR buffer[15];
  for (unsigned i = 1; i < ARRAY_SIZE(distance_filter_items); i++) {
    FormatUserDistance(Units::ToSysDistance(fixed(distance_filter_items[i])),
                       buffer);
    data_field->addEnumText(buffer);
  }

  data_field->SetAsInteger(dialog_state.distance_index);
  distance_filter->RefreshDisplay();

  InitializeDirection(false);

  // initialize datafieldenum for Type
  data_field = (DataFieldEnum*)type_filter->GetDataField();
  data_field->addEnumTexts(type_filter_items);

  const TCHAR *p = Profile::GetPathBase(ProfileKeys::WaypointFile);
  if (p != NULL)
    data_field->replaceEnumText((unsigned)TypeFilter::FILE_1, p);

  p = Profile::GetPathBase(ProfileKeys::AdditionalWaypointFile);
  if (p != NULL)
    data_field->replaceEnumText((unsigned)TypeFilter::FILE_2, p);

  data_field->SetAsInteger((int)dialog_state.type_index);
  type_filter->RefreshDisplay();
}

static void
FillList(WaypointList &list, const Waypoints &src,
         GeoPoint location, Angle heading, const WaypointListDialogState &state)
{
  if (!state.IsDefined() && src.size() >= 500)
    return;

  WaypointFilter filter;
  state.ToFilter(filter, heading);

  WaypointListBuilder builder(filter, location, list,
                              ordered_task, ordered_task_index);
  builder.Visit(src);

  if (positive(filter.distance) || !negative(filter.direction.Native()))
    list.SortByDistance(location);
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

  if (dialog_state.type_index == TypeFilter::LAST_USED)
    FillLastUsedList(waypoint_list, LastUsedWaypoints::GetList(),
                     way_points);
  else
    FillList(waypoint_list, way_points, location, last_heading,
             dialog_state);

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

class FilterDataFieldListener: public DataFieldListener
{
private:
  /* virtual methods from DataFieldListener */
  virtual void OnModified(DataField &df);
};

void
FilterDataFieldListener::OnModified(DataField &df)
{
  if (&df == name_control->GetDataField())
    dialog_state.name = df.GetAsString();
  else if (&df == distance_filter->GetDataField())
    dialog_state.distance_index = df.GetAsInteger();
  else if (&df == direction_filter->GetDataField())
    dialog_state.direction_index = df.GetAsInteger();
  else if (&df == type_filter->GetDataField())
    dialog_state.type_index = (TypeFilter)df.GetAsInteger();

  UpdateList();
}

void
WaypointListDialog::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned i)
{
  if (waypoint_list.empty()) {
    assert(i == 0);

    const UPixelScalar line_height = rc.bottom - rc.top;
    const Font &name_font =
      *UIGlobals::GetDialogLook().list.font;
    canvas.SetTextColor(COLOR_BLACK);
    canvas.Select(name_font);
    canvas.DrawText(rc.left + line_height + Layout::FastScale(2),
                    rc.top + line_height / 2 - name_font.GetHeight() / 2,
                    dialog_state.IsDefined() || way_points.IsEmpty() ?
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
OnWaypointListEnter()
{
  if (waypoint_list.size() > 0)
    dialog->SetModalResult(mrOK);
  else
    name_control->BeginEditing();
}

void
WaypointListDialog::OnActivateItem(unsigned index)
{
  OnWaypointListEnter();
}

static void
OnSelectClicked()
{
  OnWaypointListEnter();
}

static void
OnCloseClicked()
{
  dialog->SetModalResult(mrCancel);
}

static void
OnGPSUpdate(const MoreData &basic)
{
  if (dialog_state.direction_index == 1 &&
      !CommonInterface::Calculated().circling) {
    const Angle heading = basic.attitude.heading;
    Angle a = last_heading - heading;
    if (a.AsDelta().AbsoluteDegrees() >= fixed(60)) {
      last_heading = heading;
      UpdateList();
      InitializeDirection(true);
    }
  }
}

#ifdef GNAV

static bool
FormKeyDown(unsigned key_code)
{
  TypeFilter new_index = dialog_state.type_index;

  switch (key_code) {
  case KEY_APP1:
    new_index = TypeFilter::ALL;
    break;

  case KEY_APP2:
    new_index = TypeFilter::LANDABLE;
    break;

  case KEY_APP3:
    new_index = TypeFilter::TURNPOINT;
    break;

  default:
    return false;
  }

  if (dialog_state.type_index != new_index) {
    dialog_state.type_index = new_index;
    UpdateList();
    type_filter->GetDataField()->SetAsInteger((int)dialog_state.type_index);
    type_filter->RefreshDisplay();
  }

  return true;
}

#endif /* GNAV */

static constexpr CallBackTableEntry callback_table[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnSelectClicked),
  DeclareCallBackEntry(NULL)
};

const Waypoint*
ShowWaypointListDialog(SingleWindow &parent, const GeoPoint &_location,
                       OrderedTask *_ordered_task, unsigned _ordered_task_index)
{
  dialog = LoadDialog(callback_table, parent, Layout::landscape ?
      _T("IDR_XML_WAYPOINTSELECT_L") : _T("IDR_XML_WAYPOINTSELECT"));
  assert(dialog != NULL);

#ifdef GNAV
  dialog->SetKeyDownFunction(FormKeyDown);
#endif

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  WaypointListDialog dialog2;

  waypoint_list_control = (ListControl*)dialog->FindByName(_T("frmWaypointList"));
  assert(waypoint_list_control != NULL);
  waypoint_list_control->SetItemRenderer(&dialog2);
  waypoint_list_control->SetItemHeight(WaypointListRenderer::GetHeight(dialog_look));
  waypoint_list_control->SetCursorHandler(&dialog2);

  FilterDataFieldListener listener;

  name_control = (WndProperty *)dialog->FindByName(_T("name"));
  assert(name_control != nullptr);
  DataField *name_df = new PrefixDataField(_T(""),
                                           WaypointNameAllowedCharacters);
  name_control->SetDataField(name_df);
  name_df->SetListener(&listener);

  distance_filter = (WndProperty*)dialog->FindByName(_T("prpFltDistance"));
  assert(distance_filter != NULL);
  distance_filter->GetDataField()->SetListener(&listener);

  direction_filter = (WndProperty*)dialog->FindByName(_T("prpFltDirection"));
  assert(direction_filter != NULL);
  direction_filter->GetDataField()->SetListener(&listener);

  type_filter = (WndProperty *)dialog->FindByName(_T("prpFltType"));
  assert(type_filter != NULL);
  type_filter->GetDataField()->SetListener(&listener);

  location = _location;
  ordered_task = _ordered_task;
  ordered_task_index = _ordered_task_index;
  last_heading = CommonInterface::Basic().attitude.heading;

  PrepareData();
  UpdateList();

  const ScopeGPSListener l(CommonInterface::GetLiveBlackboard(), OnGPSUpdate);

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
