// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageActions.hpp"
#include "UIActions.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "MainWindow.hpp"
#include "CrossSection/CrossSectionWidget.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "DataGlobals.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "ActionInterface.hpp"
#ifdef HAVE_EDL
#include "Dialogs/Weather/MapOverlayControlsWidget.hpp"
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/StateController.hpp"
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

  static void ApplyPageOverlay(const PageLayout &layout) noexcept;
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
  weather.map = -1;

#ifdef HAVE_EDL
  if (!EDL::IsDedicatedPageSuspendedForPan())
    EDL::ClearOverlay();
#endif
}

void
PageActions::ApplyPageOverlay(const PageLayout &layout) noexcept
{
  ClearPageOverlays();

  switch (layout.overlay) {
  case PageLayout::Overlay::NONE:
    break;

  case PageLayout::Overlay::RASP: {
    WeatherUIState &weather = CommonInterface::SetUIState().weather;
    weather.map = -1;
    const auto rasp = DataGlobals::GetRasp();
    if (rasp != nullptr && layout.rasp_field >= 0 &&
        unsigned(layout.rasp_field) < rasp->GetItemCount())
      weather.map = layout.rasp_field;

    if (weather.EnterRaspDedicatedPage())
      weather.ResetRaspForDedicatedPage();
    break;
  }

  case PageLayout::Overlay::EDL:
#ifdef HAVE_EDL
    if (layout.UsesEdlOverlay()) {
      if (EDL::EnterDedicatedPage())
        EDL::ResetForDedicatedPage();
      else
        EDL::EnsureInitialised();

      EDL::RequestOverlayRefresh();
    }
#endif
    break;

  case PageLayout::Overlay::MAX:
    gcc_unreachable();
  }

  ActionInterface::SendUIState(true);
}

void
PageActions::LeavePage()
{
  PagesState &state = CommonInterface::SetUIState().pages;

  const PageLayout &layout = GetActiveLayout();

  if (layout.UsesEdlOverlay()) {
#ifdef HAVE_EDL
    if (!EDL::IsDedicatedPageSuspendedForPan()) {
      EDL::LeaveDedicatedPage();
      EDL::ClearOverlay();
    }
#endif
  } else if (layout.overlay == PageLayout::Overlay::RASP) {
    ClearPageOverlays();
    CommonInterface::SetUIState().weather.rasp_page_entered = false;
  }

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

  if (special_page.UsesEdlOverlay()) {
#ifdef HAVE_EDL
    if (!EDL::IsDedicatedPageSuspendedForPan()) {
      EDL::LeaveDedicatedPage();
      EDL::ClearOverlay();
    }
#endif
  } else if (special_page.overlay == PageLayout::Overlay::RASP) {
    ClearPageOverlays();
    CommonInterface::SetUIState().weather.rasp_page_entered = false;
  }

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

void
PageActions::Update()
{
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
  switch (layout.bottom) {
  case PageLayout::Bottom::NOTHING:
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    CommonInterface::main_window->SetBottomWidget(new CrossSectionWidget(*data_components));
    break;

  case PageLayout::Bottom::EDL_CONTROLS:
#ifdef HAVE_EDL
    {
      auto widget = CreateMapOverlayControlsBottomWidget(layout.overlay);
      CommonInterface::main_window->SetBottomWidget(widget.release());
    }
#else
    CommonInterface::main_window->SetBottomWidget(nullptr);
#endif
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
  if (special_page.bottom == configured_page.bottom)
    return;

  special_page.bottom = configured_page.bottom;
  if (special_page == configured_page)
    special_page.SetUndefined();

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
  layout.bottom = PageLayout::Bottom::EDL_CONTROLS;
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
