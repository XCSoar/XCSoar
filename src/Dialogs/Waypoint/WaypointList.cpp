// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "ui/event/KeyCode.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Profile/Current.hpp"
#include "Profile/Map.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "system/Path.hpp"
#include "Waypoint/LastUsed.hpp"
#include "Waypoint/WaypointList.hpp"
#include "Waypoint/WaypointListBuilder.hpp"
#include "Waypoint/WaypointFilter.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Form/DataField/Enum.hpp"
#include "util/StringPointer.hxx"
#include "util/AllocatedString.hxx"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "util/Macros.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"

#include <algorithm>
#include <list>

#include <cassert>
#include <stdio.h>

enum Controls {
  NAME,
  DISTANCE,
  DIRECTION,
  TYPE,
};

static constexpr unsigned distance_filter_items[] = {
  0, 25, 50, 75, 100, 150, 250, 500, 1000
};

static constexpr int direction_filter_items[] = {
  -1, -1, 0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
};

/* Static dropdown entries for the Type filter, indexed by their
   own TypeFilter id rather than by array position so the layout
   of the enum and the layout of the dropdown can evolve
   independently.  TypeFilter::FILE is intentionally absent --
   that filter is only meaningful with a concrete file_num and
   is fed into the dropdown by the dynamic per-file entries
   added inside CreateTypeDataField (IDs >= _DYNAMIC_FILE_ID_START).
   TypeFilter::MAP is included here but only added to the
   dropdown when at least one MAP-origin waypoint is loaded
   (issue #1376), and its label is replaced with the actual
   .xcm filename when available. */
struct TypeFilterChoice {
  TypeFilter id;
  const char *label;
};

static constexpr TypeFilterChoice type_filter_choices[] = {
  {TypeFilter::ALL, "*"},
  {TypeFilter::AIRPORT, "Airport"},
  {TypeFilter::LANDABLE, "Landable"},
  {TypeFilter::TURNPOINT, "Turnpoint"},
  {TypeFilter::START, "Start"},
  {TypeFilter::FINISH, "Finish"},
  {TypeFilter::FAI_TRIANGLE_LEFT, "Left FAI Triangle"},
  {TypeFilter::FAI_TRIANGLE_RIGHT, "Right FAI Triangle"},
  {TypeFilter::USER, "Custom"},
  {TypeFilter::MAP, "Map file"},
  {TypeFilter::LAST_USED, "Recently Used"},

  /* Specific Waypoint::Type filters.  Labels match the strings
     returned by GetWaypointTypeName() in WaypointInfoWidget so
     the dropdown matches the "Type" row in the details dialog
     and reuses existing po entries. */
  {TypeFilter::MOUNTAIN_TOP, "Mountain Top"},
  {TypeFilter::MOUNTAIN_PASS, "Mountain Pass"},
  {TypeFilter::BRIDGE, "Bridge"},
  {TypeFilter::TUNNEL, "Tunnel"},
  {TypeFilter::TOWER, "Tower"},
  {TypeFilter::POWERPLANT, "Power Plant"},
  {TypeFilter::OBSTACLE, "Transmitter Mast"},
  {TypeFilter::THERMAL_HOTSPOT, "Thermal hotspot"},
  {TypeFilter::MARKER, "Marker"},
  {TypeFilter::VOR, "VOR"},
  {TypeFilter::NDB, "NDB"},
  {TypeFilter::DAM, "Dam"},
  {TypeFilter::CASTLE, "Castle"},
  {TypeFilter::INTERSECTION, "Intersection"},
  {TypeFilter::REPORTING_POINT, "Control Point"},
  {TypeFilter::PG_TAKEOFF, "PG Take Off"},
  {TypeFilter::PG_LANDING, "PG Landing Zone"},
};

/* The dropdown table must list every TypeFilter value except
   FILE (which is only ever set via the dynamic per-file
   entries inserted in CreateTypeDataField).  Compile-time
   guard so adding a new TypeFilter value prompts the developer
   to add a corresponding label here. */
static_assert(ARRAY_SIZE(type_filter_choices) == unsigned(TypeFilter::COUNT) - 1,
              "type_filter_choices must cover every TypeFilter value except FILE");

struct WaypointListDialogState
{
  StaticString<WaypointFilter::NAME_LENGTH + 1> name;

  int distance_index;
  int direction_index;
  TypeFilter type_index;
  int file_num = -1;  // For FILE type: -1=all files, 0+=specific file

