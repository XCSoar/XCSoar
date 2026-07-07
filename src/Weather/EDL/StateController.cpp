// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StateController.hpp"

#include "Formatter/UserUnits.hpp"
#include "Levels.hpp"
#include "TileStore.hpp"
#include "MapWindow/MbTilesOverlay.hpp"
#include "Weather/EDL/EdlMbTilesOverlay.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "PageSettings.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>

namespace EDL {

static BrokenDateTime last_utc_hour = BrokenDateTime::Invalid();
static bool gps_ui_refresh_pending = false;

/** Cache file name of the overlay currently on the map (basename only). */
static StaticString<64> active_overlay_file;

static bool
MapShowsCachedOverlay(const TileRequest &request) noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return false;

  if (dynamic_cast<const MbTilesOverlay *>(map->GetOverlay()) == nullptr)
    return false;

  return active_overlay_file == request.BuildCacheFileName();
}

static bool
HasDedicatedPageOverlayOwnership() noexcept
{
  return CommonInterface::GetUIState().weather.edl.session.HasPageOwnership();
}

static const PageLayout &
GetActivePageLayout() noexcept
{
  const PagesState &pages = CommonInterface::GetUIState().pages;
  return pages.special_page.IsDefined()
    ? pages.special_page
    : CommonInterface::GetUISettings().pages.pages[pages.current_index];
}

bool
OverlayVisible() noexcept
{
  const auto *map = UIGlobals::GetMap();
  return map != nullptr &&
    dynamic_cast<const MbTilesOverlay *>(map->GetOverlay()) != nullptr;
}

bool
OverlayEnabled() noexcept
{
  if (OverlayVisible())
    return true;

  if (HasDedicatedPageOverlayOwnership())
    return true;

  return GetActivePageLayout().UsesEdlOverlay();
}

void
UpdateCurrentLevel() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  if (!state.level_auto_advance)
    return;

  state.isobar = ResolveLevelBelow();
}

void
EnsureInitialised() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;

  /* Overlay mode may enter through the weather dialog without visiting
     the dedicated page first, so make the shared state self-healing. */
  if (!state.forecast_datetime.IsPlausible())
    state.forecast_datetime =
      GetTrackedForecastTime(BrokenDateTime::NowUTC());

  if (!IsSupportedIsobar(state.isobar))
    state.isobar = state.level_auto_advance
      ? ResolveLevelBelow()
      : ResolveCurrentLevel();

  if (OverlayEnabled() &&
      state.status == EDLWeatherUIState::Status::DISABLED)
    state.status = EDLWeatherUIState::Status::IDLE;
}

bool
ShouldMaintainOverlay() noexcept
{
  return OverlayEnabled();
}

bool
HasOverlayCache() noexcept
{
  EnsureInitialised();

  const TileRequest request(GetForecastTime(), GetIsobar());
  return File::ExistsAny(request.BuildCachePath());
}

bool
TryApplyOverlayFromCache() noexcept
{
  if (!ShouldMaintainOverlay())
    return false;

  const TileRequest request(GetForecastTime(), GetIsobar());
  const auto path = request.BuildCachePath();
  if (!File::ExistsAny(path))
    return false;

  if (MapShowsCachedOverlay(request))
    return true;

  return TryApplyOverlay(path);
}

void
ApplyOverlayFromSession() noexcept
{
  if (TryApplyOverlayFromCache())
    return;

  ClearOverlay();
}

void
OnTimeUpdate(BrokenDateTime utc) noexcept
{
  if (!OverlayEnabled() || !utc.IsPlausible())
    return;

  if (!ShouldMaintainOverlay())
    return;

  auto &state = CommonInterface::SetUIState().weather.edl;
  if (!state.forecast_auto_advance)
    return;

  if (state.level_auto_advance)
    UpdateCurrentLevel();

  const auto tracked = GetTrackedForecastTime(utc);
  state.forecast_datetime = tracked;

  ApplyOverlayFromSession();
}

void
ProcessGpsUpdate(BrokenDateTime utc) noexcept
{
  if (!utc.IsPlausible())
    return;

  const auto hour = utc.FloorToHour();
  const bool hour_changed = hour != last_utc_hour;

  if (hour_changed)
    last_utc_hour = hour;

  auto &state = CommonInterface::SetUIState().weather.edl;
  bool changed = false;

  if (state.level_auto_advance) {
    const unsigned prev_isobar = state.isobar;
    UpdateCurrentLevel();
    if (state.isobar != prev_isobar)
      changed = true;
  }

  if (state.forecast_auto_advance && hour_changed) {
    const auto tracked = GetTrackedForecastTime(utc);
    if (state.forecast_datetime != tracked) {
      state.forecast_datetime = tracked;
      changed = true;
    }
  }

  if (!changed)
    return;

  ApplyOverlayFromSession();
  gps_ui_refresh_pending = true;
}

bool
TakeGpsUiRefreshPending() noexcept
{
  const bool pending = gps_ui_refresh_pending;
  gps_ui_refresh_pending = false;
  return pending;
}

void
ClearGpsUiRefreshPending() noexcept
{
  gps_ui_refresh_pending = false;
}

static BrokenDateTime
ReferenceUtc() noexcept
{
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    return basic.date_time_utc;

  return BrokenDateTime::NowUTC();
}

