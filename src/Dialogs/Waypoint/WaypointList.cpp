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
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Profile/Profile.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/WaypointListBuilder.hpp"
#include "Waypoint/WaypointFilter.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Components.hpp"
#include "Compiler.h"
#include "Form/DataField/Enum.hpp"
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
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <list>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

enum Controls {
  NAME,
  DISTANCE,
  DIRECTION,
  TYPE,
};

enum Buttons {
  SELECT,
};

static GeoPoint location;

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
  nullptr
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

class WaypointFilterWidget;
class WaypointListButtons;

class WaypointListWidget final
  : public ListWidget, public DataFieldListener,
    public ActionListener, NullBlackboardListener {
  ActionListener &action_listener;

  WaypointFilterWidget &filter_widget;

  WaypointList items;

public:
  WaypointListWidget(ActionListener &_action_listener,
                     WaypointFilterWidget &_filter_widget)
    :action_listener(_action_listener),
    filter_widget(_filter_widget) {}

  void UpdateList();

  void OnWaypointListEnter();

  const Waypoint *GetCursorObject() const {
    return items.empty()
      ? nullptr
      : items[GetList().GetCursorIndex()].waypoint;
  }

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

  virtual void Unprepare() override {
    DeleteWindow();
  }

  virtual void Show(const PixelRect &rc) override {
    ListWidget::Show(rc);
    UpdateList();
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  virtual void Hide() override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);

    ListWidget::Hide();
  }

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

