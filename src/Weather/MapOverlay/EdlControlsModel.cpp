// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsModel.hpp"

#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"
#include "UIState.hpp"
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"

#include <chrono>

namespace WeatherMapOverlay {

void
EdlControlsModel::OnShow() noexcept
{
  EDL::EnsureInitialised();

  if (!PageActions::GetCurrentLayout().UsesEdlOverlay())
    return;

  EDL::ApplyOverlayFromSession();
}

void
EdlControlsModel::SelectForecast(unsigned index) noexcept
{
  if (index >= forecast_times.size())
    return;

  EDL::EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.forecast_datetime = forecast_times[index];
  edl.forecast_auto_advance = false;
  edl.cursor_session_initialized = true;
}

void
EdlControlsModel::RebuildForecastTimes() noexcept
{
  EDL::EnsureInitialised();

  auto selected_time = EDL::GetForecastTime();
  if (!selected_time.IsPlausible())
    selected_time = EDL::GetTrackedForecastTime(BrokenDateTime::NowUTC());

  if (!selected_time.IsPlausible())
    return;

  const auto base_time = selected_time + std::chrono::hours{-11};
  for (unsigned i = 0; i < forecast_choices; ++i)
    forecast_times[i] = base_time + std::chrono::hours{i};
}

unsigned
EdlControlsModel::FindForecastIndex() const noexcept
{
  const auto selected = EDL::GetForecastTime();
  for (unsigned i = 0; i < forecast_choices; ++i)
    if (forecast_times[i] == selected)
      return i;

  return 0;
}

bool
EdlControlsModel::StepForecast(int delta) noexcept
{
  RebuildForecastTimes();

  const int index = int(FindForecastIndex()) + delta;
  if (index < 0 || index >= int(forecast_choices))
    return false;

  SelectForecast(unsigned(index));
  return true;
}

bool
EdlControlsModel::StepLevel(int delta) noexcept
{
  EDL::EnsureInitialised();

  const unsigned current = EDL::GetIsobar();
  int index = -1;
  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
    if (EDL::ISOBARS[i] == current) {
      index = int(i);
      break;
    }
  }

  if (index < 0)
    index = 0;

  /* ISOBARS are ascending pressure (descending altitude); invert delta so
     "<" steps down and ">" steps up in height. */
  const int new_index = index - delta;
  if (new_index < 0 || new_index >= int(EDL::NUM_ISOBARS))
    return false;

  SelectLevel(EDL::ISOBARS[unsigned(new_index)]);
  return true;
}

void
EdlControlsModel::ResumeAutoAdvance() noexcept
{
  SetForecastAutoAdvance(true);
  SetLevelAutoAdvance(true);
  EDL::UpdateCurrentLevel();

  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    EDL::OnTimeUpdate(basic.date_time_utc);
  else
    EDL::OnTimeUpdate(BrokenDateTime::NowUTC());
}

void
EdlControlsModel::FormatForecastLabel(StaticString<64> &text) const noexcept
{
  StaticString<64> base;
  EDL::FormatForecastCursorLabel(base, GetForecastAutoAdvance());

  if (HasOverlayData())
    text = base;
  else
    WeatherMapOverlay::AppendNoDataTag(text, base.c_str());
}

void
EdlControlsModel::FormatLevelLabel(StaticString<64> &text) const noexcept
{
  const unsigned isobar = EDL::GetIsobar();
  const int altitude = EDL::GetAltitudeForIsobar(isobar);

  StaticString<64> base;
  if (GetLevelAutoAdvance())
    base.Format("%s %u hPa (%d m)", _("AUTO:"), isobar / 100, altitude);
  else
    base.Format(_("%u hPa (%d m)"), isobar / 100, altitude);

  if (HasOverlayData())
    text = base;
  else
    WeatherMapOverlay::AppendNoDataTag(text, base.c_str());
}

bool
EdlControlsModel::HasOverlayData() const noexcept
{
  return EDL::HasOverlayCache();
}

bool
EdlControlsModel::GetForecastAutoAdvance() const noexcept
{
  return CommonInterface::GetUIState().weather.edl.forecast_auto_advance;
}

void
EdlControlsModel::SetForecastAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.edl.forecast_auto_advance =
    auto_advance;
}

bool
EdlControlsModel::GetLevelAutoAdvance() const noexcept
{
  return CommonInterface::GetUIState().weather.edl.level_auto_advance;
}

void
EdlControlsModel::SetLevelAutoAdvance(bool auto_advance) noexcept
{
  CommonInterface::SetUIState().weather.edl.level_auto_advance =
    auto_advance;
}

void
EdlControlsModel::SelectLevel(unsigned isobar) noexcept
{
  EDL::EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.SelectIsobar(isobar);
  edl.level_auto_advance = false;
  edl.cursor_session_initialized = true;
}

unsigned
EdlControlsModel::SelectedCachedDayIndex(const std::vector<EDL::CachedDay> &days) const noexcept
{
  if (days.empty())
    return 0;

  const auto current_day = EDL::GetForecastTime().AtMidnight();
  for (unsigned i = 0; i < days.size(); ++i)
    if (days[i].day == current_day)
      return i;

  return 0;
}

StaticString<40>
EdlControlsModel::FormatCachedDayLabel(const EDL::CachedDay &day) const noexcept
{
  StaticString<40> label;
  label.Format("%04u-%02u-%02u (%s, %u)",
               day.day.year, day.day.month, day.day.day,
               day.IsComplete() ? _("Complete") : _("Partial"),
               day.file_count);
  return label;
}

} // namespace WeatherMapOverlay
