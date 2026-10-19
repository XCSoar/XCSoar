// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"

namespace WeatherMapOverlay {

class WeGlideControlsModel final : public ControlsModel {
public:
  void FormatPrimaryLabel(StaticString<64> &text) const noexcept override;
  void FormatSecondaryLabel(StaticString<64> &text) const noexcept
    override;

  [[nodiscard]] bool HasPrimaryData() const noexcept override;
  [[nodiscard]] bool IsPrimaryEnabled() const noexcept override;
  [[nodiscard]] bool HasSecondaryData() const noexcept override;

  [[nodiscard]] bool StepPrimary(int delta) noexcept override;
  [[nodiscard]] bool StepSecondary(int delta) noexcept override;

  [[nodiscard]] bool GetPrimaryAutoAdvance() const noexcept override;
  void SetPrimaryAutoAdvance(bool auto_advance) noexcept override;
  void ApplyPrimaryAutoAdvance() noexcept override;
  void ResumePrimaryAuto() noexcept override;

  void OnGPSUpdate(const MoreData &basic) noexcept override;

  void RefreshOverlay() noexcept override;

private:
  uint32_t shown_minute = 0;
};

} // namespace WeatherMapOverlay
