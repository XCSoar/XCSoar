// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ControlsModel.hpp"

#include "util/StaticString.hxx"

#include <memory>
#include <string_view>

class SkySightClient;
namespace SkySight {
struct Layer;
}

namespace WeatherMapOverlay {

class SkySightControlsModel final : public ControlsModel {
  std::shared_ptr<SkySightClient> skysight;
  StaticString<64> layer_id;

public:
  explicit SkySightControlsModel(std::shared_ptr<SkySightClient> _skysight,
                                 std::string_view _layer_id) noexcept;

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
  void ResumePrimaryAuto() noexcept override;

  void RefreshOverlay() noexcept override;

private:
  [[nodiscard]]
  const SkySight::Layer *GetLayer() const noexcept;
};

} // namespace WeatherMapOverlay
