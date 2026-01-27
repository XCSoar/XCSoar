// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/MapItemListDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "Look/MapLook.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "MapWindow/Items/List.hpp"
#include "MapWindow/MapItemPreviewWindow.hpp"
#include "Renderer/MapItemListRenderer.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Weather/Features.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/Window.hpp"
#include "Waypoint/Factory.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "ui/window/PaintWindow.hpp"

#ifdef HAVE_NOAA
#include "Dialogs/Weather/NOAADetails.hpp"
#endif

class SeparatorWindow : public PaintWindow {
protected:
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.SelectBlackPen();
    const PixelSize size = GetSize();
    if (Layout::landscape) {
      // Vertical separator for landscape
      const int middle = size.width / 2;
      canvas.DrawLine(PixelPoint(middle, 0),
                      PixelPoint(middle, size.height));
    } else {
      // Horizontal separator for portrait
      const int middle = size.height / 2;
      canvas.DrawLine(PixelPoint(0, middle),
                      PixelPoint(size.width, middle));
    }
  }
};

class HorizontalSeparatorWindow : public PaintWindow {
protected:
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.SelectBlackPen();
    const PixelSize size = GetSize();
    // Always draw horizontal line in the middle
    const int middle = size.height / 2;
    canvas.DrawLine(PixelPoint(0, middle),
                    PixelPoint(size.width, middle));
  }
};

