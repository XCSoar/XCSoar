// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/EDL/TileStore.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <array>
#include <vector>

namespace WeatherMapOverlay {

class EdlControlsModel {
  static constexpr unsigned forecast_choices = EDL::FORECAST_HOURS_PER_DAY;

  std::array<BrokenDateTime, forecast_choices> forecast_times{};

public:
  EdlControlsModel() noexcept = default;

  void OnShow() noexcept;

  bool StepForecast(int delta) noexcept;
  bool StepLevel(int delta) noexcept;
  void ResumeAutoAdvance() noexcept;

  void FormatForecastLabel(StaticString<64> &text) const noexcept;
  void FormatLevelLabel(StaticString<64> &text) const noexcept;

  [[gnu::pure]]
  bool HasOverlayData() const noexcept;

  [[gnu::pure]]
  bool GetForecastAutoAdvance() const noexcept;

  void SetForecastAutoAdvance(bool auto_advance) noexcept;

  [[gnu::pure]]
  bool GetLevelAutoAdvance() const noexcept;

  void SetLevelAutoAdvance(bool auto_advance) noexcept;

  [[gnu::pure]]
  unsigned SelectedCachedDayIndex(const std::vector<EDL::CachedDay> &days) const noexcept;

  [[gnu::pure]]
  StaticString<40> FormatCachedDayLabel(const EDL::CachedDay &day) const noexcept;

private:
  void RebuildForecastTimes() noexcept;
  void SelectForecast(unsigned index) noexcept;
  void SelectLevel(unsigned isobar) noexcept;

  [[gnu::pure]]
  unsigned FindForecastIndex() const noexcept;
};

} // namespace WeatherMapOverlay
