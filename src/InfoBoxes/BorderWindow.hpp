// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"

struct InfoBoxLook;

/**
 * Draws the borders of an InfoBox slot which is configured
 * "invisible".  Such a slot has no visible #InfoBoxWindow: the map
 * window is extended over it, and this window paints nothing but the
 * border lines on top of the map, leaving the rest untouched.
 *
 * This keeps every grid line on the very pixel it would occupy
 * without the invisible slot.  Letting the neighbouring InfoBoxes draw
 * the shared edges instead moves them by one pixel, because
 * #BORDERTOP / #BORDERLEFT are drawn on the first row/column of a
 * window while #BORDERBOTTOM / #BORDERRIGHT are drawn on the last one.
 *
 * Requires transparent windows, which the USE_WINUSER (GDI) backend
 * does not provide; #InfoBoxManager falls back to letting the
 * neighbours draw the edges there.
 */
class InfoBoxBorderWindow final : public PaintWindow {
  const InfoBoxLook &look;

  unsigned border_kind;

public:
  InfoBoxBorderWindow(ContainerWindow &parent, PixelRect rc,
                      unsigned _border_kind,
                      const InfoBoxLook &_look) noexcept;

  void SetBorderKind(unsigned _border_kind) noexcept;

protected:
  /* virtual methods from class Window */
  void OnPaint(Canvas &canvas) noexcept override;
};