static bool
HasDetails(const MapItem &item)
{
  switch (item.type) {
  case MapItem::Type::LOCATION:
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    return false;

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

class MapItemListWidget final
  : public ListWidget {
  const MapItemList &list;

  const DialogLook &dialog_look;
  const MapLook &map_look;
  const TrafficLook &traffic_look;
  const MapSettings &settings;

  MapItemListRenderer renderer;
  MapItemPreviewWindow *preview_window = nullptr;
  SeparatorWindow *separator = nullptr;
  HorizontalSeparatorWindow *button_separator = nullptr;

  Button *settings_button, *details_button, *cancel_button, *goto_button;
  Button *ack_button;

  PixelRect list_rect;
  PixelRect preview_rect;
  PixelRect separator_rect;
  PixelRect button_separator_rect;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  MapItemListWidget(const MapItemList &_list,
                    const DialogLook &_dialog_look, const MapLook &_look,
                    const TrafficLook &_traffic_look,
                    const FinalGlideBarLook &_final_glide_look,
                    const MapSettings &_settings)
    :list(_list),
     dialog_look(_dialog_look),
     map_look(_look),
     traffic_look(_traffic_look),
     settings(_settings),
     renderer(_look, _traffic_look, _final_glide_look,
              _settings, CommonInterface::GetComputerSettings().utc_offset) {}

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

protected:
  void UpdateButtons() {
    const unsigned current = GetCursorIndex();
    details_button->SetEnabled(HasDetails(*list[current]));
    goto_button->SetEnabled(CanGotoItem(current));
    ack_button->SetEnabled(CanAckItem(current));
  }

  void OnGotoClicked();
  void OnAckClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
    UpdatePreview();
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return HasDetails(*list[index]);
  }

  bool CanGotoItem(unsigned index) const noexcept {
    return CanGotoItem(*list[index]);
  }

  static bool CanGotoItem(const MapItem &item) noexcept {
    if (!backend_components->protected_task_manager)
      return false;
    
    if (item.type == MapItem::Type::WAYPOINT)
      return true;
    
    if (item.type == MapItem::Type::LOCATION) {
      const LocationMapItem &loc_item = (const LocationMapItem &)item;
      return loc_item.vector.IsValid() && CommonInterface::Basic().location_available;
    }
    
    return false;
  }

  bool CanAckItem(unsigned index) const noexcept {
    return CanAckItem(*list[index]);
  }

  static bool CanAckItem(const MapItem &item) noexcept {
    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;

    return item.type == MapItem::Type::AIRSPACE &&
      backend_components->GetAirspaceWarnings() != nullptr &&
      !backend_components->GetAirspaceWarnings()->GetAckDay(*as_item.airspace);
  }

  void OnActivateItem(unsigned index) noexcept override;

  void UpdatePreview() noexcept {
    if (preview_window != nullptr && GetCursorIndex() < list.size()) {
      preview_window->SetMapItem(*list[GetCursorIndex()]);
    }
  }

  ~MapItemListWidget() noexcept {
    delete preview_window;
    delete separator;
    delete button_separator;
  }

  void Show(const PixelRect &rc) noexcept override {
    CalculateLayout(rc);
    ListWidget::Show(list_rect);
    if (preview_window != nullptr) {
      preview_window->MoveAndShow(preview_rect);
      UpdatePreview();
    }
    if (separator != nullptr)
      separator->MoveAndShow(separator_rect);
    if (button_separator != nullptr)
      button_separator->MoveAndShow(button_separator_rect);
  }

  void Hide() noexcept override {
    if (preview_window != nullptr)
      preview_window->Hide();
    if (separator != nullptr)
      separator->Hide();
    if (button_separator != nullptr)
      button_separator->Hide();
    ListWidget::Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    CalculateLayout(rc);
    ListWidget::Move(list_rect);
    if (preview_window != nullptr)
      preview_window->Move(preview_rect);
    if (separator != nullptr)
      separator->Move(separator_rect);
    if (button_separator != nullptr)
      button_separator->Move(button_separator_rect);
  }

private:
  void CalculateLayout(const PixelRect &rc) noexcept {
    constexpr unsigned separator_size = 2;

    // Reserve space for button separator at the top
    button_separator_rect = rc;
    button_separator_rect.bottom = button_separator_rect.top + separator_size;
    
    PixelRect remaining_rc = rc;
    remaining_rc.top = button_separator_rect.bottom;

    if (Layout::landscape) {
      // Landscape: split vertically, list on left, preview on right
      list_rect = remaining_rc;
      const unsigned split = remaining_rc.GetWidth() / 2;
      preview_rect = list_rect.CutRightSafe(split);
      
      // Add separator between list and preview
      separator_rect = remaining_rc;
      separator_rect.left = list_rect.right;
      separator_rect.right = separator_rect.left + separator_size;
      
      // Button separator spans full width
      button_separator_rect.left = rc.left;
      button_separator_rect.right = rc.right;
    } else {
      // Portrait: split horizontally, list on top, preview on bottom
      list_rect = remaining_rc;
      const unsigned split = remaining_rc.GetHeight() / 2;
      preview_rect = list_rect.CutBottomSafe(split);
      
      // Add separator between list and preview
      separator_rect = remaining_rc;
      separator_rect.top = list_rect.bottom;
      separator_rect.bottom = separator_rect.top + separator_size;
      
      // Button separator spans full width
      button_separator_rect.left = rc.left;
      button_separator_rect.right = rc.right;
    }
  }
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

  settings_button = dialog.AddButton(_("Settings"), [](){
    ShowMapItemListSettingsDialog();
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
MapItemListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  CalculateLayout(rc);

  CreateList(parent, dialog_look, list_rect,
             renderer.CalculateLayout(dialog_look));

  // Create button separator (horizontal line at top)
  WindowStyle button_separator_style;
  button_separator_style.Hide();
  button_separator = new HorizontalSeparatorWindow();
  button_separator->Create(parent, button_separator_rect, button_separator_style);

  // Create separator between list and preview
  WindowStyle separator_style;
  separator_style.Hide();
  separator = new SeparatorWindow();
  separator->Create(parent, separator_rect, separator_style);

  // Create preview window
  WindowStyle preview_style;
  preview_style.Hide();

  preview_window = new MapItemPreviewWindow(
    map_look.waypoint, map_look.airspace,
    map_look.trail, map_look.task, map_look.aircraft,
    map_look.topography, map_look.overlay, traffic_look);
  preview_window->Create(parent, preview_rect, preview_style);

  if (data_components != nullptr) {
    preview_window->SetTerrain(data_components->terrain.get());
    preview_window->SetTopograpgy(data_components->topography.get());
    preview_window->SetAirspaces(data_components->airspaces.get());
    preview_window->SetWaypoints(data_components->waypoints.get());
  }

  if (backend_components != nullptr) {
    preview_window->SetTask(backend_components->protected_task_manager.get());
    preview_window->SetGlideComputer(backend_components->glide_computer.get());
  }

  GetList().SetLength(list.size());
  UpdateButtons();

  for (unsigned i = 0; i < list.size(); ++i) {
    const MapItem &item = *list[i];
    if (HasDetails(item) || CanGotoItem(item)) {
      GetList().SetCursorIndex(i);
      break;
    }
  }

  UpdatePreview();
}

void
MapItemListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx) noexcept
{
  const MapItem &item = *list[idx];
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

  unsigned index = GetCursorIndex();
  auto const &item = *list[index];

  if (item.type == MapItem::Type::WAYPOINT) {
    auto waypoint = ((const WaypointMapItem &)item).waypoint;
    backend_components->protected_task_manager->DoGoto(std::move(waypoint));
    cancel_button->Click();
  } else if (item.type == MapItem::Type::LOCATION) {
    const LocationMapItem &loc_item = (const LocationMapItem &)item;
    const MoreData &basic = CommonInterface::Basic();
    
    if (!loc_item.vector.IsValid() || !basic.location_available)
      return;
    
    // Calculate target location
    const GeoPoint target = loc_item.vector.EndPoint(basic.location);
    
    // Reuse the goto task logic similar to TakeoffAutotask
    if (data_components != nullptr && data_components->waypoints != nullptr) {
      ProtectedTaskManager::ExclusiveLease task_manager{*backend_components->protected_task_manager};
      
      // Try to find a nearby landable waypoint first
      auto wp = data_components->waypoints->GetNearestLandable(target, 5000);
      
      if (!wp) {
        // Create a temporary waypoint at target location (similar to GenerateTakeoffPoint)
        double terrain_alt = 0;
        if (data_components->terrain != nullptr) {
          terrain_alt = data_components->terrain->GetTerrainHeight(target)
            .ToDouble(0);
        }
        
        Waypoint temp_wp = data_components->waypoints->GenerateTakeoffPoint(target, terrain_alt);
        temp_wp.name = _T("Goto");
        wp = std::make_unique<Waypoint>(std::move(temp_wp));
      }
      
      if (task_manager->DoGoto(std::move(wp))) {
        cancel_button->Click();
      }
    }
  }
}

inline void
MapItemListWidget::OnAckClicked()
{
  const AirspaceMapItem &as_item = *(const AirspaceMapItem *)
    list[GetCursorIndex()];
  backend_components->GetAirspaceWarnings()->AcknowledgeDay(as_item.airspace);
  UpdateButtons();
}

static int
ShowMapItemListDialog(const MapItemList &list,
                      const DialogLook &dialog_look, const MapLook &look,
                      const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings)
{
  TWidgetDialog<MapItemListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           dialog_look, _("Map elements at this location"));
  dialog.SetWidget(list, dialog_look, look,
                   traffic_look, final_glide_look,
                   settings);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.EnableCursorSelection();

  return dialog.ShowModal() == mrOK
    ? (int)dialog.GetWidget().GetCursorIndex()
    : -1;
}

static void
ShowMapItemDialog(const MapItem &item,
                  Waypoints *waypoints,
                  ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (item.type) {
  case MapItem::Type::LOCATION:
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
    ShowWeatherDialog(_T("overlay"));
    break;

  case MapItem::Type::RASP:
    ShowWeatherDialog(_T("rasp"));
    break;
  }
}

void
ShowMapItemListDialog(const MapItemList &list,
                      const DialogLook &dialog_look,
                      const MapLook &look,
                      const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings,
                      Waypoints *waypoints,
                      ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (list.size()) {
  case 0:
    /* no map items in the list */
    return;

  case 1:
    /* only one map item, show it */
    ShowMapItemDialog(*list[0], waypoints, airspace_warnings);
    break;

  default:
    /* more than one map item: show a list */

    int i = ShowMapItemListDialog(list, dialog_look, look,
                                  traffic_look, final_glide_look, settings);
    assert(i >= -1 && i < (int)list.size());
    if (i >= 0)
      ShowMapItemDialog(*list[i], waypoints, airspace_warnings);
  }
}
