// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "QuickGuideLinkWindow.hpp"
#include "Widget/WindowWidget.hpp"

#include <cstdint>

class Canvas;

class PreflightWindow final : public QuickGuideLinkWindow {
public:
  enum class LinkAction : std::uint8_t {
    CHECKLIST,
    PLANE,
    FLIGHT,
    WIND,
    TASK_MANAGER,
    COUNT
  };

  PreflightWindow() noexcept;
  static unsigned Layout(Canvas *canvas, const PixelRect &rc,
                         PreflightWindow *window) noexcept;

protected:
  void OnPaint(Canvas &canvas) noexcept override;

private:
  bool HandleLink(LinkAction link) noexcept;
  bool OnLinkActivated(std::size_t index) noexcept override;
};

class PreflightWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
