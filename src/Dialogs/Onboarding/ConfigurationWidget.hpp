// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "OnboardingLinkWindow.hpp"
#include "Widget/WindowWidget.hpp"

class Canvas;

class ConfigurationWindow final : public OnboardingLinkWindow {
  enum class LinkAction : std::uint8_t {
    SITE_FILES_1,
    SITE_FILES_2,
    SITE_FILES_3,
    PLANE_POLAR,
    SETUP_LOGGER_1,
    SETUP_LOGGER_2,
    SETUP_TIME,
    LOOK_INFO_BOX_SETS,
    LOOK_PAGES,
    YOUTUBE_TUTORIAL,
    DEVICES,
    COUNT
  };
public:
  ConfigurationWindow() noexcept;
protected:
  void OnPaint(Canvas &canvas) noexcept override;
private:
  unsigned DrawLink(Canvas &canvas, LinkAction link, PixelRect rc, const TCHAR *text) noexcept;
  bool HandleLink(LinkAction link_action) noexcept;
  bool OnLinkActivated(std::size_t link_action) noexcept override;
};

class ConfigurationWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
