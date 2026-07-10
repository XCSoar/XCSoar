// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageActions.hpp"
#include "UIActions.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "MainWindow.hpp"
#include "CrossSection/CrossSectionWidget.hpp"
#include "DataGlobals.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Pan.hpp"
#include "Input/InputEvents.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "Weather/MapOverlay/ControlsFactory.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/MapOverlay/PagePlacement.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Profile/Current.hpp"
#include "Profile/PageProfile.hpp"
#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#endif
#ifdef HAVE_EDL
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/StateController.hpp"
#endif
#ifdef HAVE_HTTP
#include "Weather/Skysight/Skysight.hpp"
#include "Weather/xctherm/XCThermMapOverlay.hpp"
#endif

#if defined(ENABLE_SDL) && defined(main)
/* on some platforms, SDL wraps the main() function and clutters our
   namespace with a macro called "main" */
#undef main
#endif

namespace PageActions {
  /**
   * Call this when we're about to leave the current page.  This
   * function checks if settings need to be remembered.
   */
  static void LeavePage();

  /**
   * Restore the map zoom afte switching to a configured page.
   */
  static void RestoreMapZoom();

  /**
   * Loads the layout without updating current page information in
   * #UIState.
   */
  static void LoadLayout(const PageLayout &layout);

  static const PageLayout &
  GetActiveLayout() noexcept;

  static void ClearPageOverlays() noexcept;

  static void LeaveRaspOverlay() noexcept;
  static void LeaveEdlOverlay() noexcept;
  static void LeaveXcthermOverlay() noexcept;
  static void LeaveSkysightOverlay() noexcept;

  static void LeaveWeatherOverlayPage(const PageLayout &layout) noexcept;

  static void ApplyRaspOverlay(const PageLayout &layout) noexcept;
  static void ApplyEdlOverlay() noexcept;
  static void ApplyXcthermOverlay() noexcept;
  static void ApplySkysightOverlay(const PageLayout &layout) noexcept;

  static void ApplyPageOverlay(const PageLayout &layout) noexcept;

  static void ResetOverlayCursorSession(PageLayout::Overlay overlay) noexcept;
  static ConfigureWeatherOverlayResult ConfigureWeatherOverlayPage(
    PageLayout::Overlay overlay, bool add_new_page, int rasp_field,
    std::string_view skysight_layer_id);
};

const PageLayout &
PageActions::GetActiveLayout() noexcept
{
  const PagesState &state = CommonInterface::GetUIState().pages;

  return state.special_page.IsDefined()
    ? state.special_page
    : GetConfiguredLayout();
}

void
PageActions::ClearPageOverlays() noexcept
{
  WeatherUIState &weather = CommonInterface::SetUIState().weather;
  if (!weather.rasp.IsSuspendedForPan())
    weather.map = -1;

#ifdef HAVE_EDL
  if (!weather.edl.session.IsSuspendedForPan())
    EDL::ClearOverlay();
#endif

  if (!weather.xctherm.IsSuspendedForPan()) {
#ifdef HAVE_HTTP
    XCTherm::ClearMapOverlay();
#endif
  }

#ifdef HAVE_HTTP
  if (!weather.skysight.IsSuspendedForPan())
    if (auto skysight = DataGlobals::GetSkysight(); skysight != nullptr)
      skysight->ApplyPageOverlay({});
#endif
}

void
PageActions::LeaveEdlOverlay() noexcept
{
#ifdef HAVE_EDL
  auto &weather = CommonInterface::SetUIState().weather;
  if (weather.edl.session.IsSuspendedForPan())
    return;

  weather.edl.session.LeavePage();
  EDL::ClearOverlay();
#endif
}

void
PageActions::LeaveRaspOverlay() noexcept
{
  WeatherUIState &weather = CommonInterface::SetUIState().weather;
  if (weather.rasp.IsSuspendedForPan())
    return;

  ClearPageOverlays();
  weather.rasp.LeavePage();
}

void
PageActions::LeaveXcthermOverlay() noexcept
{
  auto &xctherm = CommonInterface::SetUIState().weather.xctherm;
  if (xctherm.IsSuspendedForPan())
    return;

  xctherm.LeavePage();
#ifdef HAVE_HTTP
  XCTherm::ClearMapOverlay();
#endif
}

void
PageActions::LeaveSkysightOverlay() noexcept
{
#ifdef HAVE_HTTP
  auto &skysight_session = CommonInterface::SetUIState().weather.skysight;
  if (skysight_session.IsSuspendedForPan())
    return;

  skysight_session.LeavePage();
  if (auto skysight = DataGlobals::GetSkysight(); skysight != nullptr)
    skysight->ApplyPageOverlay({});
#endif
}

