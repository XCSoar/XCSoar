// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningStatusRenderer.hpp"
#include "Look/Colors.hpp"
#include "Language/Language.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>

int
AirspaceWarningStatusWidth(Canvas &canvas) noexcept
{
  return std::max(canvas.CalcTextWidth(_("Inside")),
                  canvas.CalcTextWidth(_("Near")));
}

void
DrawAirspaceWarningStatus(Canvas &canvas, PixelRect status_rc,
                          AirspaceWarningStatusBadge status) noexcept
{
  Color state_color;
  const char *state_text = nullptr;

  switch (status.kind) {
  case AirspaceWarningStatusBadge::Kind::Inside:
    state_color = status.active
      ? COLOR_AIRSPACE_WARNING_INSIDE
      : COLOR_AIRSPACE_WARNING_INSIDE_ACK;
    state_text = _("Inside");
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

  if (state_text == nullptr)
    return;

  const unsigned padding = Layout::GetTextPadding();
  const PixelSize state_text_size = canvas.CalcTextSize(state_text);

  /* Size the badge from the caption instead of the reserved column:
     the column is measured with the widest caption and would leave no
     room around a caption that is exactly as wide. */
  PixelRect badge_rc = status_rc;
  badge_rc.top += padding;
  badge_rc.bottom -= padding;
  badge_rc.right -= padding;
  badge_rc.left = std::max(status_rc.left,
                           badge_rc.right
                           - (int)(state_text_size.width + 2 * padding));

  canvas.DrawFilledRectangle(badge_rc, state_color);

  canvas.SetTextColor(COLOR_BLACK);
  canvas.DrawText(badge_rc.CenteredTopLeft(state_text_size), state_text);
}
