// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <functional>
#include <memory>

#include <tchar.h>

struct ButtonLook;
class Button;
class ButtonRenderer;

/**
 * A #Widget that creates a #Button.
 */
class ButtonWidget : public WindowWidget {
  std::unique_ptr<ButtonRenderer> renderer;
  const std::function<void()> callback;

public:
  ButtonWidget(std::unique_ptr<ButtonRenderer> _renderer,
               std::function<void()> _callback) noexcept;

  ButtonWidget(const ButtonLook &look, const char *caption,
               std::function<void()> _callback) noexcept;

  ~ButtonWidget() noexcept override;

  ButtonRenderer &GetRenderer() noexcept;
  const ButtonRenderer &GetRenderer() const noexcept;

  /**
   * Schedule a repaint.
   */
  void Invalidate() noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