void
ResetForDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;

  /* On first entry after leaving an EDL page, resync only when auto
     advance is enabled; manual forecast/level choices are kept. */
  if (state.forecast_auto_advance) {
    state.forecast_datetime = GetTrackedForecastTime(ReferenceUtc());
  }

  if (state.level_auto_advance)
    UpdateCurrentLevel();

  state.status = EDLWeatherUIState::Status::IDLE;
}

void
ClearOverlay() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  if (dynamic_cast<const MbTilesOverlay *>(map->GetOverlay()) == nullptr)
    return;

  map->SetOverlay(nullptr);
  active_overlay_file.clear();

  auto &state = CommonInterface::SetUIState().weather.edl;
  state.enabled = false;
  if (state.status == EDLWeatherUIState::Status::READY)
    state.status = EDLWeatherUIState::Status::IDLE;
}

void
SetLoadingStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  if (!OverlayEnabled()) {
    state.status = EDLWeatherUIState::Status::DISABLED;
    return;
  }

  state.status = EDLWeatherUIState::Status::LOADING;
}

void
SetIdleStatus() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  state.status = OverlayEnabled()
    ? EDLWeatherUIState::Status::IDLE
    : EDLWeatherUIState::Status::DISABLED;
}

void
SetErrorStatus() noexcept
{
  CommonInterface::SetUIState().weather.edl.status = EDLWeatherUIState::Status::FAILED;
}

bool
TryApplyOverlay(Path path) noexcept
{
  try {
    ApplyOverlay(path);
    return true;
  } catch (...) {
    LogError(std::current_exception(), "EDL overlay apply failed");
    SetErrorStatus();
    return false;
  }
}

void
ApplyOverlay(Path path)
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  active_overlay_file = path.GetBase().c_str();
  /* Reuse the generic overlay HUD by exposing the active EDL state as
     the overlay label instead of showing this mapping only in a widget. */
  const auto label = GetOverlayLabel();
  auto overlay = std::make_unique<EdlMbTilesOverlay>(path, label.c_str());

  if (auto *map = UIGlobals::GetMap()) {
    map->SetOverlay(std::move(overlay));
    map->QuickRedraw();
  }

  state.enabled = true;
  state.status = EDLWeatherUIState::Status::READY;
}

void
RefreshOverlayVisibility() noexcept
{
  if (OverlayEnabled())
    return;

  ClearOverlay();
}

const char *
GetStatusLabel() noexcept
{
  switch (CommonInterface::GetUIState().weather.edl.status) {
  case EDLWeatherUIState::Status::DISABLED:
    return _("Disabled");
  case EDLWeatherUIState::Status::IDLE:
    return _("Idle");
  case EDLWeatherUIState::Status::LOADING:
    return _("Loading");
  case EDLWeatherUIState::Status::READY:
    return _("Ready");
  case EDLWeatherUIState::Status::FAILED:
    return _("Error");
  }

  gcc_unreachable();
}

BrokenDateTime
GetForecastTime() noexcept
{
  return CommonInterface::GetUIState().weather.edl.forecast_datetime;
}

BrokenDateTime
GetForecastTimeLocal() noexcept
{
  EnsureInitialised();
  return GetForecastTime().ToLocal();
}

void
FormatForecastCursorLabel(StaticString<64> &text,
                            bool auto_advance) noexcept
{
  EnsureInitialised();

  const auto forecast = GetForecastTime().FloorToHour();
  BrokenDateTime now = BrokenDateTime::NowUTC();
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    now = basic.date_time_utc;

  char offset_buf[16] = {};
  if (forecast.IsPlausible() && now.IsPlausible()) {
    const int offset_min =
      (int)std::chrono::duration_cast<std::chrono::minutes>(
        forecast - now).count();
    WeatherMapOverlay::FormatSignedMinuteOffset(offset_buf, sizeof(offset_buf),
                                         offset_min);
  }

  WeatherMapOverlay::FormatAutoUtcHourLabel(text, auto_advance,
                                     unsigned(forecast.hour), offset_buf);
}

StaticString<64>
GetOverlayLabel() noexcept
{
  EnsureInitialised();

  const auto local = GetForecastTimeLocal();

  char buffer[64];
  FormatUserAltitude(GetAltitude(), buffer);

  StaticString<64> label;
  label.Format(_("EDL %02u:00 %u hPa %s"),
               unsigned(local.hour),
               GetIsobar() / 100,
               buffer);

#ifdef ENABLE_OPENGL
  if (auto *map = UIGlobals::GetMap()) {
    const GeoPoint location = map->GetLocation();
    if (location.IsValid()) {
      const auto *overlay =
        dynamic_cast<const EdlMbTilesOverlay *>(map->GetOverlay());
      if (overlay != nullptr) {
        double value_mps = 0;
        if (overlay->SampleAscendancyAt(location, value_mps)) {
          StaticString<64> with_value;
          with_value.Format(_("%s %+.1f m/s"), label.c_str(), value_mps);
          return with_value;
        }
      }
    }
  }
#endif

  return label;
}

int
GetAltitude() noexcept
{
  return GetAltitudeForIsobar(GetIsobar());
}

unsigned
GetIsobar() noexcept
{
  return CommonInterface::GetUIState().weather.edl.isobar;
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
