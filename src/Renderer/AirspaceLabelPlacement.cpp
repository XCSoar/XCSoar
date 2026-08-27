// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceLabelPlacement.hpp"
#include "LabelBlock.hpp"

#include <array>

enum class AirspaceLabelCandidate : unsigned {
  BELOW,
  ABOVE,
  RIGHT,
  LEFT,
  LOWER_RIGHT,
  LOWER_LEFT,
  UPPER_RIGHT,
  UPPER_LEFT,
};

static constexpr std::array CANDIDATES{
  AirspaceLabelCandidate::BELOW,
  AirspaceLabelCandidate::ABOVE,
  AirspaceLabelCandidate::RIGHT,
  AirspaceLabelCandidate::LEFT,
  AirspaceLabelCandidate::LOWER_RIGHT,
  AirspaceLabelCandidate::LOWER_LEFT,
  AirspaceLabelCandidate::UPPER_RIGHT,
  AirspaceLabelCandidate::UPPER_LEFT,
};

[[gnu::const]]
static PixelRect
MakeCandidateRect(const PixelPoint anchor, const PixelSize size,
                  const unsigned clearance,
                  const AirspaceLabelCandidate candidate) noexcept
{
  const int width = int(size.width);
  const int height = int(size.height);

  /* The preferred location preserves the old geometry exactly.  The other
     locations leave enough room for both collision clearances plus one pixel:
     PixelRect::OverlapsWith() treats touching edges as overlapping. */
  const int gap = 2 * int(clearance) + 1;
  const int horizontal_offset = width / 2 + gap;
  const int vertical_offset = height + gap;

  int left = 0;
  int top = 0;
  switch (candidate) {
  case AirspaceLabelCandidate::BELOW:
    left = anchor.x - width / 2;
    top = anchor.y;
    break;

  case AirspaceLabelCandidate::ABOVE:
    left = anchor.x - width / 2;
    top = anchor.y - vertical_offset;
    break;

  case AirspaceLabelCandidate::RIGHT:
    left = anchor.x + horizontal_offset;
    top = anchor.y - height / 2;
    break;

  case AirspaceLabelCandidate::LEFT:
    left = anchor.x - horizontal_offset - width;
    top = anchor.y - height / 2;
    break;

  case AirspaceLabelCandidate::LOWER_RIGHT:
    left = anchor.x + horizontal_offset;
    top = anchor.y + vertical_offset;
    break;

  case AirspaceLabelCandidate::LOWER_LEFT:
    left = anchor.x - horizontal_offset - width;
    top = anchor.y + vertical_offset;
    break;

  case AirspaceLabelCandidate::UPPER_RIGHT:
    left = anchor.x + horizontal_offset;
    top = anchor.y - vertical_offset - height / 2;
    break;

  case AirspaceLabelCandidate::UPPER_LEFT:
    left = anchor.x - horizontal_offset - width;
    top = anchor.y - vertical_offset - height / 2;
    break;
  }

  return {left, top, left + width, top + height};
}

std::optional<AirspaceLabelPlacement>
PlaceAirspaceLabel(const PixelPoint anchor, const PixelSize size,
                   const unsigned clearance, const PixelRect &map_rect,
                   LabelBlock *const label_block) noexcept
{
  if (size.width == 0 || size.height == 0)
    return std::nullopt;

  for (unsigned i = 0; i < CANDIDATES.size(); ++i) {
    const PixelRect visual_rect =
      MakeCandidateRect(anchor, size, clearance, CANDIDATES[i]);
    const PixelRect collision_rect = visual_rect.WithMargin(int(clearance));

    if (!map_rect.Contains(collision_rect))
      continue;

    if (label_block != nullptr && !label_block->check(collision_rect))
      continue;

    return AirspaceLabelPlacement{visual_rect, i};
  }

  return std::nullopt;
}
