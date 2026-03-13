// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Manager.hpp"

#include "Download.hpp"
#include "LevelResolver.hpp"
#include "MbTilesOverlay.hpp"
#include "Request.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "time/Convert.hxx"

#include <algorithm>
#include <cmath>
#include <memory>

namespace EDL {

static BrokenDateTime
ToLocalBrokenDateTime(BrokenDateTime utc) noexcept
{
  const auto tm = LocalTime(utc.ToTimePoint());
  return BrokenDateTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static int
GetCurrentAircraftAltitude() noexcept
{
  const auto &basic = CommonInterface::Basic();

  /* The dedicated page should follow the aircraft as closely as the
     available sensors allow, preferring barometric altitude. */
  if (basic.baro_altitude_available)
    return std::max(0, int(std::lround(basic.baro_altitude)));

  if (basic.gps_altitude_available)
    return std::max(0, int(std::lround(basic.gps_altitude)));

  return 3000;
}

static void
ResolveCurrentLevel() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  const auto resolved = ResolveLevel(CommonInterface::GetComputerSettings().pressure,
                                     CommonInterface::GetComputerSettings().pressure_available,
                                     state.edl_altitude);
  state.edl_isobar = resolved.isobar;
}

void
EnsureInitialised() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;

  /* Overlay mode may enter through the weather dialog without visiting
     the dedicated page first, so make the shared state self-healing. */
  if (!state.forecast_datetime.IsPlausible()) {
    const auto now = BrokenDateTime::NowUTC();
    state.forecast_datetime = BrokenDateTime(now.year, now.month, now.day,
                                             now.hour, 0, 0);
  }

  if (!IsSupportedIsobar(state.edl_isobar))
    ResolveCurrentLevel();

  if (CommonInterface::GetComputerSettings().weather.enable_edl &&
      state.edl_status == WeatherUIState::EDLStatus::DISABLED)
    state.edl_status = WeatherUIState::EDLStatus::IDLE;
}

void
ResetForDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  const auto now = BrokenDateTime::NowUTC();
  /* Dedicated EDL pages always start from the current hour/current
     aircraft altitude instead of preserving a stale previous view. */
  state.forecast_datetime = BrokenDateTime(now.year, now.month, now.day,
                                           now.hour, 0, 0);
  state.edl_altitude = GetCurrentAircraftAltitude();
  ResolveCurrentLevel();

  if (CommonInterface::GetComputerSettings().weather.enable_edl)
    state.edl_status = WeatherUIState::EDLStatus::IDLE;
  else
    state.edl_status = WeatherUIState::EDLStatus::DISABLED;
}

void
StepForecast(std::chrono::hours delta) noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  state.forecast_datetime = state.forecast_datetime + delta;
}

void
SelectIsobar(unsigned isobar) noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  state.edl_isobar = isobar;
  state.edl_altitude = GetAltitudeForIsobar(isobar);
}

bool
ShouldShowOnMainMap() noexcept
{
  return CommonInterface::GetComputerSettings().weather.show_edl_on_main_map;
}

void
SetShowOnMainMap(bool enabled) noexcept
{
  CommonInterface::SetComputerSettings().weather.show_edl_on_main_map = enabled;
  Profile::Set(ProfileKeys::ShowEDLOnMainMap, enabled);
}

void
ClearOverlay() noexcept
{
  if (auto *map = UIGlobals::GetMap())
    map->SetOverlay(nullptr);
}

bool
OverlayEnabled() noexcept
{
  return CommonInterface::GetComputerSettings().weather.enable_edl;
}

bool
OverlayVisible() noexcept
{
  const auto *map = UIGlobals::GetMap();
  return map != nullptr &&
    dynamic_cast<const MbTilesOverlay *>(map->GetOverlay()) != nullptr;
}

void
SetLoadingStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  if (!OverlayEnabled()) {
    state.edl_status = WeatherUIState::EDLStatus::DISABLED;
    return;
  }

  state.edl_status = WeatherUIState::EDLStatus::LOADING;
}

void
SetIdleStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  state.edl_status = OverlayEnabled()
    ? WeatherUIState::EDLStatus::IDLE
    : WeatherUIState::EDLStatus::DISABLED;
}

void
SetErrorStatus() noexcept
{
  CommonInterface::SetUIState().weather.edl_status = WeatherUIState::EDLStatus::ERROR;
}

void
ApplyOverlay(Path path) noexcept
{
  auto &state = CommonInterface::SetUIState().weather;
  /* Reuse the generic overlay HUD by exposing the active EDL state as
     the overlay label instead of showing this mapping only in a widget. */
  const auto label = GetOverlayLabel();
  auto overlay = std::make_unique<MbTilesOverlay>(path, label.c_str());

  if (auto *map = UIGlobals::GetMap()) {
    map->SetOverlay(std::move(overlay));
    map->QuickRedraw();
  }

  state.edl_enabled = true;
  state.edl_status = WeatherUIState::EDLStatus::READY;
}

void
RefreshOverlayVisibility() noexcept
{
  if (ShouldShowOnMainMap())
    return;

  ClearOverlay();
}

const char *
GetStatusLabel() noexcept
{
  switch (CommonInterface::GetUIState().weather.edl_status) {
  case WeatherUIState::EDLStatus::DISABLED:
    return _("Disabled");
  case WeatherUIState::EDLStatus::IDLE:
    return _("Idle");
  case WeatherUIState::EDLStatus::LOADING:
    return _("Loading");
  case WeatherUIState::EDLStatus::READY:
    return _("Ready");
  case WeatherUIState::EDLStatus::ERROR:
    return _("Error");
  }

  return _("Error");
}

BrokenDateTime
GetForecastTime() noexcept
{
  return CommonInterface::GetUIState().weather.forecast_datetime;
}

BrokenDateTime
GetForecastTimeLocal() noexcept
{
  EnsureInitialised();
  return ToLocalBrokenDateTime(GetForecastTime());
}

BrokenDateTime
ToLocalForecastTime(BrokenDateTime forecast) noexcept
{
  return ToLocalBrokenDateTime(forecast);
}

StaticString<64>
GetOverlayLabel() noexcept
{
  EnsureInitialised();

  const auto local = GetForecastTimeLocal();

  StaticString<64> label;
  label.Format("EDL %02u:00 %u hPa %d m",
               unsigned(local.hour),
               GetIsobar() / 100,
               GetAltitude());
  return label;
}

int
GetAltitude() noexcept
{
  return CommonInterface::GetUIState().weather.edl_altitude;
}

unsigned
GetIsobar() noexcept
{
  return CommonInterface::GetUIState().weather.edl_isobar;
}

int
GetAltitudeForIsobar(unsigned isobar) noexcept
{
  const auto pressure = AtmosphericPressure::Pascal(isobar);
  const auto &qnh = CommonInterface::GetComputerSettings().pressure;
  const double altitude =
    CommonInterface::GetComputerSettings().pressure_available && qnh.IsPlausible()
    ? qnh.StaticPressureToQNHAltitude(pressure)
    : AtmosphericPressure::StaticPressureToPressureAltitude(pressure);
  return std::max(0, int(std::lround(altitude)));
}

} // namespace EDL
