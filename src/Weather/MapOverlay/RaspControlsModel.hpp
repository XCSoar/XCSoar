// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

namespace WeatherMapOverlay {

class RaspControlsModel {
public:
  void OnShow() noexcept;
  void SyncFromPageLayout() noexcept;

  void SetTime(unsigned minute_of_day) noexcept;

  [[gnu::pure]]
  bool GetTimeAutoAdvance() const noexcept;

  void SetTimeAutoAdvance(bool auto_advance) noexcept;
  void ApplyAutoAdvanceTime() noexcept;
  void ResumeAutoAdvance() noexcept;

  bool StepTime(int delta) noexcept;
  void FormatTimeLabel(StaticString<64> &text) const noexcept;

  bool StepField(int delta) noexcept;
  void SelectField(unsigned field_index) noexcept;
  void FormatFieldLabel(StaticString<64> &text) const noexcept;

  [[gnu::pure]]
  bool HasTimeData() const noexcept;

  [[gnu::pure]]
  bool HasFieldData() const noexcept;
};

} // namespace WeatherMapOverlay
