// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"
#include "Asset.hpp"

#include <cmath>
#include <numbers>

unsigned
ButtonFrameRenderer::GetMargin() noexcept
{
  /* wide enough for the keyboard focus ring around the face */
  return Layout::VptScale(2);
}

unsigned
ButtonFrameRenderer::GetEdgeMargin(const PixelRect &rc) noexcept
{
  const unsigned margin = GetMargin();

  return rc.GetWidth() > 4 * margin && rc.GetHeight() > 4 * margin
    ? margin
    : 0;
}

static constexpr const auto &
GetStateLook(const ButtonLook &look, ButtonState state) noexcept
{
  switch (state) {
  case ButtonState::DISABLED:
  case ButtonState::ENABLED:
    break;

  case ButtonState::SELECTED:
  case ButtonState::FOCUSED:
  case ButtonState::PRESSED:
    return look.focused;
  }

  return look.standard;
}

/**
 * Inset the face from the window edges so adjacent buttons get
 * breathing room and rounded corners reveal the background.
 */
[[gnu::pure]]
static PixelRect
GetFaceRect(PixelRect rc) noexcept
{
  rc.Grow(-(int)ButtonFrameRenderer::GetMargin());

  return rc;
}

/**
 * Draw a dashed outline along a round rectangle, walking the path by
 * arc length because the canvas has no primitive for it.
 */
static void
DrawDashedRoundRectangle(Canvas &canvas, const PixelRect &rc,
                         unsigned diameter, unsigned thickness,
                         unsigned dash, Color color) noexcept
{
  const double radius = diameter / 2.;
  const double straight_x = rc.GetWidth() - diameter;
  const double straight_y = rc.GetHeight() - diameter;
  if (straight_x < 0 || straight_y < 0 || dash == 0)
    return;

  /* one quarter of the corner circle, walked in the same units as
     the straight parts */
  const double quarter = radius * std::numbers::pi / 2.;
  const double perimeter = 2. * (straight_x + straight_y) + 4. * quarter;

  /* stretch the pattern to a whole number of periods, so the last
     dash does not run into the first one where the path closes */
  const unsigned periods = std::max(4u,
                                    (unsigned)std::lround(perimeter
                                                          / (2. * dash)));
  const double period = perimeter / periods;

  const double left = rc.left + radius, right = rc.right - radius;
  const double top = rc.top + radius, bottom = rc.bottom - radius;

  for (double t = 0.; t < perimeter; t += 1.) {
    if (std::fmod(t, period) >= period / 2.)
      continue;

    double d = t;
    double x, y;

    if (d < straight_x) {
      x = left + d;
      y = top - radius;
    } else if ((d -= straight_x) < quarter) {
      const double a = d / quarter * std::numbers::pi / 2.;
      x = right + radius * std::sin(a);
      y = top - radius * std::cos(a);
    } else if ((d -= quarter) < straight_y) {
      x = right + radius;
      y = top + d;
    } else if ((d -= straight_y) < quarter) {
      const double a = d / quarter * std::numbers::pi / 2.;
      x = right + radius * std::cos(a);
      y = bottom + radius * std::sin(a);
    } else if ((d -= quarter) < straight_x) {
      x = right - d;
      y = bottom + radius;
    } else if ((d -= straight_x) < quarter) {
      const double a = d / quarter * std::numbers::pi / 2.;
      x = left - radius * std::sin(a);
      y = bottom + radius * std::cos(a);
    } else if ((d -= quarter) < straight_y) {
      x = left - radius;
      y = bottom - d;
    } else {
      const double a = (d - straight_y) / quarter * std::numbers::pi / 2.;
      x = left - radius * std::cos(a);
      y = top - radius * std::sin(a);
    }

    const int px = (int)std::lround(x), py = (int)std::lround(y);
    const int half = (int)thickness / 2;
    canvas.DrawFilledRectangle({px - half, py - half,
                                px - half + (int)thickness,
                                py - half + (int)thickness},
                               color);
  }
}

[[gnu::pure]]
static unsigned
GetCornerDiameter(const PixelRect &face) noexcept
{
  /* radius comparable to a Tailwind "rounded-lg" card; the cap keeps
     small buttons from turning into pills */
  return std::min(Layout::VptScale(14),
                  std::min(std::max(2u, (unsigned)face.GetWidth() / 2),
                           std::max(2u, (unsigned)face.GetHeight() / 2)));
}

