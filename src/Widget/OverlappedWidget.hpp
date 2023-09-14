// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

/**
 * Base class for widgets that are designed to be "overlapped" windows
 * (in contrast to "tiled" windows): they obscur an area of another
 * #Window behind it.  That brings a few new aspects: the Z-order of
 * windows, and when the Widget gets hidden, the area "behind" it must
 * be redrawn.
 */
class OverlappedWidget : public WindowWidget {
public:
  /**
   * Bring this #Widget to the top of the z-order.  This is a hack to
   * allow overlapped widgets.
   */
  void Raise() noexcept;

#ifdef USE_WINUSER
  void Hide() noexcept override;
#endif
};