void
PageActions::LeaveWeatherOverlayPage(const PageLayout &layout) noexcept
{
  if (layout.UsesEdlOverlay())
    LeaveEdlOverlay();
  else if (layout.overlay == PageLayout::Overlay::RASP)
    LeaveRaspOverlay();
  else if (layout.UsesXcthermOverlay())
    LeaveXcthermOverlay();
  else if (layout.UsesSkysightOverlay())
    LeaveSkysightOverlay();
}

void
PageActions::ApplyRaspOverlay(const PageLayout &layout) noexcept
{
  WeatherUIState &weather = CommonInterface::SetUIState().weather;
  weather.map = Rasp::GetFieldIndex(layout);

  if (!weather.time_auto_advance)
    weather.rasp.cursor_initialized = true;

  const bool first_enter = weather.rasp.EnterPage();

  if (!weather.rasp.cursor_initialized) {
    weather.ResetRaspForDedicatedPage();
  } else if (first_enter) {
    ActionInterface::ScheduleSendUIState();
  }

#ifdef HAVE_DOWNLOAD_MANAGER
  if (!weather.rasp.cursor_initialized || first_enter)
    RequestConfiguredRaspUpdateIfOutOfDate();
#endif
}

void
PageActions::ApplyEdlOverlay() noexcept
{
#ifdef HAVE_EDL
  auto &edl = CommonInterface::SetUIState().weather.edl;
  if (!edl.forecast_auto_advance || !edl.level_auto_advance)
    edl.session.cursor_initialized = true;

  const bool first_enter = edl.session.EnterPage();

  if (!edl.session.cursor_initialized) {
    EDL::ResetForDedicatedPage();
    EDL::RequestOverlayRefresh();
  } else if (first_enter) {
    EDL::ApplyOverlayFromSession();
    EDL::RequestOverlayRefresh();
  }
#endif
}

void
PageActions::ApplyXcthermOverlay() noexcept
{
  auto &xctherm = CommonInterface::SetUIState().weather.xctherm;
  const bool first_enter = xctherm.EnterPage();
#ifdef HAVE_HTTP
  if (!xctherm.cursor_initialized)
    XCTherm::ActivatePageOverlay();
  else if (first_enter)
    XCTherm::ApplyCursorOverlayFromSession();
#endif
}

void
PageActions::ApplySkysightOverlay(const PageLayout &layout) noexcept
{
#ifdef HAVE_HTTP
  auto &skysight_session = CommonInterface::SetUIState().weather.skysight;
  skysight_session.EnterPage();

  if (auto skysight = DataGlobals::GetSkysight(); skysight != nullptr)
    skysight->ApplyPageOverlay(layout.skysight_overlay.c_str());
#else
  (void)layout;
#endif
}

void
PageActions::SuspendWeatherOverlaysForPan() noexcept
{
  WeatherUIState &weather = CommonInterface::SetUIState().weather;
  const PageLayout &layout = GetCurrentLayout();

  if (layout.UsesEdlOverlay())
    weather.edl.session.SuspendForPan();
  if (layout.UsesRaspOverlay())
    weather.rasp.SuspendForPan();
  if (layout.UsesXcthermOverlay())
    weather.xctherm.SuspendForPan();
  if (layout.UsesSkysightOverlay())
    weather.skysight.SuspendForPan();
}

void
PageActions::ResumeWeatherOverlaysAfterPan() noexcept
{
  WeatherUIState &weather = CommonInterface::SetUIState().weather;
  weather.edl.session.ResumeAfterPan();
  weather.rasp.ResumeAfterPan();
  weather.xctherm.ResumeAfterPan();
  weather.skysight.ResumeAfterPan();
}

void
PageActions::ApplyPageOverlay(const PageLayout &layout) noexcept
{
  ClearPageOverlays();

  switch (layout.overlay) {
  case PageLayout::Overlay::NONE:
    break;

  case PageLayout::Overlay::RASP:
    ApplyRaspOverlay(layout);
    break;

  case PageLayout::Overlay::EDL:
    ApplyEdlOverlay();
    break;

  case PageLayout::Overlay::XCTHERM:
    ApplyXcthermOverlay();
    break;

  case PageLayout::Overlay::SKYSIGHT:
    ApplySkysightOverlay(layout);
    break;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
  }

  if (layout.UsesWeatherOverlay())
    ActionInterface::SendUIState(true);
}