  bool IsDefined() const {
    return !name.empty() || distance_index > 0 ||
      direction_index > 0 || type_index != TypeFilter::ALL;
  }

  void ToFilter(WaypointFilter &filter, Angle heading) const {
    filter.name = name;
    filter.distance =
      Units::ToSysDistance(distance_filter_items[distance_index]);
    filter.type_index = type_index;
    filter.file_num = file_num;

    if (direction_index != 1)
      filter.direction = Angle::Degrees(direction_filter_items[direction_index]);
    else
      filter.direction = heading;
  }
};

class WaypointFilterWidget;

class WaypointListWidget
  : public ListWidget, public DataFieldListener,
    NullBlackboardListener {
  Waypoints &way_points;
  TwoTextRowsRenderer row_renderer;

protected:
  WndForm &dialog;
  WaypointList items;

  WaypointFilterWidget &filter_widget;

private:
  const GeoPoint location;
  Angle last_heading;
  OrderedTask *const ordered_task;
  const unsigned ordered_task_index;
  const bool prepopulate_with_task;
  bool has_filter_prompt_row = false;

public:
  WaypointListWidget(Waypoints &_way_points, WndForm &_dialog,
                     WaypointFilterWidget &_filter_widget,
                     GeoPoint _location, Angle _heading,
                     OrderedTask *_ordered_task,
                     unsigned _ordered_task_index,
                     bool _prepopulate_with_task)
    :way_points(_way_points), dialog(_dialog),
     filter_widget(_filter_widget),
     location(_location), last_heading(_heading),
     ordered_task(_ordered_task),
     ordered_task_index(_ordered_task_index),
     prepopulate_with_task(_prepopulate_with_task) {}

  void UpdateList();

  virtual void OnWaypointListEnter();

  WaypointPtr GetCursorObject() const {
    if (items.empty())
      return nullptr;
    unsigned i = GetList().GetCursorIndex();
    if (has_filter_prompt_row) {
      if (i == 0)
        return nullptr;
      --i;
    }
    if (i >= items.size())
      return nullptr;
    return items[i].waypoint;
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    UpdateList();
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  void Hide() noexcept override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);

    ListWidget::Hide();
  }

  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;

private:
  /* virtual methods from BlackboardListener */
  void OnGPSUpdate([[maybe_unused]] const MoreData &basic) override;
};

class WaypointListPersistentWidget final
  : public WaypointListWidget {
  const bool allow_navigation;
  const bool allow_edit;

public:
  WaypointListPersistentWidget(Waypoints &_way_points, WndForm &_dialog,
                               WaypointFilterWidget &_filter_widget,
                               GeoPoint _location, Angle _heading,
                               bool _allow_navigation, bool _allow_edit)
    :WaypointListWidget(_way_points, _dialog, _filter_widget, _location, _heading,
                        nullptr, 0, false),
     allow_navigation(_allow_navigation), allow_edit(_allow_edit) {}

  void OnWaypointListEnter() override;
};

class WaypointFilterWidget : public RowFormWidget {
  Angle last_heading;

  DataFieldListener *listener;

public:
  WaypointFilterWidget(const DialogLook &look, Angle _heading)
    :RowFormWidget(look, true), last_heading(_heading) {}

  void SetListener(DataFieldListener *_listener) {
    listener = _listener;
  }

  void Update(Angle last_heading);

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) noexcept override;
};

class WaypointListButtons : public RowFormWidget {
  WndForm &dialog;
  WaypointListWidget *list;

public:
  WaypointListButtons(const DialogLook &look, WndForm &_dialog)
    :RowFormWidget(look), dialog(_dialog) {}

  void SetList(WaypointListWidget *_list) {
    list = _list;
  }

