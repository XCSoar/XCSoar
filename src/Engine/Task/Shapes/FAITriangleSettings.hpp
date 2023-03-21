// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct FAITriangleSettings {
  /**
   * Specifies which threshold is used for applying the "large
   * triangle" rules.
   */
  enum class Threshold : uint8_t {
    /**
     * Standard FAI (750km).
     */
    FAI,

    /**
     * OLC, DMSt and others: 500km.
     */
    KM500,

    /**
     * A dummy entry that is used for validating profile values.
     */
    MAX
  };

  Threshold threshold;

  void SetDefaults() {
    threshold = Threshold::FAI;
  }

  /**
   * Returns the threshold for large triangles [m].
   */
  [[gnu::pure]]
  double GetThreshold() const;
};
