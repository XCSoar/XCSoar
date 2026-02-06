// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/control/LinkableWindow.hpp"

class Canvas;

/**
 * A window in the Quick Guide that can have clickable links.
 *
 * Inherits link focus/navigation from LinkableWindow.
 * Subclasses call DrawLink() during OnPaint() to render links,
 * and implement OnLinkActivated() to handle clicks.
 */
class QuickGuideLinkWindow : public LinkableWindow {
public:
  /**
   * Draw a link and register its rectangle for hit-testing.
   *
   * @param canvas The canvas to draw on
   * @param index The link index (must be sequential starting from 0)
   * @param rc The rectangle to draw in
   * @param text The link text to display
   * @return the height of the drawn text
   */
  unsigned DrawLink(Canvas &canvas, std::size_t index,
                    PixelRect rc, const char *text) noexcept;
};
