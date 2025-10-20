// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PageActions.hpp"
#include "UIActions.hpp"
#include "UIState.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "MainWindow.hpp"
#include "CrossSection/CrossSectionWidget.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Pan.hpp"
#include "UIGlobals.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"

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
};

void
PageActions::LeavePage()
{
  PagesState &state = CommonInterface::SetUIState().pages;

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
LoadBottom(PageLayout::Bottom bottom)
{
  switch (bottom) {
  case PageLayout::Bottom::NOTHING:
    CommonInterface::main_window->SetBottomWidget(nullptr);
    break;

  case PageLayout::Bottom::CROSS_SECTION:
    CommonInterface::main_window->SetBottomWidget(new CrossSectionWidget(*data_components));
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

  DisablePan();

  LoadInfoBoxes(layout.infobox_config);
  LoadBottom(layout.bottom);
  LoadMain(layout.main);

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();
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
PageActions::Restore()
{
  PageLayout &special_page = CommonInterface::SetUIState().pages.special_page;
  if (!special_page.IsDefined())
    return;

  special_page.SetUndefined();

  LoadLayout(GetConfiguredLayout());
  RestoreMapZoom();
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

  LoadBottom(configured_page.bottom);
}

GlueMapWindow *
PageActions::ShowMap()
{
  PageLayout layout = GetCurrentLayout();
  if (layout.main != PageLayout::Main::MAP && layout.main != PageLayout::Main::MAP_NORTH_UP) {
    /* not showing map currently: activate it */

    if (GetConfiguredLayout().main == PageLayout::Main::MAP || GetConfiguredLayout().main == PageLayout::Main::MAP_NORTH_UP)
      /* the configured page is a map page: restore it */
      Restore();
    else {
      /* generate a "special" map page based on the current page */
      layout.main = PageLayout::Main::MAP;
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
PageActions::SetCustomBottom(Widget *widget)
{
  assert(widget != nullptr);

  PagesState &state = CommonInterface::SetUIState().pages;

  state.special_page = GetCurrentLayout();
  state.special_page.bottom = PageLayout::Bottom::CUSTOM;
  CommonInterface::main_window->SetBottomWidget(widget);
}
