// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapHudLayout.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

PixelRect
MapHudLayout::InsetPadding(PixelRect rc) noexcept
{
  const unsigned margin = Layout::GetTextPadding();
  if (rc.GetWidth() <= margin * 2 || rc.GetHeight() <= margin * 2)
    return rc;

  rc.Grow(-int(margin));
  return rc;
}

MapHudLayout
MapHudLayout::Build(PixelRect hud,
                    unsigned top_right_margin,
                    unsigned bottom_margin,
                    unsigned compass_slot,
                    unsigned scale_title_clearance) noexcept
{
  MapHudLayout layout;
  layout.content = InsetPadding(hud);

  layout.top_right = layout.content;
  layout.top_right.right -= std::min(int(top_right_margin),
                                     int(layout.top_right.GetWidth()));

  layout.bottom = layout.content;
  layout.bottom.bottom -= int(bottom_margin);

  layout.compass_slot = compass_slot;
  layout.scale_title_clearance = scale_title_clearance;
  return layout;
}

PixelRect
MapHudLayout::GetThermalBandRect() const noexcept
{
  PixelRect tb;
  tb.left = content.left;
  tb.right = content.left + Layout::Scale(25);
  tb.top = content.top;
  tb.bottom = content.top + int(content.GetHeight()) / 5;
  return tb;
}

PixelPoint
MapHudLayout::GetPanInfoOrigin(bool compass_visible) const noexcept
{
  int top = top_right.top;
  if (compass_visible)
    top += int(compass_slot);
  return {top_right.right, top};
}
