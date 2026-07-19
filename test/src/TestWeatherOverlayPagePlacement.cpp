// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/MapOverlay/PagePlacement.hpp"
#include "TestUtil.hpp"

using namespace WeatherMapOverlay;

static void
TestApplyToCurrentPage()
{
  PageSettings settings;
  settings.SetDefaults();

  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::RASP, 3));

  const auto &page = settings.pages[0];
  ok1(page.overlay == PageLayout::Overlay::RASP);
  ok1(page.bottom == PageLayout::Bottom::WEATHER_CONTROLS);
  ok1(page.main == PageLayout::Main::MAP);
  ok1(page.rasp_field == 3);
}

static void
TestAddNewOverlayPageFromNonMapSource()
{
  PageSettings settings;
  settings.SetDefaults();
  settings.pages[0].main = PageLayout::Main::FLARM_RADAR;
  settings.pages[0].overlay = PageLayout::Overlay::NONE;
  settings.pages[0].bottom = PageLayout::Bottom::NOTHING;
  settings.pages[0].Normalise();

  unsigned new_page_index = 0;
  const auto result = AddWeatherOverlayPage(settings,
                                            PageLayout::Overlay::XCTHERM,
                                            new_page_index);
  ok1(result == AddPageResult::SUCCESS);
  ok1(new_page_index == 2);
  ok1(settings.n_pages == 3);

  const auto &page = settings.pages[new_page_index];
  ok1(page.main == PageLayout::Main::MAP);
  ok1(page.infobox_config.enabled);
  ok1(page.overlay == PageLayout::Overlay::XCTHERM);
  ok1(page.bottom == PageLayout::Bottom::WEATHER_CONTROLS);
  ok1(page.xctherm_layer == PageLayout::XCTHERM_LAYER_AUTO);
  ok1(page.xctherm_time == PageLayout::XCTHERM_TIME_AUTO);
}

static void
TestReplaceCurrentOverlay()
{
  PageSettings settings;
  settings.SetDefaults();
  settings.pages[0].overlay = PageLayout::Overlay::EDL;
  settings.pages[0].bottom = PageLayout::Bottom::WEATHER_CONTROLS;
  settings.pages[0].Normalise();

  settings.pages[0].edl_isobar = 70000;
  settings.pages[0].edl_time = 500000;
  settings.pages[0].rasp_time = 15 * 60;
  settings.pages[0].xctherm_layer = 3;
  settings.pages[0].xctherm_time = 14;
  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::RASP, 1));
  ok1(settings.pages[0].overlay == PageLayout::Overlay::RASP);
  ok1(settings.pages[0].rasp_field == 1);
  ok1(settings.pages[0].rasp_time == 15 * 60);
  ok1(settings.pages[0].edl_isobar == 0);
  ok1(settings.pages[0].edl_time == PageLayout::EDL_TIME_AUTO);
  ok1(settings.pages[0].xctherm_layer ==
      PageLayout::XCTHERM_LAYER_AUTO);
  ok1(settings.pages[0].xctherm_time ==
      PageLayout::XCTHERM_TIME_AUTO);
  ok1(settings.pages[0].bottom == PageLayout::Bottom::WEATHER_CONTROLS);

  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::EDL));
  ok1(settings.pages[0].rasp_time == PageLayout::RASP_TIME_AUTO);
  ok1(settings.pages[0].edl_time == PageLayout::EDL_TIME_AUTO);
}

static void
TestPageLimitGuard()
{
  PageSettings settings;
  settings.SetDefaults();
  settings.n_pages = PageSettings::MAX_PAGES;
  for (unsigned i = 0; i < settings.n_pages; ++i)
    settings.pages[i] = PageLayout::Default();

  unsigned new_page_index = 0;
  const auto result = AddWeatherOverlayPage(settings,
                                            PageLayout::Overlay::EDL,
                                            new_page_index);
  ok1(result == AddPageResult::PAGE_LIMIT_REACHED);
}

