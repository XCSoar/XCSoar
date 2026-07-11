// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"

#ifdef HAVE_HTTP
#include "Weather/xctherm/XCThermControlsModel.hpp"
#endif

namespace WeatherMapOverlay {

/** Weather cursor-bar model backed by XCTherm controls state. */
class XcthermControlsModel final : public ControlsModel {
#ifdef HAVE_HTTP
  mutable XCTherm::XCThermControlsModel backend;
  bool prepared = false;

  template<typename F>
  void WithBackend(F &&f) noexcept
  {
    std::forward<F>(f)(backend);
  }

  template<typename F>
  void WithBackend(F &&f) const noexcept
  {
    std::forward<F>(f)(backend);
  }
#else
  template<typename F>
  void WithBackend(F &&) noexcept {}
#endif

public:
  void OnShow() noexcept override;
  void OnHide() noexcept override;

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
  void EnablePrimaryAutoFromInput() noexcept override;

  [[nodiscard]]
  PrimaryLabelAction GetPrimaryLabelAction() const noexcept override;
  void OpenPrimaryPicker() noexcept override;

  [[nodiscard]]
  SecondaryLabelAction GetSecondaryLabelAction() const noexcept override;
  void OpenSecondaryPicker() noexcept override;

  [[nodiscard]]
  bool SupportsSecondaryAutoAdvance() const noexcept override;
  [[nodiscard]]
  bool GetSecondaryAutoAdvance() const noexcept override;
  void SetSecondaryAutoAdvance(bool auto_advance) noexcept override;

  void ResumePrimaryAuto() noexcept override;
  void ResumeSecondaryAuto() noexcept override;

  void OnGPSUpdate(const MoreData &basic) noexcept override;

  void RefreshOverlay() noexcept override;
};

} // namespace WeatherMapOverlay
