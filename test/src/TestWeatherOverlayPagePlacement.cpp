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
  const auto result = AddWeatherOverlayPage(settings, 0,
                                            PageLayout::Overlay::XCTHERM,
                                            new_page_index);
  ok1(result == AddPageResult::SUCCESS);
  ok1(new_page_index == 2);
  ok1(settings.n_pages == 3);

  const auto &page = settings.pages[new_page_index];
  ok1(page.main == PageLayout::Main::MAP);
  ok1(page.overlay == PageLayout::Overlay::XCTHERM);
  ok1(page.bottom == PageLayout::Bottom::WEATHER_CONTROLS);
}

static void
TestReplaceCurrentOverlay()
{
  PageSettings settings;
  settings.SetDefaults();
  settings.pages[0].overlay = PageLayout::Overlay::EDL;
  settings.pages[0].bottom = PageLayout::Bottom::WEATHER_CONTROLS;
  settings.pages[0].Normalise();

  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::RASP, 1));
  ok1(settings.pages[0].overlay == PageLayout::Overlay::RASP);
  ok1(settings.pages[0].rasp_field == 1);
  ok1(settings.pages[0].bottom == PageLayout::Bottom::WEATHER_CONTROLS);
}

static void
TestApplySkySightOverlay()
{
  PageSettings settings;
  settings.SetDefaults();

  ok1(ApplyWeatherOverlayToPage(settings, 0, PageLayout::Overlay::SKYSIGHT,
                                -1, "wind_925"));
  ok1(settings.pages[0].overlay == PageLayout::Overlay::SKYSIGHT);
  ok1(settings.pages[0].skysight_overlay == "wind_925");
  ok1(settings.pages[0].bottom == PageLayout::Bottom::WEATHER_CONTROLS);
  ok1(settings.pages[0].rasp_field == -1);
}

static void
TestChangeSkySightPageLayer()
{
  PageSettings settings;
  settings.SetDefaults();

  ok1(!SetSkySightLayerOnPage(settings, 0, "thermal"));
  ok1(ApplyWeatherOverlayToPage(settings, 0,
                                PageLayout::Overlay::SKYSIGHT,
                                -1, "wind"));
  ok1(SetSkySightLayerOnPage(settings, 0, "thermal"));
  ok1(settings.pages[0].skysight_overlay == "thermal");
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
  const auto result = AddWeatherOverlayPage(settings, 0,
                                            PageLayout::Overlay::EDL,
                                            new_page_index);
  ok1(result == AddPageResult::PAGE_LIMIT_REACHED);
}

static void
TestInvalidSourcePage()
{
  PageSettings settings;
  settings.SetDefaults();

  unsigned new_page_index = 0;
  const auto result = AddWeatherOverlayPage(settings, settings.n_pages,
                                            PageLayout::Overlay::EDL,
                                            new_page_index);
  ok1(result == AddPageResult::INVALID_SOURCE_PAGE);
}

int
main()
{
  plan_tests(22 + 4);

  TestApplyToCurrentPage();
  TestAddNewOverlayPageFromNonMapSource();
  TestReplaceCurrentOverlay();
  TestApplySkySightOverlay();
  TestChangeSkySightPageLayer();
  TestPageLimitGuard();
  TestInvalidSourcePage();

  return exit_status();
}