  /* virtual methods from class Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override {
    AddButton(_("Details"), [this](){
      list->OnWaypointListEnter();
    });

    AddButton(_("Close"), dialog.MakeModalResultCallback(mrCancel));
  }
};

static WaypointListDialogState dialog_state;

static const char *
GetDirectionData(char *buffer, size_t size, int direction_filter_index,
                 Angle heading)
{
  if (direction_filter_index == 0)
    return "*";
  else if (direction_filter_index == 1)
    StringFormatUnsafe(buffer, "HDG(%s)",
                       FormatBearing(heading).c_str());
  else
    FormatBearing(buffer, size, direction_filter_items[direction_filter_index]);

  return buffer;
}

void
WaypointFilterWidget::Update(Angle _last_heading)
{
  last_heading = _last_heading;

  WndProperty &direction_control = GetControl(DIRECTION);
  DataFieldEnum &direction_df = *(DataFieldEnum *)
    direction_control.GetDataField();

  char buffer[22];
  direction_df.replaceEnumText(1, GetDirectionData(buffer, ARRAY_SIZE(buffer),
                                                   1, last_heading));
  direction_control.RefreshDisplay();
}

static void
FillList(WaypointList &list, const Waypoints &src,
         GeoPoint location, Angle heading, const WaypointListDialogState &state,
         OrderedTask *ordered_task, unsigned ordered_task_index)
{
  if (!state.IsDefined() && src.size() >= 500)
    return;

  WaypointFilter filter;
  state.ToFilter(filter, heading);

  WaypointListBuilder builder(filter, location, list,
                              ordered_task, ordered_task_index);
  builder.Visit(src);

  if (filter.distance > 0 || !filter.direction.IsNegative())
    list.SortByDistance(location);
  else
    list.SortByName();

  list.MakeUnique();
}

static void
FillLastUsedList(WaypointList &list,
                 const WaypointIDList &last_used_ids,
                 const Waypoints &waypoints)
{
  for (auto it = last_used_ids.rbegin(); it != last_used_ids.rend(); it++) {
    auto waypoint = waypoints.LookupId(*it);
    if (waypoint == nullptr)
      continue;

    list.emplace_back(std::move(waypoint));
  }
}

static void
FillTaskWaypointsList(WaypointList &list, const OrderedTask &task)
{
  for (unsigned i = 0; i < task.TaskSize(); ++i) {
    auto wp = task.GetPoint(i).GetWaypointPtr();
    if (wp != nullptr)
      list.emplace_back(std::move(wp));
  }
}


void
WaypointListWidget::UpdateList()
{
  items.clear();

  if (dialog_state.type_index == TypeFilter::LAST_USED)
    FillLastUsedList(items, LastUsedWaypoints::GetList(),
                     way_points);
  else if (prepopulate_with_task && ordered_task != nullptr &&
           !dialog_state.IsDefined())
    FillTaskWaypointsList(items, *ordered_task);
  else
    FillList(items, way_points, location, last_heading,
             dialog_state,
             ordered_task, ordered_task_index);

  has_filter_prompt_row = prepopulate_with_task && ordered_task != nullptr &&
                          !dialog_state.IsDefined() && !items.empty();

  auto &list = GetList();
  const unsigned extra = has_filter_prompt_row ? 1u : 0u;
  list.SetLength(std::max(1u, (unsigned)items.size() + extra));
  list.SetOrigin(0);
  // Skip the prompt row by default so the cursor lands on the first task wp.
  list.SetCursorIndex(has_filter_prompt_row ? 1u : 0u);
  list.Invalidate();
}

void
WaypointListWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font_bold,
                                          look.small_font));
  UpdateList();
}

static DataField *
CreateNameDataField(Waypoints &waypoints, DataFieldListener *listener)
{
  /* Substring search variant of the on-screen-keyboard hint:
     enable only those keys that, when appended to the current
     input, still yield at least one substring match against any
     waypoint's name or shortname.  When nothing matches, the
     callback returns nullptr and the keyboard re-enables every
     key so the user can backspace and try again. */
  return new PrefixDataField("", [&waypoints](const char *input){
    static char buffer[256];
    return waypoints.SuggestNameSubstring(input, buffer,
                                          ARRAY_SIZE(buffer));
  }, listener);
}

static DataField *
CreateDistanceDataField(DataFieldListener *listener)
{
  DataFieldEnum *df = new DataFieldEnum(listener);
  df->addEnumText("*");

  for (unsigned i = 1; i < ARRAY_SIZE(distance_filter_items); i++) {
    df->addEnumText(FormatUserDistance(Units::ToSysDistance(distance_filter_items[i])));
  }

  df->SetValue(dialog_state.distance_index);
  return df;
}

static DataField *
CreateDirectionDataField(DataFieldListener *listener, Angle last_heading)
{
  char buffer[22];
  DataFieldEnum *df = new DataFieldEnum(listener);
  for (unsigned i = 0; i < ARRAY_SIZE(direction_filter_items); i++)
    df->addEnumText(GetDirectionData(buffer, ARRAY_SIZE(buffer), i,
                                     last_heading));

  df->SetValue(dialog_state.direction_index);
  return df;
}

