// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Form/Button.hpp"
#include "ui/window/PaintWindow.hpp"

#include <string>

/**
 * A bottom widget that provides XCTherm wave forecast controls.
 * Shows layer (altitude) and time steppers below the map,
 * and updates the map overlay when the user changes selections.
 *
 * Listens to live blackboard updates so the auto-switch logic
 * (altitude midpoint hysteresis, :45 time rule) is driven by the
 * latest GPS fix.
 */
class XCThermControlsWidget final : public WindowWidget,
                                    public NullBlackboardListener {
  class ControlsWindow;

public:
  XCThermControlsWidget() = default;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;
};