void
PageActions::LeavePage()
{
  PagesState &state = CommonInterface::SetUIState().pages;

  LeaveWeatherOverlayPage(GetActiveLayout());

  if (state.special_page.IsDefined())
    return;

  const GlueMapWindow *map = UIGlobals::GetMapIfActive();
  if (map != nullptr) {
    const MapSettings &map_settings = CommonInterface::GetMapSettings();
    PageState &page = state.pages[state.current_index];
    page.cruise_scale = map_settings.cruise_scale;
    page.circling_scale = map_settings.circling_scale;
    page.auto_zoom_enabled = map_settings.auto_zoom_enabled;
  }
}

void
PageActions::Restore()
{
  PageLayout &special_page = CommonInterface::SetUIState().pages.special_page;
  if (!special_page.IsDefined())
    return;

  LeaveWeatherOverlayPage(special_page);

  special_page.SetUndefined();

  LoadLayout(GetConfiguredLayout());
  RestoreMapZoom();
}

void
PageActions::RestoreMapZoom()
{
  const PagesState &state = CommonInterface::SetUIState().pages;
  if (state.special_page.IsDefined())
    return;

  const PageState &page = state.pages[state.current_index];
  const PageSettings &settings = CommonInterface::GetUISettings().pages;

  if (settings.distinct_zoom) {
    MapSettings &map_settings = CommonInterface::SetMapSettings();

    if (page.cruise_scale > 0)
      map_settings.cruise_scale = page.cruise_scale;
    if (page.circling_scale > 0)
      map_settings.circling_scale = page.circling_scale;

    map_settings.auto_zoom_enabled = page.auto_zoom_enabled;

    GlueMapWindow *map = UIGlobals::GetMapIfActive();
    if (map != nullptr) {
      map->RestoreMapScale();
      map->QuickRedraw();
    }
  }
}

const PageLayout &
PageActions::GetConfiguredLayout()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const PagesState &state = CommonInterface::GetUIState().pages;

  return settings.pages[state.current_index];
}

const PageLayout &
PageActions::GetCurrentLayout()
{
  const PagesState &state = CommonInterface::GetUIState().pages;

  return state.special_page.IsDefined()
    ? state.special_page
    : GetConfiguredLayout();
}

bool
PageActions::IsStuckPanFullScreenLayout() noexcept
{
  const PagesState &state = CommonInterface::GetUIState().pages;

  return state.special_page.IsDefined() &&
    state.special_page == PageLayout::FullScreen() &&
    GetConfiguredLayout() != state.special_page;
}

void
PageActions::Update()
{
  /* While panning, GetCurrentLayout() is the transient FullScreen page.
     LoadLayout() calls DisablePan() without Restore(), which would leave
     the UI stuck on FullScreen without the configured bottom widget. */
  if (IsPanning())
    return;

  if (IsStuckPanFullScreenLayout()) {
    Restore();
    return;
  }

  LoadLayout(GetCurrentLayout());
}

void
PageActions::ScheduleUpdate() noexcept
{
  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->SchedulePageActionsUpdate();
}


unsigned
PageActions::NextIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const PagesState &state = CommonInterface::GetUIState().pages;

  if (state.special_page.IsDefined())
    /* if a "special" page is active, any page switch will return to
       the last configured page */
    return state.current_index;

  return (state.current_index + 1) % settings.n_pages;
}


void
PageActions::Next()
{
  LeavePage();

  PagesState &state = CommonInterface::SetUIState().pages;

  state.current_index = NextIndex();
  state.special_page.SetUndefined();

  Update();
  RestoreMapZoom();
}

unsigned
PageActions::PrevIndex()
{
  const PageSettings &settings = CommonInterface::GetUISettings().pages;
  const PagesState &state = CommonInterface::GetUIState().pages;

  if (state.special_page.IsDefined())
    /* if a "special" page is active, any page switch will return to
       the last configured page */
    return state.current_index;

  return (state.current_index + settings.n_pages - 1)
    % settings.n_pages;
}


void
PageActions::Prev()
{
  LeavePage();

  PagesState &state = CommonInterface::SetUIState().pages;

  state.current_index = PrevIndex();
  state.special_page.SetUndefined();

  Update();
  RestoreMapZoom();
}

static void
LoadMain(PageLayout::Main main)
{
  switch (main) {
  case PageLayout::Main::MAP:
  case PageLayout::Main::MAP_NORTH_UP:
  case PageLayout::Main::EDL_MAP:
    CommonInterface::main_window->ActivateMap();
    break;

  case PageLayout::Main::FLARM_RADAR:
    UIActions::ShowTrafficRadar();
    break;

  case PageLayout::Main::THERMAL_ASSISTANT:
    UIActions::ShowThermalAssistant();
    break;

  case PageLayout::Main::HORIZON:
    UIActions::ShowHorizon();
    break;

  case PageLayout::Main::MAX:
    gcc_unreachable();
  }
}

