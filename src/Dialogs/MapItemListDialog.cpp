// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/MapItemListDialog.hpp"
#include "Dialogs/MapItemPreviewWindow.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Geo/GeoPoint.hpp"
#include "MapWindow/Preview/MapPreviewFocus.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/Window.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Screen/Layout.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "MapWindow/Items/List.hpp"
#include "Renderer/MapItemListRenderer.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Weather/Features.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Geo/GeoVector.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Protection.hpp"

#include <cassert>
#include <limits>
#include <memory>
#include <variant>

#ifdef HAVE_NOAA
#include "Dialogs/Weather/NOAADetails.hpp"
#endif

static bool
HasDetails(const MapItem &item)
{
  switch (item.type) {
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    return false;

  case MapItem::Type::LOCATION:
  case MapItem::Type::AIRSPACE:
  case MapItem::Type::WAYPOINT:
  case MapItem::Type::TASK_OZ:
  case MapItem::Type::TRAFFIC:
#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
#endif
  case MapItem::Type::OVERLAY:
  case MapItem::Type::RASP:
    return true;
  }

  return false;
}

static MapPreviewFocus
MapItemToFocus(const MapItem &item) noexcept
{
  switch (item.type) {
  case MapItem::Type::AIRSPACE:
    return static_cast<const AirspaceMapItem &>(item).airspace;

  case MapItem::Type::WAYPOINT:
    return static_cast<const WaypointMapItem &>(item).waypoint;

  case MapItem::Type::LOCATION:
    return MapPreviewFocusLocationLeg{
      static_cast<const LocationMapItem &>(item).location};

  case MapItem::Type::TASK_OZ:
    return MapPreviewFocusTaskOZ{
      (unsigned)static_cast<const TaskOZMapItem &>(item).index};

  case MapItem::Type::TRAFFIC: {
    const auto &t = static_cast<const TrafficMapItem &>(item);
    return MapPreviewFocusTraffic{t.id, t.color};
  }

  case MapItem::Type::THERMAL:
    return static_cast<const ThermalMapItem &>(item).thermal.location;

  case MapItem::Type::SELF:
    return static_cast<const SelfMapItem &>(item).location;

  case MapItem::Type::ARRIVAL_ALTITUDE:
    return MapPreviewFocusLocationLeg{
      static_cast<const ArrivalAltitudeMapItem &>(item).destination};

#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
#endif
  case MapItem::Type::OVERLAY:
  case MapItem::Type::RASP:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    return std::monostate{};
  }

  return std::monostate{};
}

class MapItemPreviewWidget final : public WindowWidget {
public:
  explicit MapItemPreviewWidget(std::unique_ptr<MapItemPreviewWindow> w) noexcept
    : WindowWidget(std::move(w)) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    auto &map = static_cast<MapItemPreviewWindow &>(GetWindow());

    WindowStyle style;
    style.Hide();
    style.Border();

    map.Create(parent, rc, style);
  }

  PixelSize GetMinimumSize() const noexcept override {
    return {Layout::Scale(120), Layout::Scale(100)};
  }

  PixelSize GetMaximumSize() const noexcept override {
    return WidgetMaximumSizeUnbounded();
  }
};

