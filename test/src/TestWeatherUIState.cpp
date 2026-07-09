// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Weather/WeatherUIState.hpp"
#include "TestUtil.hpp"

static void
TestOverlaySession()
{
  OverlaySession session;
  session.Clear();

  ok1(!session.page_entered);
  ok1(!session.suspended_for_pan);
  ok1(!session.cursor_initialized);
  ok1(!session.HasPageOwnership());

  ok1(session.EnterPage());
  ok1(!session.EnterPage());
  ok1(session.page_entered);
  ok1(session.HasPageOwnership());

  session.SuspendForPan();
  ok1(session.IsSuspendedForPan());
  ok1(session.HasPageOwnership());

  session.LeavePage();
  ok1(!session.page_entered);
  ok1(session.HasPageOwnership());

  session.ResumeAfterPan();
  ok1(!session.IsSuspendedForPan());
  ok1(!session.HasPageOwnership());

  session.SuspendForPan();
  ok1(!session.IsSuspendedForPan());
}

static void
TestWeatherUiStateRaspReset()
{
  WeatherUIState weather;
  weather.Clear();

  ok1(weather.map == -1);
  ok1(weather.time_auto_advance);
  ok1(!weather.time.IsPlausible());
  ok1(!weather.rasp.cursor_initialized);

  weather.time_auto_advance = false;
  weather.time = BrokenTime(12, 30);
  weather.ResetRaspForDedicatedPage();
  ok1(weather.time == BrokenTime(12, 30));

  weather.time_auto_advance = true;
  weather.ResetRaspForDedicatedPage();
  ok1(!weather.time.IsPlausible());
}

static void
TestWeatherUiStateXcthermCursor()
{
  WeatherUIState weather;
  weather.Clear();

  ok1(!weather.xctherm.cursor_initialized);
  ok1(weather.xctherm_cursor.layer == 0);
  ok1(weather.xctherm_cursor.forecast_utc_hour == 12);
  ok1(!weather.xctherm_cursor.altitude_manual_override);
  ok1(!weather.xctherm_cursor.time_manual_override);

  weather.xctherm.EnterPage();
  weather.xctherm.SuspendForPan();
  weather.xctherm.cursor_initialized = true;
  weather.xctherm_cursor.layer = 7;
  weather.xctherm_cursor.forecast_utc_hour = 19;
  weather.xctherm_cursor.altitude_manual_override = true;
  weather.xctherm_cursor.time_manual_override = true;

  weather.Clear();
  ok1(!weather.xctherm.HasPageOwnership());
  ok1(!weather.xctherm.cursor_initialized);
  ok1(weather.xctherm_cursor.layer == 0);
  ok1(weather.xctherm_cursor.forecast_utc_hour == 12);
  ok1(!weather.xctherm_cursor.altitude_manual_override);
  ok1(!weather.xctherm_cursor.time_manual_override);
}

int
main()
{
  plan_tests(32);

  TestOverlaySession();
  TestWeatherUiStateRaspReset();
  TestWeatherUiStateXcthermCursor();

  return exit_status();
}
