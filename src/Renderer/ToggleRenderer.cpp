// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ToggleRenderer.hpp"
#include "Look/Colors.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Pen.hpp"

#include <algorithm>

/**
 * The height the switch would like, and the distance between its
 * track and its thumb.
 */
static constexpr unsigned TOGGLE_HEIGHT_PT = 22;
static constexpr unsigned TOGGLE_INSET_PT = 2;

/** The hair line of the outline.  At least one pixel. */
static int
OutlineThickness() noexcept
{
  return std::max(1u, Layout::ScalePenWidth(1) / 2);
}

int
ToggleHeight(int row_height) noexcept
{
  /* it must not touch the edges of a row which is shorter than the
     switch would like to be.  The room it leaves is the one the text
     keeps.  The padding of a card is the horizontal one and much
     larger: on a device without a touch screen, where a row is only
     as tall as one line of text, it would leave nothing at all */
  return std::max(6, std::min((int)Layout::VptScale(TOGGLE_HEIGHT_PT),
                              row_height
                              - 2 * (int)Layout::GetTextPadding())) & ~1;
}

int
ToggleWidth(int height) noexcept
{
  return height * 33 / 20;
}

/** The colors of the switch which shows a boolean value. */
struct ToggleColors {
  /** the pill behind the thumb */
  Color track_color;

  /** the thumb which sits at one of its ends */
  Color thumb_color;

  /** the outline of the pill; the same as #track_color draws none */
  Color outline_color;
};

[[gnu::pure]]
static ToggleColors
GetToggleColors(const DialogLook &look, bool checked,
                Color background_color, Color text_color) noexcept
{
  if (IsDithered())
    /* a display which knows two colors has nothing but the two colors
       of the row it sits on: the switch which is on is filled with
       the color of the text, the one which is off shows the row
       through it and draws its outline instead */
    return {checked ? text_color : background_color,
            checked ? background_color : text_color,
            text_color};

  if (checked)
    return {COLOR_TOGGLE_ON, COLOR_WHITE, COLOR_TOGGLE_ON};

  const Color track = look.dark_mode
    ? COLOR_TOGGLE_TRACK_DARK
    : COLOR_TOGGLE_TRACK_LIGHT;

  return {track, COLOR_WHITE, track};
}

void
DrawToggle(Canvas &canvas, const PixelRect &rc, bool checked,
           const DialogLook &look, Color background_color,
           Color text_color) noexcept
{
  const auto colors = GetToggleColors(look, checked,
                                      background_color, text_color);

  /* the pill is a rectangle between two half circles; other than a
     rounded rectangle whose corners are painted over, this touches no
     pixel outside the switch, and the background may be anything.

     Only the circles take their color from the brush; it and the pen
     have a name because a temporary would be gone at the semicolon,
     and a canvas which selects the object itself rather than a copy
     of it would draw with a deleted one.  The pen paints the rim of
     the circle: give it the color of the brush, because a null pen is
     black and the rim would be a frayed dark edge */
  const auto draw_pill = [&canvas](const PixelRect &r, Color color){
    const int radius = (int)r.GetHeight() / 2;
    const int centre_y = r.top + radius;

    canvas.DrawFilledRectangle({r.left + radius, r.top,
                                r.right - radius, r.bottom},
                               color);

    const Brush brush(color);
    const Pen pen(0, color);
    canvas.Select(brush);
    canvas.Select(pen);

    canvas.DrawCircle({r.left + radius, centre_y}, radius);
    canvas.DrawCircle({r.right - radius, centre_y}, radius);
  };

  if (colors.outline_color == colors.track_color)
    draw_pill(rc, colors.track_color);
  else {
    /* the switch which is off has the color of the row behind it:
       draw it one line larger in the color of the outline, and let
       the track cover all but that line */
    draw_pill(rc, colors.outline_color);

    PixelRect inner = rc;
    inner.Grow(-OutlineThickness());
    draw_pill(inner, colors.track_color);
  }

  const int radius = (int)rc.GetHeight() / 2;
  const int inset = std::max(1, (int)Layout::VptScale(TOGGLE_INSET_PT));
  const int thumb_radius = radius - inset;

  const Brush thumb_brush(colors.thumb_color);
  const Pen thumb_pen(0, colors.thumb_color);
  canvas.Select(thumb_brush);
  canvas.Select(thumb_pen);
  canvas.DrawCircle({checked
                     ? rc.right - radius
                     : rc.left + radius,
                     rc.top + radius}, thumb_radius);
}
