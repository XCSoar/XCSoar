// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"
#include "PageSettings.hpp"

namespace WeatherMapOverlay {

/**
 * Placeholder cursor-bar model for XCTherm until the backend is wired.
 */
class XcthermControlsModel final : public ControlsModel {
public:
  void FormatPrimaryLabel(StaticString<64> &text) const noexcept override;
  void FormatSecondaryLabel(StaticString<64> &text) const noexcept override;

  [[nodiscard]]
  bool HasPrimaryData() const noexcept override;
  [[nodiscard]]
  bool HasSecondaryData() const noexcept override;

  [[nodiscard]]
  bool StepPrimary(int delta) noexcept override;
  [[nodiscard]]
  bool StepSecondary(int delta) noexcept override;

  [[nodiscard]]
  bool GetPrimaryAutoAdvance() const noexcept override;
  void SetPrimaryAutoAdvance(bool auto_advance) noexcept override;
  void ApplyPrimaryAutoAdvance() noexcept override;

  void RefreshOverlay() noexcept override;
  void OnEnterPage(const PageLayout &layout) noexcept override;
  void OnLeavePage() noexcept override;
};

} // namespace WeatherMapOverlay