static void
LoadBottom(const PageLayout &layout)
{
  /* Weather controls bottom widget is opt-in (Config → System → Pages).
     Weather overlays share WeatherMapOverlay::ControlsWidget. Same opt-in
     model as Cross Section. */
  switch (layout.bottom) {
  case PageLayout::Bottom::NOTHING:
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    CommonInterface::main_window->SetBottomWidget(new CrossSectionWidget(*data_components));
    break;

  case PageLayout::Bottom::WEATHER_CONTROLS:
    if (auto model = WeatherMapOverlay::CreateControlsModel(layout)) {
      CommonInterface::main_window->SetBottomWidget(
        new WeatherMapOverlay::ControlsWidget(std::move(model)));
      break;
    }
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CUSTOM:
    /* don't touch */
    break;

  case PageLayout::Bottom::MAX:
    gcc_unreachable();
  }
}

static void
LoadInfoBoxes(const PageLayout::InfoBoxConfig &config)
{
  UIState &ui_state = CommonInterface::SetUIState();

  if (!config.enabled) {
    CommonInterface::main_window->SetFullScreen(true);
    ui_state.auxiliary_enabled = false;
  } else {
    if (!config.auto_switch && config.panel < InfoBoxSettings::MAX_PANELS) {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = true;
      ui_state.auxiliary_index = config.panel;
    }
    else {
      CommonInterface::main_window->SetFullScreen(false);
      ui_state.auxiliary_enabled = false;
    }
  }
}

void
PageActions::LoadLayout(const PageLayout &layout)
{
  if (!layout.valid)
    return;

  PageLayout active = layout;
  active.Normalise();

  DisablePan();

  LoadInfoBoxes(active.infobox_config);
  LoadMain(active.main);
  ApplyPageOverlay(active);
  LoadBottom(active);

  if (!active.UsesWeatherOverlay() && InputEvents::IsMode("weather"))
    InputEvents::setMode(InputEvents::MODE_DEFAULT);

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState(false);
  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->ScheduleRefreshInfoBoxes();
}

void
PageActions::OpenLayout(const PageLayout &layout)
{
  LeavePage();

  PagesState &state = CommonInterface::SetUIState().pages;
  state.special_page = layout;

  LoadLayout(layout);
}


void
PageActions::DeferredRestore()
{
  CommonInterface::main_window->DeferredRestorePage();
}

void
PageActions::RestoreBottom()
{
  PageLayout &special_page = CommonInterface::SetUIState().pages.special_page;
  if (!special_page.IsDefined())
    return;

  const PageLayout &configured_page = GetConfiguredLayout();

  if (special_page.bottom != configured_page.bottom) {
    special_page.bottom = configured_page.bottom;
    if (special_page == configured_page)
      special_page.SetUndefined();
  }

  /* Always apply the configured bottom.  A prior restore may have updated
     special_page.bottom without tearing down a SetCustomBottom widget. */
  LoadBottom(configured_page);
}

GlueMapWindow *
PageActions::ShowMap()
{
  PageLayout layout = GetCurrentLayout();
  if (!layout.IsMapMain()) {
    /* not showing map currently: activate it */

    if (GetConfiguredLayout().IsMapMain())
      /* the configured page is a map page: restore it */
      Restore();
    else {
      /* generate a "special" map page based on the current page */
      layout.main = PageLayout::Main::MAP;
      layout.overlay = PageLayout::Overlay::NONE;
      OpenLayout(layout);
    }
  }

  return CommonInterface::main_window->ActivateMap();
}

GlueMapWindow *
PageActions::ShowOnlyMap()
{
  OpenLayout(PageLayout::FullScreen());
  return CommonInterface::main_window->ActivateMap();
}

void
PageActions::ShowTrafficRadar()
{
  PageLayout layout = GetCurrentLayout();
  if (layout.main == PageLayout::Main::FLARM_RADAR)
    /* already showing the traffic radar */
    return;

  if (GetConfiguredLayout().main == PageLayout::Main::FLARM_RADAR)
    /* the configured page is a traffic radar page: restore it */
    Restore();
  else {
    /* generate a "special" page based on the current page */
    layout.main = PageLayout::Main::FLARM_RADAR;
    layout.bottom = PageLayout::Bottom::NOTHING;
    OpenLayout(layout);
  }
}


