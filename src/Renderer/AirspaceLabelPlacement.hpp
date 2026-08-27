// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"

#include <optional>

class LabelBlock;

/**
 * A location selected for an airspace altitude label.
 *
 * The visual rectangle is the rectangle used for drawing.  The collision
 * rectangle used internally by PlaceAirspaceLabel() is larger by the supplied
 * clearance.
 */
struct AirspaceLabelPlacement {
  PixelRect visual_rect;
  unsigned candidate_index;
};

/**
 * Select and, when @p label_block is supplied, reserve one of the bounded
 * positions around @p anchor for a label of @p size.
 *
 * A placement is returned only when its collision rectangle is fully inside
 * @p map_rect.  This makes calls to LabelBlock safe and keeps displaced labels
 * close to their airspace anchor.  Without a LabelBlock, this chooses the
 * first in-bounds candidate without reserving it.
 */
[[nodiscard]] std::optional<AirspaceLabelPlacement>
PlaceAirspaceLabel(PixelPoint anchor, PixelSize size, unsigned clearance,
                   const PixelRect &map_rect,
                   LabelBlock *label_block) noexcept;
