// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/TextInBox.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Rect.hpp"
#include "util/NonCopyable.hpp"
#include "util/StaticArray.hxx"
#include "Sizes.h" /* for NAME_SIZE */

#include <tchar.h>

class WaypointLabelList : private NonCopyable {
  static constexpr int WPCIRCLESIZE = 2;

public:
  struct Label{
    char Name[NAME_SIZE+1];
    PixelPoint Pos;
    TextInBoxMode Mode;
    int AltArivalAGL;
    bool inTask;
    bool isLandable;
    bool isAirport;
    bool isWatchedWaypoint;
    bool bold;
  };

protected:
  PixelRect clip_rect;

  StaticArray<Label, 256u> labels;

public:
  explicit WaypointLabelList(PixelRect _rect) noexcept
    :clip_rect(_rect)
  {
    clip_rect.Grow(WPCIRCLESIZE);
    clip_rect.right += WPCIRCLESIZE * 2;
  }

  void Add(const char *name, PixelPoint p,
           TextInBoxMode Mode, bool bold,
           int AltArivalAGL,
           bool inTask, bool isLandable, bool isAirport,
           bool isWatchedWaypoint) noexcept;
  void Sort() noexcept;

  auto begin() const noexcept {
    return labels.begin();
  }

  auto end() const noexcept {
    return labels.end();
  }
};