static DataField *
CreateTypeDataField(DataFieldListener *listener,
                    const Waypoints &waypoints)
{
  constexpr unsigned DYNAMIC_FILE_ID_START =
    (unsigned)TypeFilter::_DYNAMIC_FILE_ID_START;

  DataFieldEnum *df = new DataFieldEnum(listener);

  /* Only show the MAP entry when waypoints from the .xcm
     archive are actually loaded.  WaypointGlue only loads the
     embedded waypoints.cup/waypoints.xcw when no other waypoint
     file produced any result (see WaypointGlue::LoadWaypoints),
     so a configured .xcm alongside a WPFileList would otherwise
     advertise a filter that yields zero matches (issue #1376).
     The label is replaced with the actual .xcm filename below. */
  const bool has_map_waypoints =
    std::any_of(waypoints.begin(), waypoints.end(),
                [](const auto &wp){
                  return wp->origin == WaypointOrigin::MAP;
                });
  const auto map_path = Profile::map.GetPathBase(ProfileKeys::MapFile);

  for (const auto &c : type_filter_choices) {
    if (c.id == TypeFilter::MAP) {
      if (!has_map_waypoints)
        continue;
      df->addEnumText(map_path != nullptr ? map_path.c_str() : c.label,
                      unsigned(c.id));
      continue;
    }
    df->addEnumText(c.label, unsigned(c.id));
  }

  // Dynamically add file entries based on loaded waypoint files.
  // IDs >= _DYNAMIC_FILE_ID_START encode the file index.
  const auto paths = Profile::GetMultiplePaths(ProfileKeys::WaypointFileList,
                                               nullptr);
  for (size_t i = 0; i < paths.size(); ++i) {
    const auto filename = paths[i].GetBase();
    if (filename != nullptr)
      df->addEnumText(filename.c_str(), DYNAMIC_FILE_ID_START + i);
  }

  // Set current value based on type_index and file_num.
  unsigned value;
  if (dialog_state.type_index == TypeFilter::FILE && dialog_state.file_num >= 0)
    value = DYNAMIC_FILE_ID_START + dialog_state.file_num;
  else
    value = (unsigned)dialog_state.type_index;
  df->SetValue(value);

  return df;
}

void
WaypointFilterWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  Add(_("Name"), nullptr, CreateNameDataField(*data_components->waypoints, listener));
  Add(_("Distance"), nullptr, CreateDistanceDataField(listener));
  Add(_("Direction"), nullptr, CreateDirectionDataField(listener, last_heading));
  Add(_("Type"), nullptr,
      CreateTypeDataField(listener, *data_components->waypoints));
}

void
WaypointListWidget::OnModified(DataField &df) noexcept
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
    unsigned value = dfe.GetValue();
    
    // Decode value: >= _DYNAMIC_FILE_ID_START means FILE filter with specific file index
    constexpr unsigned DYNAMIC_FILE_ID_START = (unsigned)TypeFilter::_DYNAMIC_FILE_ID_START;
    if (value >= DYNAMIC_FILE_ID_START) {
      dialog_state.type_index = TypeFilter::FILE;
      dialog_state.file_num = value - DYNAMIC_FILE_ID_START;
    } else {
      dialog_state.type_index = (TypeFilter)value;
      dialog_state.file_num = -1;  // Reset file_num for non-FILE filters
    }
  }

  UpdateList();
}

void
WaypointListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned i) noexcept
{
  if (items.empty()) {
    assert(i == 0);

    const auto *text = dialog_state.IsDefined() || way_points.IsEmpty()
      ? _("No Match!")
      : _("Choose a filter or click here");
    row_renderer.DrawFirstRow(canvas, rc, text);
    return;
  }

  if (has_filter_prompt_row) {
    if (i == 0) {
      // prompt row prepended above task waypoints
      row_renderer.DrawFirstRow(canvas, rc,
                                _("Choose a filter or click here"));
      return;
    }
    --i;
  }

  const struct WaypointListItem &info = items[i];

  WaypointListRenderer::Draw(canvas, rc, *info.waypoint,
                             info.GetVector(location),
                             row_renderer,
                             UIGlobals::GetMapLook().waypoint,
                             CommonInterface::GetMapSettings().waypoint);
}

