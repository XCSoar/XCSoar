// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <tchar.h>

class Color;

/**
 * A #Widget implementation that displays multi-line text.
 */
class TextWidget : public WindowWidget {
public:
  void SetText(const char *text) noexcept;
  void SetColor(Color _color) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