static void
TestNormaliseCursorValues()
{
  auto page = PageLayout::Default();
  page.overlay = PageLayout::Overlay::XCTHERM;
  page.xctherm_layer = -2;
  page.xctherm_time = 24;
  page.Normalise();
  ok1(page.xctherm_layer == PageLayout::XCTHERM_LAYER_AUTO);
  ok1(page.xctherm_time == PageLayout::XCTHERM_TIME_AUTO);

  page.overlay = PageLayout::Overlay::EDL;
  page.edl_time = -3;
  page.Normalise();
  ok1(page.edl_time == PageLayout::EDL_TIME_AUTO);

  page.edl_time = PageLayout::EDL_TIME_NOW;
  page.edl_isobar = 70000;
  page.Normalise();
  ok1(page.edl_time == PageLayout::EDL_TIME_NOW);
  ok1(page.edl_isobar == 70000);

  page.overlay = PageLayout::Overlay::RASP;
  page.rasp_time = PageLayout::RASP_TIME_NOW;
  page.edl_time = 12345;
  page.edl_isobar = 70000;
  page.xctherm_layer = 2;
  page.xctherm_time = 10;
  page.Normalise();
  ok1(page.rasp_time == PageLayout::RASP_TIME_NOW);
  ok1(page.edl_time == PageLayout::EDL_TIME_AUTO);
  ok1(page.edl_isobar == 0);
  ok1(page.xctherm_layer == PageLayout::XCTHERM_LAYER_AUTO);
  ok1(page.xctherm_time == PageLayout::XCTHERM_TIME_AUTO);
}

static void
TestXCThermManualCursorDefaultsOnAdd()
{
  PageSettings settings;
  settings.SetDefaults();

  unsigned new_page_index = 0;
  ok1(AddWeatherOverlayPage(settings, PageLayout::Overlay::XCTHERM,
                            new_page_index) == AddPageResult::SUCCESS);
  const auto &page = settings.pages[new_page_index];
  ok1(page.xctherm_layer == PageLayout::XCTHERM_LAYER_AUTO);
  ok1(page.xctherm_time == PageLayout::XCTHERM_TIME_AUTO);

  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::EDL));
  ok1(settings.pages[0].edl_time == PageLayout::EDL_TIME_AUTO);
  ok1(settings.pages[0].edl_isobar == 0);
}

static void
TestAddPageFromDraftCopiesCursors()
{
  PageSettings settings;
  settings.SetDefaults();

  PageLayout draft = PageLayout::Default();
  ApplyWeatherOverlayToLayout(draft, PageLayout::Overlay::RASP, 4);
  draft.rasp_time = 12 * 60;

  unsigned new_page_index = 0;
  ok1(AddWeatherOverlayPageFromDraft(settings, draft, new_page_index) ==
      AddPageResult::SUCCESS);
  ok1(settings.pages[new_page_index].rasp_field == 4);
  ok1(settings.pages[new_page_index].rasp_time == 12 * 60);

  draft = PageLayout::Default();
  ApplyWeatherOverlayToLayout(draft, PageLayout::Overlay::EDL);
  draft.edl_time = 500000;
  draft.edl_isobar = 70000;
  ok1(AddWeatherOverlayPageFromDraft(settings, draft, new_page_index) ==
      AddPageResult::SUCCESS);
  ok1(settings.pages[new_page_index].edl_time == 500000);
  ok1(settings.pages[new_page_index].edl_isobar == 70000);

  draft = PageLayout::Default();
  ApplyWeatherOverlayToLayout(draft, PageLayout::Overlay::XCTHERM);
  draft.xctherm_layer = 2;
  draft.xctherm_time = 15;
  ok1(AddWeatherOverlayPageFromDraft(settings, draft, new_page_index) ==
      AddPageResult::SUCCESS);
  ok1(settings.pages[new_page_index].xctherm_layer == 2);
  ok1(settings.pages[new_page_index].xctherm_time == 15);
}

int
main()
{
  plan_tests(43 + 9);

  TestApplyToCurrentPage();
  TestAddNewOverlayPageFromNonMapSource();
  TestReplaceCurrentOverlay();
  TestPageLimitGuard();
  TestNormaliseCursorValues();
  TestXCThermManualCursorDefaultsOnAdd();
  TestAddPageFromDraftCopiesCursors();

  return exit_status();
}