void
ButtonFrameRenderer::DrawButton(Canvas &canvas, PixelRect rc,
                                ButtonState state) const noexcept
{
  const ButtonLook::StateLook &_look = GetStateLook(look, state);
  const PixelRect face = GetFaceRect(rc);
  const unsigned diameter = GetCornerDiameter(face);

  /* a selected button wears the pressed face: the cursor has armed
     it, and the two states cannot appear on the same button */
  const Color fill = state == ButtonState::PRESSED ||
    state == ButtonState::SELECTED
    ? _look.pressed_background_color
    : state == ButtonState::DISABLED
    ? look.disabled.background_color
    : _look.background_color;

  /* the border follows the face: a pressed button keeps the border
     it would otherwise outgrow, and a disabled one carries the page
     background color, which leaves it without a visible outline */
  const Color border = state == ButtonState::PRESSED ||
    state == ButtonState::SELECTED
    ? fill
    : state == ButtonState::DISABLED
    ? look.disabled.ring_color
    : _look.ring_color;

  canvas.SelectNullPen();

  if (state == ButtonState::FOCUSED) {
    /* a solid ring hugging the face from the outside, like a
       Tailwind `ring-3`, drawn as a filled round rectangle because a
       fan rasterizes cleaner than a thick stroked outline */
    /* exactly the margin: any wider and the ring would grow into the
       neighbouring button */
    const unsigned width = std::max(2u, GetMargin());
    PixelRect ring_rc = face;
    ring_rc.Grow((int)width);

    const Brush ring_brush{look.focus_ring_color};
    canvas.Select(ring_brush);
    canvas.DrawRoundRectangle(ring_rc, PixelSize{diameter + 2 * width});

    /* ring_brush dies at the end of this block; it must not stay
       selected (GDI) */
    canvas.SelectHollowBrush();
  }

  /* No drop shadow: a border on the face outline, like a Tailwind
     `ring ring-inset`.  Two filled round rectangles, not a stroke,
     whose width would wobble around the corners */
  const unsigned border_width = std::max(1u, Layout::ScaleFinePenWidth(1));

  PixelRect inner = face;
  unsigned inner_diameter = diameter;

  /** Fill the current rectangle, then step inwards by @p width. */
  const auto band = [&canvas, &inner, &inner_diameter](Color color,
                                                       unsigned width) {
    const Brush brush{color};
    canvas.Select(brush);
    canvas.DrawRoundRectangle(inner, PixelSize{inner_diameter});
    canvas.SelectHollowBrush();

    inner.Grow(-(int)width);
    inner_diameter = inner_diameter > 2 * width
      ? inner_diameter - 2 * width
      : 2;
  };

  band(border, border_width);

  if (inner.left < inner.right && inner.top < inner.bottom) {
    const Brush fill_brush{fill};
    canvas.Select(fill_brush);
    canvas.DrawRoundRectangle(inner, PixelSize{inner_diameter});
    canvas.SelectHollowBrush();
  }

  if (IsDithered() && state == ButtonState::DISABLED) {
    /* a dashed outline says "not now" where a grey face cannot */
    PixelRect dashed = face;
    dashed.Grow(-(int)(border_width / 2));
    DrawDashedRoundRectangle(canvas, dashed,
                             diameter > border_width
                             ? diameter - border_width
                             : diameter,
                             border_width,
                             std::max(2u, Layout::VptScale(4)),
                             look.standard.ring_color);
  }

  /* deselect the local pen/brush before they go out of scope (the GDI
     backend must not delete objects still selected in the DC) */
  canvas.SelectHollowBrush();
  canvas.SelectNullPen();
}

PixelRect
ButtonFrameRenderer::GetDrawingRect(PixelRect rc,
                                    [[maybe_unused]] ButtonState state) const noexcept
{
  rc = GetFaceRect(rc);
  rc.Grow(-(int)GetMargin());
  return rc;
}

unsigned
ButtonRenderer::GetMinimumButtonWidth() const noexcept
{
  return Layout::GetMaximumControlHeight();
}
