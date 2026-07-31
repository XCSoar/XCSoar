// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

class Color;

/**
 * A #Widget implementation that displays multi-line text.
 *
 * @param top_separator draw a thin line along the top edge (e.g. to
 *        separate list-dialog help text from the list above)
 */
class TextWidget : public WindowWidget {
  const bool top_separator;

public:
  explicit TextWidget(bool _top_separator = false) noexcept
    :top_separator(_top_separator) {}

  void SetText(const char *text) noexcept;
  void SetColor(Color _color) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
