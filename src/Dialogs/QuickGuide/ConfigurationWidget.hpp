// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "QuickGuideLinkWindow.hpp"
#include "Widget/WindowWidget.hpp"

#include <cstdint>

class Canvas;

class ConfigurationWindow final : public QuickGuideLinkWindow {
public:
  enum class LinkAction : std::uint8_t {
    SITE_FILES_1,
    SITE_FILES_2,
    SITE_FILES_3,
    PLANE_POLAR,
    SETUP_LOGGER_1,
    SETUP_LOGGER_2,
    SETUP_TIME,
    CHECKLIST,
    LOOK_INFO_BOX_SETS,
    LOOK_PAGES,
    YOUTUBE_TUTORIAL,
    DEVICES,
    COUNT
  };

  ConfigurationWindow() noexcept;
  static unsigned Layout(Canvas *canvas, const PixelRect &rc,
                         ConfigurationWindow *window) noexcept;

protected:
  void OnPaint(Canvas &canvas) noexcept override;

private:
  bool HandleLink(LinkAction link) noexcept;
  bool OnLinkActivated(std::size_t index) noexcept override;
};

class ConfigurationWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