class MapItemListWidget final
  : public ListWidget {
  const MapItemList &list;

  const DialogLook &dialog_look;
  const MapSettings &settings;

  const GeoPoint query_location;

  MapItemListRenderer renderer;

  MapItemPreviewWindow *preview_map = nullptr;

  Button *settings_button, *details_button, *cancel_button, *goto_button;
  Button *ack_button, *enable_button;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  MapItemListWidget(const MapItemList &_list,
                    const DialogLook &_dialog_look, const MapLook &_look,
                    const TrafficLook &_traffic_look,
                    const FinalGlideBarLook &_final_glide_look,
                    const MapSettings &_settings,
                    GeoPoint _query_location) noexcept
    :list(_list),
     dialog_look(_dialog_look),
     settings(_settings),
     query_location(_query_location),
     renderer(_look, _traffic_look, _final_glide_look,
              _settings, CommonInterface::GetComputerSettings().utc_offset) {}

  void SetPreviewMap(MapItemPreviewWindow *p) noexcept {
    preview_map = p;
  }

  void SyncPreviewMap() noexcept {
    if (preview_map == nullptr)
      return;

    preview_map->SetQueryFallbackLocation(query_location);
    preview_map->SetPreviewFocus(MapItemToFocus(*list[GetCursorIndex()]));
  }

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

protected:
  void UpdateButtons() {
    const unsigned current = GetCursorIndex();
    details_button->SetEnabled(HasDetails(*list[current]));
    goto_button->SetEnabled(CanGotoItem(current));
    ack_button->SetEnabled(CanAckItem(current));
    enable_button->SetEnabled(CanEnableItem(current));
  }

  void OnGotoClicked();
  void OnAckClicked();
  void OnEnableClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
    SyncPreviewMap();
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return HasDetails(*list[index]);
  }

  bool CanGotoItem(unsigned index) const noexcept {
    return CanGotoItem(*list[index]);
  }

  static bool CanGotoItem(const MapItem &item) noexcept {
    return backend_components != nullptr &&
           backend_components->protected_task_manager &&
           (item.type == MapItem::Type::WAYPOINT ||
            item.type == MapItem::Type::LOCATION);
  }

  bool CanAckItem(unsigned index) const noexcept {
    return CanAckItem(*list[index]);
  }

  static bool CanAckItem(const MapItem &item) noexcept {
    if (backend_components == nullptr)
      return false;

    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;

    return item.type == MapItem::Type::AIRSPACE &&
      backend_components->GetAirspaceWarnings() != nullptr &&
      !backend_components->GetAirspaceWarnings()->GetAckDay(*as_item.airspace);
  }

  bool CanEnableItem(unsigned index) const noexcept {
    return CanEnableItem(*list[index]);
  }

  static bool CanEnableItem(const MapItem &item) noexcept {
    if (backend_components == nullptr)
      return false;

    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;

    return item.type == MapItem::Type::AIRSPACE &&
      backend_components->GetAirspaceWarnings() != nullptr &&
      backend_components->GetAirspaceWarnings()->GetAckDay(*as_item.airspace);
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
MapItemListWidget::CreateButtons(WidgetDialog &dialog)
{
  details_button = dialog.AddButton(_("Details"), mrOK);

  goto_button = dialog.AddButton(_("Goto"), [this](){
    OnGotoClicked();
  });

  ack_button = dialog.AddButton(_("Ack Day"), [this](){
    OnAckClicked();
  });

  enable_button = dialog.AddButton(_("Enable"), [this](){
    OnEnableClicked();
  });

  settings_button = dialog.AddButton(_("Settings"), [](){
    ShowMapItemListSettingsDialog();
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
MapItemListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  CreateList(parent, dialog_look, rc,
             renderer.CalculateLayout(dialog_look));

  GetList().SetLength(list.size());
  UpdateButtons();

  // First, try to find a waypoint to preselect
  unsigned selected_index = list.size();
  for (unsigned i = 0; i < list.size(); ++i) {
    const MapItem &item = *list[i];
    if (item.type == MapItem::Type::WAYPOINT) {
      selected_index = i;
      break;
    }
  }

  // If no waypoint found, fall back to first item with details or can goto
  if (selected_index >= list.size()) {
    for (unsigned i = 0; i < list.size(); ++i) {
      const MapItem &item = *list[i];
      if (HasDetails(item) || CanGotoItem(item)) {
        selected_index = i;
        break;
      }
    }
  }

  // Set cursor if we found something to select
  if (selected_index < list.size()) {
    GetList().SetCursorIndex(selected_index);
    UpdateButtons();
  }

  SyncPreviewMap();
}

void
MapItemListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx) noexcept
{
  const MapItem &item = *list[idx];

  if (item.type == MapItem::Type::AIRSPACE &&
      backend_components != nullptr &&
      backend_components->GetAirspaceWarnings() != nullptr &&
      backend_components->GetAirspaceWarnings()->GetAckDay(
        *static_cast<const AirspaceMapItem &>(item).airspace))
    canvas.SetTextColor(COLOR_GRAY);
  else
    canvas.SetTextColor(dialog_look.list.text_color);

  renderer.Draw(canvas, rc, item,
                &CommonInterface::Basic().flarm.traffic);

  if ((settings.item_list.add_arrival_altitude &&
       item.type == MapItem::Type::ARRIVAL_ALTITUDE) ||
      (!settings.item_list.add_arrival_altitude &&
       item.type == MapItem::Type::LOCATION)) {
    canvas.SelectBlackPen();
    canvas.DrawLine({rc.left, rc.bottom - 1}, {rc.right, rc.bottom - 1});
  }
}

void
MapItemListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  details_button->Click();
}

inline void
MapItemListWidget::OnGotoClicked()
{
  if (!backend_components->protected_task_manager)
    return;

  if (data_components == nullptr || data_components->waypoints == nullptr)
    return;

  unsigned index = GetCursorIndex();
  auto const &item = *list[index];

  assert(item.type == MapItem::Type::WAYPOINT ||
         item.type == MapItem::Type::LOCATION);

  WaypointPtr waypoint;

  if (item.type == MapItem::Type::LOCATION) {
    const auto &loc_item = static_cast<const LocationMapItem &>(item);

    // Use the stored location directly
    const GeoPoint &location = loc_item.location;

    // Get terrain elevation (prefer stored elevation, fall back to terrain lookup)
    double elevation = std::numeric_limits<double>::quiet_NaN();
    if (loc_item.HasElevation()) {
      elevation = loc_item.elevation;
    } else if (data_components->terrain != nullptr) {
      const auto h = data_components->terrain->GetTerrainHeight(location);
      if (!h.IsSpecial()) {
        elevation = h.GetValue();
      }
    }

    // Create temporary goto waypoint (elevation may be NaN if unavailable)
    auto &way_points = *data_components->waypoints;
    const char *goto_name = "(goto)";
    {
      ScopeSuspendAllThreads suspend;
      way_points.AddTempPoint(location, elevation, goto_name);
      waypoint = way_points.LookupName(goto_name);
    }
    if (!waypoint)
      return;
  } else {
    waypoint = static_cast<const WaypointMapItem &>(item).waypoint;

    // Remove old temporary goto waypoint when selecting a regular waypoint
    auto &way_points = *data_components->waypoints;
    {
      ScopeSuspendAllThreads suspend;
      way_points.EraseTempGoto();
    }
  }

  backend_components->protected_task_manager->DoGoto(std::move(waypoint));
  cancel_button->Click();
}

inline void
MapItemListWidget::OnAckClicked()
{
  const AirspaceMapItem &as_item = *(const AirspaceMapItem *)
    list[GetCursorIndex()];
  backend_components->GetAirspaceWarnings()->AcknowledgeDay(as_item.airspace);
  UpdateButtons();
}

inline void
MapItemListWidget::OnEnableClicked()
{
  const AirspaceMapItem &as_item = *(const AirspaceMapItem *)
    list[GetCursorIndex()];
  backend_components->GetAirspaceWarnings()->AcknowledgeDay(as_item.airspace,
                                                            false);
  UpdateButtons();
}

static int
RunMapItemListModal(const MapItemList &list,
                    GeoPoint query_location,
                    const DialogLook &dialog_look, const MapLook &look,
                    const TrafficLook &traffic_look,
                    const FinalGlideBarLook &final_glide_look,
                    const MapSettings &settings)
{
  auto preview_window =
    std::make_unique<MapItemPreviewWindow>(look, traffic_look);
  preview_window->SetQueryFallbackLocation(query_location);
  MapItemPreviewWindow *const preview_raw = preview_window.get();

  auto preview_widget =
    std::make_unique<MapItemPreviewWidget>(std::move(preview_window));

  auto list_widget = std::make_unique<MapItemListWidget>(
    list, dialog_look, look, traffic_look, final_glide_look, settings,
    query_location);
  list_widget->SetPreviewMap(preview_raw);

  auto split = std::make_unique<TwoWidgets>(
    std::move(list_widget), std::move(preview_widget),
    TwoWidgetsSplit::SCREEN_ORIENTATION);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      dialog_look, _("Map elements at this location"),
                      split.get());

  auto &list_ref = static_cast<MapItemListWidget &>(split->GetFirst());

  list_ref.CreateButtons(dialog);
  dialog.EnableCursorSelection();

  const int result = dialog.ShowModal() == mrOK
    ? (int)list_ref.GetCursorIndex()
    : -1;

  assert(dialog.StealWidget() == split.get());
  split.reset();

  return result;
}

