// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "QuickGuideLinkWindow.hpp"
#include "Widget/WindowWidget.hpp"

#include <cstdint>

class Canvas;

class PostflightWindow final : public QuickGuideLinkWindow {
public:
  enum class LinkAction : std::uint8_t {
    FLIGHT_DOWNLOAD,
    ANALYSIS,
    STATUS,
    WEGLIDE,
    COUNT
  };

  PostflightWindow() noexcept;
  static unsigned Layout(Canvas *canvas, const PixelRect &rc,
                         PostflightWindow *window) noexcept;

protected:
  void OnPaint(Canvas &canvas) noexcept override;
  bool OnLinkActivated(std::size_t index) noexcept override;
};

class PostflightWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
