// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#include "Weather/EDL/Levels.hpp"
#include "time/BrokenDateTime.hpp"

#include <array>
#include <memory>

namespace WeatherMapOverlay {

class EdlControlsModel final : public ControlsModel,
                               private EDL::DownloadListener
{
  static constexpr unsigned forecast_choices = EDL::FORECAST_HOURS_PER_DAY;

  std::array<BrokenDateTime, forecast_choices> forecast_times{};
  EDL::DownloadGlue *edl_listener_glue = nullptr;

public:
  EdlControlsModel() noexcept = default;
  ~EdlControlsModel() noexcept override;

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

  [[nodiscard]]
  bool SupportsSecondaryAutoAdvance() const noexcept override;
  [[nodiscard]]
  bool GetSecondaryAutoAdvance() const noexcept override;
  void SetSecondaryAutoAdvance(bool auto_advance) noexcept override;
  void ApplySecondaryAutoAdvance() noexcept override;

  void ResumePrimaryAuto() noexcept override;
  void ResumeSecondaryAuto() noexcept override;

  void RefreshOverlay() noexcept override;
  void OnGPSUpdate(const MoreData &basic) noexcept override;

  void OnDownloadFinished(const EDL::DownloadNotification &) noexcept override;

private:
  void RebuildForecastTimes() noexcept;
  void SelectForecast(unsigned index) noexcept;
  void SelectLevel(unsigned isobar) noexcept;
  void UnregisterEdlDownloadListener() noexcept;

  [[nodiscard]] [[gnu::pure]]
  unsigned FindForecastIndex() const noexcept;

  [[nodiscard]]
  bool HasOverlayData() const noexcept;
};

} // namespace WeatherMapOverlay