static void
ShowMapItemDialog(const MapItem &item,
                  Waypoints *waypoints,
                  ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (item.type) {
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    break;

  case MapItem::Type::AIRSPACE:
    dlgAirspaceDetails(((const AirspaceMapItem &)item).airspace,
                       airspace_warnings);
    break;
  case MapItem::Type::LOCATION: {
    if (waypoints == nullptr)
      break;

    const auto &loc_item = static_cast<const LocationMapItem &>(item);

    // Use the stored location directly
    const GeoPoint &location = loc_item.location;

    // Get terrain elevation (prefer stored elevation, fall back to terrain lookup)
    double elevation = std::numeric_limits<double>::quiet_NaN();
    if (loc_item.HasElevation()) {
      elevation = loc_item.elevation;
    } else if (data_components != nullptr &&
               data_components->terrain != nullptr) {
      const auto h = data_components->terrain->GetTerrainHeight(location);
      if (!h.IsSpecial()) {
        elevation = h.GetValue();
      }
    }

    // Create temporary goto waypoint for display (elevation may be NaN if unavailable)
    const char *goto_name = "(goto)";
    {
      ScopeSuspendAllThreads suspend;
      waypoints->AddTempPoint(location, elevation, goto_name);
    }

    // Lookup the waypoint we just created and show details
    auto wp = waypoints->LookupName(goto_name);
    if (wp)
      dlgWaypointDetailsShowModal(waypoints, wp, true, true);
    break;
  }
  case MapItem::Type::WAYPOINT:
    dlgWaypointDetailsShowModal(waypoints,
                                ((const WaypointMapItem &)item).waypoint,
                                true, true);
    break;
  case MapItem::Type::TASK_OZ:
    dlgTargetShowModal(((const TaskOZMapItem &)item).index);
    break;
  case MapItem::Type::TRAFFIC:
    dlgFlarmTrafficDetailsShowModal(((const TrafficMapItem &)item).id);
    break;

#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
    dlgNOAADetailsShowModal(((const WeatherStationMapItem &)item).station);
    break;
#endif

  case MapItem::Type::OVERLAY:
    ShowWeatherDialog("overlay");
    break;

  case MapItem::Type::RASP:
    ShowWeatherDialog("rasp");
    break;
  }
}

void
ShowMapItemListDialog(const MapItemList &list,
                      const GeoPoint &query_location,
                      const DialogLook &dialog_look,
                      const MapLook &look,
                      const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings,
                      Waypoints *waypoints,
                      ProtectedAirspaceWarningManager *airspace_warnings)
{
  if (list.empty())
    /* no map items in the list */
    return;

  /* always show list dialog when there are items, so user can choose action */
  int i = RunMapItemListModal(list, query_location, dialog_look, look,
                              traffic_look, final_glide_look, settings);
  assert(i >= -1 && i < (int)list.size());
  if (i >= 0)
    ShowMapItemDialog(*list[i], waypoints, airspace_warnings);
}
