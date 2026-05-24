// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"
#include "Usage.hpp"
#include "Weather/EDL/TileStore.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <array>
#include <vector>

class DataFieldEnum;

namespace MapOverlay {

class EdlControlsModel {
  static constexpr unsigned forecast_choices = EDL::FORECAST_HOURS_PER_DAY;

  std::array<BrokenDateTime, forecast_choices> forecast_times{};

public:
  EdlControlsModel() noexcept = default;

  bool OnShow(Usage usage, PageLayout::Overlay overlay) noexcept;

  void FillForecastChoices(DataFieldEnum &field) noexcept;
  void SelectForecast(unsigned index) noexcept;

  void FillLevelChoices(DataFieldEnum &field) const noexcept;
  void SelectLevel(unsigned isobar) noexcept;

  [[gnu::pure]]
  bool GetForecastAutoAdvance() const noexcept;

  void SetForecastAutoAdvance(bool auto_advance) noexcept;

  [[gnu::pure]]
  unsigned SelectedCachedDayIndex(const std::vector<EDL::CachedDay> &days) const noexcept;

  [[gnu::pure]]
  StaticString<40> FormatCachedDayLabel(const EDL::CachedDay &day) const noexcept;
};

} // namespace MapOverlay
