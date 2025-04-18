// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/AntiFlickerWindow.hpp"
#include "ThermalAssistantRenderer.hpp"

struct ThermalAssistantLook;

class ThermalAssistantWindow : public AntiFlickerWindow
{
  ThermalAssistantRenderer renderer;

#ifdef ENABLE_OPENGL
  const bool transparent;
#endif

public:
  /**
   * @param transparent draw in a circular area only, the rest of the
   * window is transparent (OpenGL only)
   */
  ThermalAssistantWindow(const ThermalAssistantLook &look,
                         unsigned _padding, bool _small = false,
                         bool transparent=false) noexcept;

  void Update(const AttitudeState &attitude,
              const DerivedInfo &_derived) noexcept;

protected:
  void DrawCircle(Canvas &canvas) noexcept;

  virtual void OnResize(PixelSize new_size) noexcept override;
  virtual void OnPaintBuffer(Canvas &canvas) noexcept override;
};
