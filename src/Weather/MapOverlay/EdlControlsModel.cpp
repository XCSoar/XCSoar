// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlControlsModel.hpp"

#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageActions.hpp"
#include "UIState.hpp"
#include "Weather/EDL/Levels.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Form/DataField/Enum.hpp"

#include <chrono>

namespace MapOverlay {

bool
EdlControlsModel::OnShow(Usage usage, PageLayout::Overlay overlay) noexcept
{
  if (overlay != PageLayout::Overlay::EDL)
    return false;

  EDL::EnsureInitialised();

  const bool edl_page = PageActions::GetCurrentLayout().UsesEdlOverlay();

  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible())
    EDL::OnTimeUpdate(basic.date_time_utc);
  else
    EDL::OnTimeUpdate(BrokenDateTime::NowUTC());

  return usage == Usage::MAP_BOTTOM && edl_page && !EDL::OverlayVisible();
}

void
EdlControlsModel::FillForecastChoices(DataFieldEnum &field) noexcept
{
  EDL::EnsureInitialised();

  auto selected_time = EDL::GetForecastTime();
  if (!selected_time.IsPlausible())
    selected_time = EDL::GetTrackedForecastTime(BrokenDateTime::NowUTC());

  if (!selected_time.IsPlausible())
    return;

  CommonInterface::SetUIState().weather.edl.forecast_datetime = selected_time;

  field.ClearChoices();

  const auto base_time = selected_time + std::chrono::hours{-11};
  unsigned selected_index = 0;
  for (unsigned i = 0; i < forecast_choices; ++i) {
    forecast_times[i] = base_time + std::chrono::hours{i};
    StaticString<32> label;
    label.Format("%02u:00", unsigned(forecast_times[i].ToLocal().hour));
    field.AddChoice(i, label.c_str());

    if (forecast_times[i] == selected_time)
      selected_index = i;
  }

  field.SetValue(selected_index);
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

void
EdlControlsModel::FillLevelChoices(DataFieldEnum &field) const noexcept
{
  field.ClearChoices();

  for (unsigned i = 0; i < EDL::NUM_ISOBARS; ++i) {
    StaticString<32> label;
    label.Format(_("%u hPa (%d m)"),
                 EDL::ISOBARS[i] / 100,
                 EDL::GetAltitudeForIsobar(EDL::ISOBARS[i]));
    field.AddChoice(EDL::ISOBARS[i], label.c_str());
  }

  field.SetValue(EDL::GetIsobar());
}

void
EdlControlsModel::SelectLevel(unsigned isobar) noexcept
{
  EDL::EnsureInitialised();
  auto &edl = CommonInterface::SetUIState().weather.edl;
  edl.SelectIsobar(isobar);
  edl.forecast_auto_advance = false;
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

} // namespace MapOverlay
