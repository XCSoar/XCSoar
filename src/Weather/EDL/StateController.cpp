// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StateController.hpp"

#include "Formatter/UserUnits.hpp"
#include "Levels.hpp"
#include "TileStore.hpp"
#include "MapWindow/MbTilesOverlay.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "PageSettings.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "system/FileUtil.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>

namespace EDL {

static BrokenDateTime last_utc_hour = BrokenDateTime::Invalid();
static bool gps_ui_refresh_pending = false;

static bool
HasDedicatedPageOverlayOwnership() noexcept
{
  const auto &state = CommonInterface::GetUIState().weather.edl;
  return state.dedicated_page_entered || state.dedicated_page_suspended_for_pan;
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
  if (!state.forecast_auto_advance)
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
    state.isobar = state.forecast_auto_advance
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
TryApplyOverlayFromCache() noexcept
{
  if (!ShouldMaintainOverlay())
    return false;

  const TileRequest request(GetForecastTime(), GetIsobar());
  const auto path = request.BuildCachePath();
  if (!File::ExistsAny(path))
    return false;

  ApplyOverlay(path);
  return true;
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

  UpdateCurrentLevel();

  const auto tracked = GetTrackedForecastTime(utc);
  state.forecast_datetime = tracked;

  TryApplyOverlayFromCache();
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

  const auto &state = CommonInterface::GetUIState().weather.edl;
  if (!state.forecast_auto_advance)
    return;

  const unsigned prev_isobar = state.isobar;
  UpdateCurrentLevel();
  const bool level_changed =
    CommonInterface::GetUIState().weather.edl.isobar != prev_isobar;

  if (hour_changed || level_changed) {
    OnTimeUpdate(utc);
    gps_ui_refresh_pending = true;
  }
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

void
ResetForDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;

  /* On first entry after leaving an EDL page, resync only when auto
     advance is enabled; manual forecast/level choices are kept. */
  if (state.forecast_auto_advance) {
    state.forecast_datetime =
      GetTrackedForecastTime(BrokenDateTime::NowUTC());
    UpdateCurrentLevel();
  }

  state.status = EDLWeatherUIState::Status::IDLE;
}

void
LeaveDedicatedPage() noexcept
{
  CommonInterface::SetUIState().weather.edl.dedicated_page_entered = false;
}

bool
EnterDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  if (state.dedicated_page_entered)
    return false;

  state.dedicated_page_entered = true;
  return true;
}

void
SuspendDedicatedPageForPan() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  state.dedicated_page_suspended_for_pan = state.dedicated_page_entered;
}

void
ResumeDedicatedPageAfterPan() noexcept
{
  CommonInterface::SetUIState().weather.edl.dedicated_page_suspended_for_pan = false;
}

bool
IsDedicatedPageSuspendedForPan() noexcept
{
  return CommonInterface::GetUIState().weather.edl.dedicated_page_suspended_for_pan;
}

void
ClearOverlay() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map == nullptr || HasDedicatedPageOverlayOwnership())
    return;

  if (map->GetOverlay() == nullptr)
    return;

  map->SetOverlay(nullptr);
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

void
ApplyOverlay(Path path)
{
  auto &state = CommonInterface::SetUIState().weather.edl;
  /* Reuse the generic overlay HUD by exposing the active EDL state as
     the overlay label instead of showing this mapping only in a widget. */
  const auto label = GetOverlayLabel();
  auto overlay = std::make_unique<MbTilesOverlay>(path, label.c_str());

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
