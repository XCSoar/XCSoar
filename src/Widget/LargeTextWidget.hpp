// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

struct DialogLook;

/**
 * A #Widget implementation that displays multi-line text.
 */
class LargeTextWidget : public WindowWidget {
  const DialogLook &look;
  const char *text;

public:
  explicit LargeTextWidget(const DialogLook &_look,
                           const char *_text=nullptr) noexcept
    :look(_look), text(_text) {}

  void SetText(const char *text) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
