// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"
#include "Weather/EDL/DownloadGlue.hpp"

namespace WeatherMapOverlay {

/**
 * Weather cursor-bar model backed by #EDL::FieldControls.
 */
class EdlControlsModel final : public ControlsModel,
                               private EDL::DownloadListener
{
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
  PrimaryLabelAction GetPrimaryLabelAction() const noexcept override;
  void OpenPrimaryPicker() noexcept override;
  void EnablePrimaryAutoFromInput() noexcept override;

  [[nodiscard]]
  bool SupportsSecondaryAutoAdvance() const noexcept override;
  [[nodiscard]]
  bool GetSecondaryAutoAdvance() const noexcept override;
  void SetSecondaryAutoAdvance(bool auto_advance) noexcept override;
  void ApplySecondaryAutoAdvance() noexcept override;

  [[nodiscard]]
  SecondaryLabelAction GetSecondaryLabelAction() const noexcept override;

  void ResumePrimaryAuto() noexcept override;
  void ResumeSecondaryAuto() noexcept override;

  [[nodiscard]]
  SecondaryPickerResult OpenSecondaryPicker() noexcept override;

  void RefreshOverlay() noexcept override;
  void OnGPSUpdate(const MoreData &basic) noexcept override;

  void OnDownloadFinished(const EDL::DownloadNotification &) noexcept override;

private:
  void UnregisterEdlDownloadListener() noexcept;
};

} // namespace WeatherMapOverlay