void
WaypointListWidget::OnWaypointListEnter()
{
  if (items.empty()) {
    filter_widget.GetControl(NAME).BeginEditing();
    return;
  }

  if (has_filter_prompt_row && GetList().GetCursorIndex() == 0) {
    // user activated the leading "Choose a filter" prompt row
    filter_widget.GetControl(NAME).BeginEditing();
    return;
  }

  dialog.SetModalResult(mrOK);
}

void
WaypointListPersistentWidget::OnWaypointListEnter()
{
  if (items.empty()) {
    filter_widget.GetControl(NAME).BeginEditing();
    return;
  }

  if (dlgWaypointDetailsShowModalForBrowseParent(
        data_components->waypoints.get(),
        WaypointPtr(items[GetList().GetCursorIndex()].waypoint), allow_navigation,
        allow_edit))
    dialog.SetModalResult(mrOK);
}

void
WaypointListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  OnWaypointListEnter();
}

void
WaypointListWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  if (dialog_state.direction_index == 1 &&
      !CommonInterface::Calculated().circling) {
    const Angle heading = basic.attitude.heading;
    Angle a = last_heading - heading;
    if (a.AsDelta().Absolute() >= Angle::Degrees(60)) {
      last_heading = heading;
      filter_widget.Update(last_heading);
      UpdateList();
    }
  }
}

WaypointPtr
ShowWaypointListDialog(Waypoints &waypoints, const GeoPoint &_location,
                       OrderedTask *_ordered_task,
                       unsigned _ordered_task_index,
                       std::optional<TypeFilter> initial_type,
                       bool _prepopulate_with_task)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const Angle heading = CommonInterface::Basic().attitude.heading;

  dialog_state = {};

  /* When the caller forces an initial Type filter (e.g. via
     ``event=GotoLookup recent``), also reset the other filter
     dimensions so the user sees a focused list of just that
     category instead of an arbitrary intersection with stale
     distance/direction settings from an earlier invocation. */
  if (initial_type) {
    dialog_state.type_index = *initial_type;
    dialog_state.file_num = -1;
    dialog_state.distance_index = 0;
    dialog_state.direction_index = 0;
  }

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Select Waypoint"));

  auto left_widget =
    std::make_unique<TwoWidgets>(std::make_unique<WaypointFilterWidget>(look, heading),
                                 std::make_unique<WaypointListButtons>(look, dialog),
                                 true);

  auto &filter_widget = (WaypointFilterWidget &)left_widget->GetFirst();
  auto &buttons_widget = (WaypointListButtons &)left_widget->GetSecond();

  auto list_widget =
    std::make_unique<WaypointListWidget>(waypoints, dialog, filter_widget,
                                         _location, heading,
                                         _ordered_task, _ordered_task_index,
                                         _prepopulate_with_task);
  const auto &list_widget_ = *list_widget;

  filter_widget.SetListener(list_widget.get());
  buttons_widget.SetList(list_widget.get());

  TwoWidgets *widget = new TwoWidgets(std::move(left_widget),
                                      std::move(list_widget),
                                      false);

  dialog.FinishPreliminary(widget);
  return dialog.ShowModal() == mrOK
    ? list_widget_.GetCursorObject()
    : nullptr;
}

void
ShowWaypointListPersistentDialog(const GeoPoint &_location,
                                 bool allow_navigation, bool allow_edit)
{
  if (data_components == nullptr || data_components->waypoints == nullptr)
    return;

  const DialogLook &look = UIGlobals::GetDialogLook();

  const Angle heading = CommonInterface::Basic().attitude.heading;

  dialog_state.name.clear();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Select Waypoint"));

  auto left_widget =
    std::make_unique<TwoWidgets>(std::make_unique<WaypointFilterWidget>(look, heading),
                                 std::make_unique<WaypointListButtons>(look, dialog),
                                 true);

  auto &filter_widget = (WaypointFilterWidget &)left_widget->GetFirst();
  auto &buttons_widget = (WaypointListButtons &)left_widget->GetSecond();

  auto list_widget = std::make_unique<WaypointListPersistentWidget>(
    *data_components->waypoints, dialog, filter_widget, _location, heading,
    allow_navigation, allow_edit);

  filter_widget.SetListener(list_widget.get());
  buttons_widget.SetList(list_widget.get());

  TwoWidgets *widget = new TwoWidgets(std::move(left_widget),
                                      std::move(list_widget),
                                      false);

  dialog.FinishPreliminary(widget);
  dialog.ShowModal();
}

