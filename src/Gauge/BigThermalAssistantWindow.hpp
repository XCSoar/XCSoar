// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ThermalAssistantWindow.hpp"
#include "UIUtil/GestureManager.hpp"

class BigThermalAssistantWindow : public ThermalAssistantWindow {
  bool dragging;
  GestureManager gestures;

public:
  BigThermalAssistantWindow(const ThermalAssistantLook &look,
                            unsigned padding)
    :ThermalAssistantWindow(look, padding, false),
     dragging(false) {}

private:
  void StopDragging() {
    if (!dragging)
      return;

    dragging = false;
    ReleaseCapture();
  }

protected:
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnCancelMode() noexcept override;
};
