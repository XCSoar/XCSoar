// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"

#include <cstdint>

struct DialogLook;

/**
 * Lets the user pick the screen edges up to which the InfoBoxes,
 * gauges and map overlays may extend, by tapping the edges of a
 * schematic screen.
 *
 * @see DisplaySettings::safe_area_stretch
 */
class SafeAreaStretchWidget final : public WindowWidget {
  const DialogLook &look;

  uint8_t edges;

public:
  SafeAreaStretchWidget(const DialogLook &_look, uint8_t _edges) noexcept
    :look(_look), edges(_edges) {}

  uint8_t GetEdges() const noexcept {
    return edges;
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