void
PageActions::ShowThermalAssistant()
{
  PageLayout layout = GetCurrentLayout();
  if (layout.main == PageLayout::Main::THERMAL_ASSISTANT)
    /* already showing the traffic radar */
    return;

  if (GetConfiguredLayout().main == PageLayout::Main::THERMAL_ASSISTANT)
    /* the configured page is a traffic radar page: restore it */
    Restore();
  else {
    /* generate a "special" page based on the current page */
    layout.main = PageLayout::Main::THERMAL_ASSISTANT;
    layout.bottom = PageLayout::Bottom::NOTHING;
    OpenLayout(layout);
  }
}

void
PageActions::ShowWeatherPage()
{
#ifdef HAVE_EDL
  PageLayout layout = GetCurrentLayout();
  if (!layout.IsMapMain())
    layout.main = PageLayout::Main::MAP;
  layout.overlay = PageLayout::Overlay::EDL;
  layout.bottom = PageLayout::Bottom::WEATHER_CONTROLS;
  OpenLayout(layout);
#else
  ShowWeatherDialog("edl");
#endif
}

void
PageActions::SetCustomBottom(Widget *widget)
{
  assert(widget != nullptr);

  PagesState &state = CommonInterface::SetUIState().pages;

  state.special_page = GetCurrentLayout();
  state.special_page.bottom = PageLayout::Bottom::CUSTOM;
  CommonInterface::main_window->SetBottomWidget(widget);
}

void
PageActions::ResetOverlayCursorSession(PageLayout::Overlay overlay) noexcept
{
  auto &weather = CommonInterface::SetUIState().weather;

  switch (overlay) {
  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    break;

  case PageLayout::Overlay::RASP:
    weather.rasp.cursor_initialized = false;
    break;

  case PageLayout::Overlay::EDL:
    weather.edl.session.cursor_initialized = false;
    break;

  case PageLayout::Overlay::XCTHERM:
    weather.xctherm.cursor_initialized = false;
    break;

  case PageLayout::Overlay::SKYSIGHT:
    weather.skysight.cursor_initialized = false;
    break;
  }
}

PageActions::ConfigureWeatherOverlayResult
PageActions::ConfigureWeatherOverlayPage(PageLayout::Overlay overlay,
                                         bool add_new_page,
                                         int rasp_field,
                                         std::string_view skysight_layer_id)
{
  auto &settings = CommonInterface::SetUISettings().pages;
  auto &pages = CommonInterface::SetUIState().pages;
  if (settings.n_pages == 0)
    return ConfigureWeatherOverlayResult::NO_CONFIGURED_PAGE;

  if (pages.current_index >= settings.n_pages)
    pages.current_index = 0;

  unsigned target_page_index = pages.current_index;
  ConfigureWeatherOverlayResult result =
    ConfigureWeatherOverlayResult::APPLIED_CURRENT;
  const bool current_is_map_main =
    settings.pages[pages.current_index].IsMapMain();

  /* Preserve non-map pages: "add to page" should not rewrite a task/list page
     into a map page; create a new weather page instead. */
  if (add_new_page || !current_is_map_main) {
    const auto add_result = WeatherMapOverlay::AddWeatherOverlayPage(
      settings, pages.current_index, overlay, target_page_index, rasp_field,
      skysight_layer_id);
    if (add_result == WeatherMapOverlay::AddPageResult::PAGE_LIMIT_REACHED)
      return ConfigureWeatherOverlayResult::PAGE_LIMIT_REACHED;
    if (add_result != WeatherMapOverlay::AddPageResult::SUCCESS)
      return ConfigureWeatherOverlayResult::NO_CONFIGURED_PAGE;

    result = ConfigureWeatherOverlayResult::ADDED_PAGE;
  } else if (!WeatherMapOverlay::ApplyWeatherOverlayToPage(
               settings, pages.current_index, overlay, rasp_field,
               skysight_layer_id))
    return ConfigureWeatherOverlayResult::NO_CONFIGURED_PAGE;

  pages.current_index = target_page_index;
  pages.special_page.SetUndefined();
  ResetOverlayCursorSession(overlay);

  Profile::Save(Profile::map, settings);
  Update();

  return result;
}

PageActions::ConfigureWeatherOverlayResult
PageActions::AddWeatherOverlayToCurrentPage(PageLayout::Overlay overlay,
                                            int rasp_field,
                                            std::string_view skysight_layer_id)
{
  return ConfigureWeatherOverlayPage(overlay, false, rasp_field,
                                     skysight_layer_id);
}

PageActions::ConfigureWeatherOverlayResult
PageActions::AddWeatherOverlayToNewPage(PageLayout::Overlay overlay,
                                        int rasp_field,
                                        std::string_view skysight_layer_id)
{
  return ConfigureWeatherOverlayPage(overlay, true, rasp_field,
                                     skysight_layer_id);
}
