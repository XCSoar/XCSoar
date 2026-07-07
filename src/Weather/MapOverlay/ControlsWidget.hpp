// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "Blackboard/BlackboardListenerRegistration.hpp"
#include "ControlsModel.hpp"
#include "Widget/CursorBarWidget.hpp"

#include <memory>

namespace WeatherMapOverlay {

/**
 * Map-bottom weather cursor bar for all weather overlays (EDL, RASP,
 * XCTherm). Overlay-specific behaviour lives in #ControlsModel.
 */
class ControlsWidget final : public CursorBarWidget,
                             public NullBlackboardListener {
  static constexpr unsigned PRIMARY_ROW = 0;
  static constexpr unsigned SECONDARY_ROW = 1;

  std::unique_ptr<ControlsModel> model;
  BlackboardListenerRegistration blackboard_listener;

  void RegisterBlackboard() noexcept;
  void UnregisterBlackboard() noexcept;

  void UpdateLabels() noexcept;
  void RefreshOverlay() noexcept;
  void ApplyUpdate(ControlsUpdate update) noexcept;

  void OnStepPrimary(int delta) noexcept;
  void OnStepSecondary(int delta) noexcept;

public:
  explicit ControlsWidget(std::unique_ptr<ControlsModel> _model) noexcept;
  ~ControlsWidget() noexcept override;

  ControlsWidget(const ControlsWidget &) = delete;
  ControlsWidget &operator=(const ControlsWidget &) = delete;

  void HandleWeatherOverlayInput(const char *misc) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;
};

} // namespace WeatherMapOverlay