private:
  /* virtual methods from BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override;
};

class WaypointFilterWidget : public RowFormWidget {
  DataFieldListener *listener;

public:
  WaypointFilterWidget(const DialogLook &look)
    :RowFormWidget(look, true) {}

  void SetListener(DataFieldListener *_listener) {
    listener = _listener;
  }

  void Update();

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
#ifdef GNAV
  virtual bool KeyPress(unsigned key_code) override;
#endif
};

class WaypointListButtons : public RowFormWidget {
  ActionListener &dialog;
  ActionListener *list;

public:
  WaypointListButtons(const DialogLook &look, ActionListener &_dialog)
    :RowFormWidget(look), dialog(_dialog) {}

  void SetList(ActionListener *_list) {
    list = _list;
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    AddButton(_("Select"), *list, SELECT);
    AddButton(_("Cancel"), dialog, mrCancel);
  }
};

static WaypointListDialogState dialog_state;

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

void
WaypointFilterWidget::Update()
{
  WndProperty &direction_control = GetControl(DIRECTION);
  DataFieldEnum &direction_df = *(DataFieldEnum *)
    direction_control.GetDataField();

  TCHAR buffer[12];
  direction_df.replaceEnumText(1, GetDirectionData(buffer, ARRAY_SIZE(buffer),
                                                   1));
  direction_control.RefreshDisplay();
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
    if (waypoint == nullptr)
      continue;

    list.emplace_back(*waypoint);
  }
}

void
WaypointListWidget::UpdateList()
{
  items.clear();

  if (dialog_state.type_index == TypeFilter::LAST_USED)
    FillLastUsedList(items, LastUsedWaypoints::GetList(),
                     way_points);
  else
    FillList(items, way_points, location, last_heading,
             dialog_state);

  auto &list = GetList();
  list.SetLength(std::max(1u, (unsigned)items.size()));
  list.SetOrigin(0);
  list.SetCursorIndex(0);
  list.Invalidate();
}

void
WaypointListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc, WaypointListRenderer::GetHeight(look));
  UpdateList();
}

static const TCHAR *
WaypointNameAllowedCharacters(const TCHAR *prefix)
{
  static TCHAR buffer[256];
  return way_points.SuggestNamePrefix(prefix, buffer, ARRAY_SIZE(buffer));
}

static DataField *
CreateNameDataField(DataFieldListener *listener)
{
  return new PrefixDataField(_T(""), WaypointNameAllowedCharacters, listener);
}

static DataField *
CreateDistanceDataField(DataFieldListener *listener)
{
  DataFieldEnum *df = new DataFieldEnum(listener);
  df->addEnumText(_T("*"));

  TCHAR buffer[15];
  for (unsigned i = 1; i < ARRAY_SIZE(distance_filter_items); i++) {
    FormatUserDistance(Units::ToSysDistance(fixed(distance_filter_items[i])),
                       buffer);
    df->addEnumText(buffer);
  }

  df->Set(dialog_state.distance_index);
  return df;
}

static DataField *
CreateDirectionDataField(DataFieldListener *listener)
{
  TCHAR buffer[12];
  DataFieldEnum *df = new DataFieldEnum(listener);
  for (unsigned i = 0; i < ARRAY_SIZE(direction_filter_items); i++)
    df->addEnumText(GetDirectionData(buffer, ARRAY_SIZE(buffer), i));

  df->Set(dialog_state.direction_index);
  return df;
}

static DataField *
CreateTypeDataField(DataFieldListener *listener)
{
  DataFieldEnum *df = new DataFieldEnum(listener);
  df->addEnumTexts(type_filter_items);

  const TCHAR *p = Profile::GetPathBase(ProfileKeys::WaypointFile);
  if (p != nullptr)
    df->replaceEnumText((unsigned)TypeFilter::FILE_1, p);

  p = Profile::GetPathBase(ProfileKeys::AdditionalWaypointFile);
  if (p != nullptr)
    df->replaceEnumText((unsigned)TypeFilter::FILE_2, p);

  df->Set((int)dialog_state.type_index);
  return df;
}

void
WaypointFilterWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc)
{
  Add(_("Name"), nullptr, CreateNameDataField(listener));
  Add(_("Distance"), nullptr, CreateDistanceDataField(listener));
  Add(_("Direction"), nullptr, CreateDirectionDataField(listener));
  Add(_("Type"), nullptr, CreateTypeDataField(listener));
}

void
WaypointListWidget::OnModified(DataField &df)
{
  if (filter_widget.IsDataField(NAME, df)) {
    dialog_state.name = df.GetAsString();

    /* pass the focus to the list so the user can use the up/down keys
       to select an item right away after the text input dialog has
       been closed; however if the value was changed by
       incrementing/decrementing the first letter (cursor left/right),
       don't move the focus; we don't know for sure how the value was
       changed, but if the filter has only one letter, it's most
       likely changed by left/right */
    if (dialog_state.name.length() > 1)
      GetList().SetFocus();
  } else if (filter_widget.IsDataField(DISTANCE, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    dialog_state.distance_index = dfe.GetValue();
  } else if (filter_widget.IsDataField(DIRECTION, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    dialog_state.direction_index = dfe.GetValue();
  } else if (filter_widget.IsDataField(TYPE, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    dialog_state.type_index = (TypeFilter)dfe.GetValue();
  }

  UpdateList();
}

void
WaypointListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned i)
{
  if (items.empty()) {
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

  assert(i < items.size());

  const struct WaypointListItem &info = items[i];

  WaypointListRenderer::Draw(canvas, rc, *info.waypoint,
                             info.GetVector(location),
                             UIGlobals::GetDialogLook(),
                             UIGlobals::GetMapLook().waypoint,
                             CommonInterface::GetMapSettings().waypoint);
}

void
WaypointListWidget::OnWaypointListEnter()
{
  if (!items.empty())
    action_listener.OnAction(mrOK);
  else
    filter_widget.GetControl(NAME).BeginEditing();
}

void
WaypointListWidget::OnActivateItem(unsigned index)
{
  OnWaypointListEnter();
}

void
WaypointListWidget::OnAction(int id)
{
  switch (Buttons(id)) {
  case SELECT:
    OnWaypointListEnter();
    break;
  }
}

void
WaypointListWidget::OnGPSUpdate(const MoreData &basic)
{
  if (dialog_state.direction_index == 1 &&
      !CommonInterface::Calculated().circling) {
    const Angle heading = basic.attitude.heading;
    Angle a = last_heading - heading;
    if (a.AsDelta().AbsoluteDegrees() >= fixed(60)) {
      last_heading = heading;
      filter_widget.Update();
      UpdateList();
    }
  }
}

#ifdef GNAV

bool
WaypointFilterWidget::KeyPress(unsigned key_code)
{
  switch (key_code) {
  case KEY_APP1:
    LoadValueEnum(TYPE, TypeFilter::ALL);
    return true;

  case KEY_APP2:
    LoadValueEnum(TYPE, TypeFilter::LANDABLE);
    return true;

  case KEY_APP3:
    LoadValueEnum(TYPE, TypeFilter::TURNPOINT);
    return true;

  default:
    return false;
  }
}

#endif /* GNAV */

const Waypoint*
ShowWaypointListDialog(const GeoPoint &_location,
                       OrderedTask *_ordered_task, unsigned _ordered_task_index)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  location = _location;
  ordered_task = _ordered_task;
  ordered_task_index = _ordered_task_index;
  last_heading = CommonInterface::Basic().attitude.heading;
  dialog_state.name.clear();

  WidgetDialog dialog(look);

  WaypointFilterWidget *filter_widget = new WaypointFilterWidget(look);

  WaypointListButtons *buttons_widget = new WaypointListButtons(look, dialog);

  TwoWidgets *left_widget =
    new TwoWidgets(filter_widget, buttons_widget, true);

  WaypointListWidget *const list_widget =
    new WaypointListWidget(dialog, *filter_widget);

  filter_widget->SetListener(list_widget);
  buttons_widget->SetList(list_widget);

  TwoWidgets *widget = new TwoWidgets(left_widget, list_widget, false);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Select Waypoint"), widget);
  return dialog.ShowModal() == mrOK
    ? list_widget->GetCursorObject()
    : nullptr;
}
