// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningStatusRenderer.hpp"
#include "Look/Colors.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>

[[gnu::pure]]
static int
CaptionWidth(Canvas &canvas) noexcept
{
  return std::max(canvas.CalcTextWidth(C_("Status", "Inside")),
                  canvas.CalcTextWidth(_("Near")));
}

int
AirspaceWarningStatusWidth(Canvas &canvas, const Font &font) noexcept
{
  canvas.Select(font);

  /* caption + padding on both sides + gap to the row edge */
  return CaptionWidth(canvas) + 3 * (int)Layout::GetTextPadding();
}

void
DrawAirspaceWarningStatus(Canvas &canvas, const Font &font,
                          PixelRect status_rc,
                          AirspaceWarningStatusBadge status) noexcept
{
  Color state_color;
  const char *state_text = nullptr;

  switch (status.kind) {
  case AirspaceWarningStatusBadge::Kind::Inside:
    state_color = status.active
      ? COLOR_AIRSPACE_WARNING_INSIDE
      : COLOR_AIRSPACE_WARNING_INSIDE_ACK;
    state_text = C_("Status", "Inside");
    break;

  case AirspaceWarningStatusBadge::Kind::Near:
    state_color = status.active
      ? COLOR_AIRSPACE_WARNING_NEAR
      : COLOR_AIRSPACE_WARNING_NEAR_ACK;
    state_text = _("Near");
    break;

  case AirspaceWarningStatusBadge::Kind::None:
    return;
  }

  canvas.Select(font);

  const unsigned padding = Layout::GetTextPadding();
  const PixelSize state_text_size = canvas.CalcTextSize(state_text);

  /* Fill the reserved column (pad top/bottom/right).  Both captions
     share this rectangle so Inside and Near stay the same width. */
  PixelRect badge_rc = status_rc;
  badge_rc.top += padding;
  badge_rc.bottom -= padding;
  badge_rc.right -= padding;

  canvas.DrawFilledRectangle(badge_rc, state_color);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.DrawText(badge_rc.CenteredTopLeft(state_text_size), state_text);
}
