// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ScrollBar.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/PaintWindow.hpp"
#include "Asset.hpp"
#include "Hardware/CPU.hpp"
#include "Look/ButtonLook.hpp"
#include "util/Compiler.h"
#include "util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <cassert>
#include <chrono>

/*
 * Three styles share this class:
 *
 *   STANDARD  a framed bar with arrow buttons, GetBarWidth() wide,
 *             painted by PaintBar()
 *   SLIM      a rounded thumb, always THUMB_WIDE_WIDTH_PT wide,
 *             painted by PaintSlim()
 *   OVERLAY   the same thumb above the content, THUMB_THIN_WIDTH_PT
 *             wide at rest and THUMB_WIDE_WIDTH_PT while it is
 *             hovered or dragged, painted by PaintOverlay()
 *
 * SLIM and OVERLAY share the thumb geometry (THUMB_*) and the grays
 * in ThumbColors; everything named OVERLAY_* belongs to OVERLAY
 * alone.
 */

/**
 * How long the #ScrollBar::Style::OVERLAY stays fully visible after
 * the last scroll movement.
 */
static constexpr auto OVERLAY_HOLD = std::chrono::milliseconds{800};

/**
 * How long the overlay stays visible while the pointer rests on it.
 * A hover is refreshed by every mouse move; this is the safety net
 * for the pointer leaving the window, where no more moves arrive.
 */
static constexpr auto OVERLAY_HOVER_HOLD = std::chrono::seconds{3};

/** The interval between two fade-out steps. */
static constexpr auto OVERLAY_FADE_INTERVAL = std::chrono::milliseconds{40};

/** How much alpha the overlay loses per fade-out step. */
static constexpr uint8_t OVERLAY_FADE_STEP = 0x20;

/** The alpha of the overlay while it is fully visible. */
static constexpr uint8_t OVERLAY_ALPHA = 0xcc;

/**
 * The alpha of the track behind the overlay.  The track is more
 * opaque than the thumb on it, because it has to cover the content
 * far enough to read as a rail, while the thumb may let a little
 * through.
 */
static constexpr uint8_t OVERLAY_TRACK_ALPHA = 0xf4;

/** The grays a thumb is painted in; the alpha is kept separately. */
struct ThumbColors {
  /** the translucent overlay, at rest and in its wide shape */
  uint8_t overlay, overlay_wide;

  /** the opaque thumb: #ScrollBar::Style::SLIM, and the overlay
      where the canvas cannot blend */
  uint8_t opaque;

  /** the track behind the wide overlay, and the hairline beside it */
  uint8_t track, track_border;
};

/**
 * The thumb reads as a shade of the background it lies on: darker
 * than the content in the light theme, lighter in the dark one, and
 * the wide shade moves further away from the background in both.
 * The hairline keeps a smaller distance from the track in the dark
 * theme, because the same step reads louder at the dark end of the
 * scale.  #ThumbColors::opaque is what the overlay composites to
 * over the list background; #ScrollBar::Style::SLIM lies on the
 * dialog background instead and borrows the same value.
 */
static constexpr ThumbColors LIGHT_COLORS{0x80, 0x60, 0x99, 0xf4, 0xd2};
static constexpr ThumbColors DARK_COLORS{0xa0, 0xc4, 0x8c, 0x2e, 0x42};

/**
 * The two widths of a thumb.  #ScrollBar::Style::OVERLAY draws the
 * thin one at rest and the wide one while the pointer rests on it or
 * drags it; #ScrollBar::Style::SLIM always draws the wide one, so
 * that a bar which can be dragged always looks the same.  The thin
 * thumb only shows the scroll position; the wide one is nearly twice
 * as wide because a finger lands on it.
 */
static constexpr unsigned THUMB_THIN_WIDTH_PT = 6;
static constexpr unsigned THUMB_WIDE_WIDTH_PT = 11;

/**
 * The gap between a thumb and the sides of its column, and the one at
 * its two round caps.  The empty area beside a cap is wider than the
 * gap at its apex, so the thumb has to come closer to the edge there
 * for the two to look the same.
 */
static constexpr unsigned THUMB_MARGIN_PT = 2;
static constexpr unsigned THUMB_CAP_MARGIN_PT = 1;

/** The hairline between the track and the content. */
static constexpr unsigned OVERLAY_BORDER_PT = 1;

/**
 * The global scroll bar style; see ScrollBar::SetGlobalStyle().  It
 * defaults to the traditional scroll bar, so that unconfigured code
 * paths (e.g. test programs) behave as before.
 */
static ScrollBar::Style global_style = ScrollBar::Style::STANDARD;

void
ScrollBar::SetGlobalStyle(Style _style) noexcept
{
  global_style = _style;
}

ScrollBar::Style
ScrollBar::GetGlobalStyle() noexcept
{
  return global_style;
}

unsigned
ScrollBar::GetBarWidth() noexcept
{
  // if the device has a pointer (mouse/touchscreen/etc.)
  if (HasPointer())
    /* with a mouse, the scroll bar can be smaller */
    return Layout::GetMinimumControlHeight();
  else
    // thin for devices without touch screen
    return Layout::VptScale(10);
}

/**
 * The width of a wide thumb.  It also sets the width of the track,
 * so that the thin overlay and the wide one share one column.
 */
[[gnu::pure]]
static unsigned
GetWideThumbWidth() noexcept
{
  return std::max(Layout::VptScale(THUMB_WIDE_WIDTH_PT), 5u);
}

/** Builds a gray, translucent if an alpha is given. */
static constexpr Color
Gray(uint8_t gray, uint8_t alpha = 0xff) noexcept
{
  return Color(gray, gray, gray, alpha);
}

unsigned
ScrollBar::GetScrollStep() noexcept
{
  return GetBarWidth();
}

/**
 * Can the overlay be faded out smoothly?  E-paper and slow CPUs
 * cannot keep up with the animation and hide it in one step instead
 * (same gate as the scroll animations in List and VScrollPanel).
 */
[[gnu::pure]]
static bool
UseOverlayFade() noexcept
{
  return !HasEPaper() && !IsSlowCPU();
}

ScrollBar::ScrollBar(PaintWindow &_window,
                     const ButtonLook &_button_look) noexcept
  :window(_window), button_renderer(_button_look), dragging(false)
{
  // Reset the ScrollBar on creation
  Reset();
}

ScrollBar::~ScrollBar() noexcept = default;

void
ScrollBar::SetSize(const PixelSize size) noexcept
{
  style = GetGlobalStyle();

  unsigned width, margin, cap_margin;

  if (!HasArrowButtons()) {
    /* inset from the sides of its column, and a little closer at its
       round caps.  #Style::SLIM draws the wide thumb, so that a bar
       which can be dragged always looks the same, and it reserves
       exactly the column it reacts in, leaving no gap beside
       itself */
    margin = std::max(Layout::VptScale(THUMB_MARGIN_PT), 2u);
    cap_margin = std::max(Layout::VptScale(THUMB_CAP_MARGIN_PT), 1u);
    width = style == Style::SLIM
      ? GetWideThumbWidth()
      : std::max(Layout::VptScale(THUMB_THIN_WIDTH_PT), 3u);
  } else {
    width = GetBarWidth();
    margin = cap_margin = 0;
  }

  thumb_margin = margin;
  thumb_cap_margin = cap_margin;

  // Update the coordinates of the scrollbar
  rc.left = size.width - width - margin;
  rc.top = cap_margin;
  rc.right = size.width - margin;
  rc.bottom = size.height - cap_margin;
}

void
ScrollBar::Reset() noexcept
{
  rc.SetEmpty();
  rc_slider.SetEmpty();
  HideOverlay();
}

const ThumbColors &
ScrollBar::GetThumbColors() const noexcept
{
  return button_renderer.GetLook().dark_mode ? DARK_COLORS : LIGHT_COLORS;
}

uint8_t
ScrollBar::FadedAlpha(uint8_t full) const noexcept
{
  return (uint8_t)(full * overlay_alpha / OVERLAY_ALPHA);
}

PixelRect
ScrollBar::GetTrackRect() const noexcept
{
  const int margin = (int)thumb_margin;

  /* the track reaches the edges the thumb is inset from, and holds
     the wide thumb with the same gap on either side */
  return PixelRect{
    PixelPoint{rc.right - (int)GetWideThumbWidth() - margin,
               rc.top - (int)thumb_cap_margin},
    PixelPoint{rc.right + margin, rc.bottom + (int)thumb_cap_margin},
  };
}

PixelRect
ScrollBar::GetHitRect() const noexcept
{
  PixelRect r = GetTrackRect();

  if (style == Style::OVERLAY && HasPointer()) {
    /* the overlay floats above the content and its thin thumb does
       not fill the track, so the column may reach into the content
       to give a finger something to hit; a #Style::SLIM bar fills
       its track and reserves it, and reacts only there */
    const unsigned min_width = Layout::GetMinimumControlHeight() / 2;
    r.left = std::min(r.left,
                      r.right - (int)std::max(min_width,
                                              (unsigned)r.GetWidth()));
  }

  return r;
}

PixelRect
ScrollBar::GetThumbRect() const noexcept
{
  PixelRect r = rc_slider;

  if (overlay_wide)
    /* grow to the left only, so that the thumb keeps its distance
       from the edge and does not jump sideways under the pointer */
    r.left = r.right - (int)GetWideThumbWidth();

  return r;
}

void
ScrollBar::InvalidateOverlay() noexcept
{
  if (window.IsDefined())
    window.Invalidate(GetTrackRect());
}

/**
 * Can a thumb be operated right now?  The #Style::OVERLAY only takes
 * presses while it is visible; otherwise the content owns the whole
 * width.
 */
bool
ScrollBar::IsThumbOperable() const noexcept
{
  return rc_slider.GetHeight() > 0 &&
    (style == Style::SLIM || overlay_alpha > 0);
}

bool
ScrollBar::IsInside(PixelPoint pt) const noexcept
{
  if (HasArrowButtons())
    return rc.Contains(pt);

  return IsThumbOperable() && GetHitRect().Contains(pt);
}

bool
ScrollBar::IsInsideSlider(PixelPoint pt) const noexcept
{
  if (HasArrowButtons())
    return rc_slider.Contains(pt);

  if (!IsThumbOperable())
    return false;

  /* some slack past the ends of the thumb, because the thumb of a
     long list is short */
  const int slack = (int)Layout::VptScale(6);

  PixelRect r = GetHitRect();
  r.top = std::max(r.top, rc_slider.top - slack);
  r.bottom = std::min(r.bottom, rc_slider.bottom + slack);
  return r.Contains(pt);
}

void
ScrollBar::NotifyMouseMove(PixelPoint pt) noexcept
{
  if (style != Style::OVERLAY || !IsDefined() || dragging)
    return;

  const bool hover = GetTrackRect().Contains(pt);
  const bool changed = hover != overlay_hover;

  overlay_hover = hover;

  if (hover) {
    /* show it right away and hold it while the pointer rests on it */
    const bool appeared = overlay_alpha != OVERLAY_ALPHA || !overlay_wide;
    overlay_wide = true;
    ShowOverlay();
    overlay_timer.Schedule(OVERLAY_HOVER_HOLD);

    if (appeared)
      InvalidateOverlay();
  } else if (changed)
    /* fade it out like after a scroll movement; it keeps its wide
       shape until it is gone */
    overlay_timer.Schedule(OVERLAY_HOLD);
}

void
ScrollBar::ShowOverlay() noexcept
{
  overlay_alpha = OVERLAY_ALPHA;
  overlay_since = std::chrono::steady_clock::now();

  if (!overlay_hover && !overlay_held)
    overlay_timer.Schedule(OVERLAY_HOLD);
}

void
ScrollBar::NotifyScroll() noexcept
{
  if (style != Style::OVERLAY || !IsDefined())
    return;

  const bool was_invisible = overlay_alpha == 0;

  ShowOverlay();

  if (was_invisible)
    InvalidateOverlay();
}

void
ScrollBar::HoldOverlay() noexcept
{
  if (style != Style::OVERLAY)
    return;

  overlay_held = true;
  overlay_timer.Cancel();
}

void
ScrollBar::ReleaseOverlay() noexcept
{
  if (!overlay_held)
    return;

  overlay_held = false;

  /* the hold starts now, not at the last pixel of movement: a drag
     that rests at the end of the list still leaves the overlay
     standing after the user lets go */
  if (overlay_alpha > 0)
    ShowOverlay();
}

void
ScrollBar::HideOverlay() noexcept
{
  overlay_timer.Cancel();
  overlay_alpha = 0;
  overlay_hover = false;
  overlay_wide = false;
  overlay_held = false;
}

void
ScrollBar::OnOverlayTimer() noexcept
{
  if (overlay_alpha == 0) {
    overlay_timer.Cancel();
    return;
  }

  /* never start fading sooner than the hold after the overlay was
     last shown; this does not depend on every path through a drag
     having scheduled the timer correctly */
  if (const auto elapsed = std::chrono::steady_clock::now() - overlay_since;
      elapsed < OVERLAY_HOLD) {
    overlay_timer.Schedule(OVERLAY_HOLD - elapsed);
    return;
  }

  if (UseOverlayFade() && overlay_alpha > OVERLAY_FADE_STEP) {
    overlay_alpha -= OVERLAY_FADE_STEP;
    overlay_timer.Schedule(OVERLAY_FADE_INTERVAL);
  } else {
    overlay_alpha = 0;
    overlay_hover = false;
    overlay_wide = false;
    overlay_timer.Cancel();
  }

  InvalidateOverlay();
}

void
ScrollBar::SetSlider(unsigned size, unsigned view_size,
                     unsigned origin) noexcept
{
  const int netto_height = GetNettoHeight();

  // If (no size) slider fills the whole area (no scrolling)
  int height = size > 0
    ? (int)(netto_height * view_size / size)
    : netto_height;
  // Prevent the slider from getting to small
  const int min_height = !HasArrowButtons()
    /* a thumb is only a few pixels wide, but it must stay long
       enough to be recognisable and to be grabbed */
    ? (int)Layout::VptScale(16)
    : GetWidth();
  if (height < min_height)
    height = min_height;

  if (height > netto_height)
    height = netto_height;

  // Calculate highest origin (counted in ListItems)
  unsigned max_origin = size - view_size;

  // Move the slider to the appropriate position
  int top = (max_origin > 0) ?
      ((netto_height - height) * origin / max_origin) : 0;

  // Prevent the slider from getting to big
  // TODO: not needed?!
  if (top + height > netto_height)
    height = netto_height - top;

  // Update slider coordinates
  rc_slider.left = rc.left;
  rc_slider.top = rc.top + GetArrowHeight() + top;
  rc_slider.right = rc.right;
  rc_slider.bottom = rc_slider.top + height;
}

unsigned
ScrollBar::ToOrigin(unsigned size, unsigned view_size, int y) const noexcept
{
  // Calculate highest origin (counted in ListItems)
  unsigned max_origin = size - view_size;
  if (max_origin <= 0)
    return 0;

  y -= rc.top + GetArrowHeight();
  if (y < 0)
    return 0;

  unsigned origin = y * max_origin / GetScrollHeight();
  return std::min(origin, max_origin);
}

void
ScrollBar::Paint(Canvas &canvas) noexcept
{
  Paint(canvas, ButtonState::ENABLED, ButtonState::ENABLED);
}

void
ScrollBar::Paint(Canvas &canvas, ButtonState up_state,
                 ButtonState down_state) noexcept
{
  switch (style) {
  case Style::STANDARD:
    PaintBar(canvas, up_state, down_state);
    break;

  case Style::SLIM:
    PaintSlim(canvas);
    break;

  case Style::OVERLAY:
    PaintOverlay(canvas);
    break;
  }
}

void
ScrollBar::PaintSlim(Canvas &canvas) noexcept
{
  if (rc_slider.GetHeight() <= 0)
    return;

  /* permanently visible, so it is opaque; the column is its own and
     there is no content underneath to shine through */
  thumb_brush.Create(IsDithered()
                     ? COLOR_BLACK
                     : Gray(GetThumbColors().opaque));

  canvas.SelectNullPen();
  canvas.Select(thumb_brush);

  const PixelRect rc_thumb = GetThumbRect();
  const unsigned diameter = rc_thumb.GetWidth();
  canvas.DrawRoundRectangle(rc_thumb, PixelSize{diameter, diameter});
}

void
ScrollBar::PaintOverlay(Canvas &canvas) noexcept
{
  if (overlay_alpha == 0 || rc_slider.GetHeight() <= 0)
    return;

  const PixelRect rc_thumb = GetThumbRect();

  const ThumbColors &colors = GetThumbColors();

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;

  if (overlay_wide) {
    /* the track shows up together with the wide thumb, so that it
       reads as something that can be grabbed */
    const PixelRect track = GetTrackRect();
    const uint8_t track_alpha = FadedAlpha(OVERLAY_TRACK_ALPHA);
    canvas.DrawFilledRectangle(track, Gray(colors.track, track_alpha));

    /* a hairline separates the track from the content */
    const int border = (int)std::max(Layout::VptScale(OVERLAY_BORDER_PT), 1u);
    canvas.DrawFilledRectangle(PixelRect{track.GetTopLeft(),
                                         PixelPoint{track.left + border,
                                                    track.bottom}},
                               Gray(colors.track_border, track_alpha));
  }

  /* the wide thumb stands out more against the content, so that the
     grab is visible */
  thumb_brush.Create(Gray(overlay_wide ? colors.overlay_wide : colors.overlay,
                          overlay_alpha));
#else
  /* this canvas cannot blend a translucent fill; a plain gray is
     legible on the background of either theme */
  thumb_brush.Create(IsDithered()
                     ? COLOR_BLACK
                     : Gray(colors.opaque));
#endif

  canvas.SelectNullPen();
  canvas.Select(thumb_brush);

  const unsigned diameter = rc_thumb.GetWidth();
  canvas.DrawRoundRectangle(rc_thumb, PixelSize{diameter, diameter});
}

void
ScrollBar::PaintBar(Canvas &canvas, ButtonState up_state,
                    ButtonState down_state) noexcept
{
  // draw rectangle around entire scrollbar area
  canvas.SelectBlackPen();
  canvas.SelectHollowBrush();
  canvas.DrawRectangle(rc);

  // draw the up/down arrow buttons
  const int arrow_padding = std::max(GetWidth() / 4, 4);

  PixelRect up_arrow_rect = rc;
  ++up_arrow_rect.left;
  up_arrow_rect.bottom = up_arrow_rect.top + GetWidth();

  PixelRect down_arrow_rect = rc;
  ++down_arrow_rect.left;
  down_arrow_rect.top = down_arrow_rect.bottom - GetWidth();

  canvas.DrawExactLine(up_arrow_rect.GetBottomLeft(),
                       up_arrow_rect.GetBottomRight());
  canvas.DrawExactLine({down_arrow_rect.left, down_arrow_rect.top - 1},
                       {down_arrow_rect.right, down_arrow_rect.top - 1});

  button_renderer.DrawButton(canvas, up_arrow_rect, up_state);
  button_renderer.DrawButton(canvas, down_arrow_rect, down_state);

  const ButtonLook &look = button_renderer.GetLook();
  canvas.SelectNullPen();

  const auto select_foreground = [&canvas, &look](ButtonState state) {
    switch (state) {
    case ButtonState::DISABLED:
      canvas.Select(look.disabled.brush);
      break;
    case ButtonState::FOCUSED:
    case ButtonState::PRESSED:
      /* match button rendering: focused and pressed share the same palette */
      canvas.Select(look.focused.foreground_brush);
      break;
    case ButtonState::SELECTED:
      canvas.Select(look.selected.foreground_brush);
      break;
    case ButtonState::ENABLED:
      canvas.Select(look.standard.foreground_brush);
      break;
    default:
      gcc_unreachable();
    }
  };

  const BulkPixelPoint up_arrow[3] = {
    { (up_arrow_rect.left + rc.right) / 2,
      up_arrow_rect.top + arrow_padding },
    { up_arrow_rect.left + arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
    { rc.right - arrow_padding,
      up_arrow_rect.bottom - arrow_padding },
  };
  select_foreground(up_state);
  canvas.DrawTriangleFan(up_arrow, ARRAY_SIZE(up_arrow));

  const BulkPixelPoint down_arrow[3] = {
    { (down_arrow_rect.left + rc.right) / 2,
      down_arrow_rect.bottom - arrow_padding },
    { down_arrow_rect.left + arrow_padding,
      down_arrow_rect.top + arrow_padding },
    { rc.right - arrow_padding,
      down_arrow_rect.top + arrow_padding },
  };
  select_foreground(down_state);
  canvas.DrawTriangleFan(down_arrow, ARRAY_SIZE(down_arrow));

  // ###################
  // ####  Slider   ####
  // ###################

  if (rc_slider.top + 4 < rc_slider.bottom) {
    canvas.SelectBlackPen();
    canvas.DrawExactLine(rc_slider.GetTopLeft(), rc_slider.GetTopRight());
    canvas.DrawExactLine(rc_slider.GetBottomLeft(),
                         rc_slider.GetBottomRight());

    PixelRect rc_slider2 = rc_slider;
    ++rc_slider2.left;
    ++rc_slider2.top;
    button_renderer.DrawButton(canvas, rc_slider2,
                               dragging ? ButtonState::PRESSED : ButtonState::ENABLED);
  }

  // fill the rest with darker gray
  const Color background_color = IsDithered() ? COLOR_BLACK : COLOR_GRAY;

  if (up_arrow_rect.bottom + 1 < rc_slider.top)
    canvas.DrawFilledRectangle({rc.left + 1, up_arrow_rect.bottom + 1, rc.right, rc_slider.top},
                               background_color);

  if (rc_slider.bottom + 1 < down_arrow_rect.top - 1)
    canvas.DrawFilledRectangle({rc.left + 1, rc_slider.bottom + 1, rc.right, down_arrow_rect.top - 1},
                               background_color);
}

void
ScrollBar::DragBegin(PaintWindow *w, unsigned y) noexcept
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Save the offset of the drag
  drag_offset = y - rc_slider.top;

  StartDrag(w);
}

void
ScrollBar::DragBeginCentred(PaintWindow *w) noexcept
{
  // Make sure that we are not dragging already
  assert(!dragging);

  // Pick the slider up by its middle
  drag_offset = GetSliderHeight() / 2;

  StartDrag(w);
}

void
ScrollBar::StartDrag(PaintWindow *w) noexcept
{
  dragging = true;

  if (style == Style::OVERLAY) {
    /* hold the overlay while it is being dragged; it grows, so that
       the finger does not cover it completely */
    overlay_wide = true;
    HoldOverlay();
    ShowOverlay();
  }

  w->SetCapture();
  w->Invalidate(HasArrowButtons() ? rc_slider : GetTrackRect());
}

void
ScrollBar::DragEnd(PaintWindow *w) noexcept
{
  // If we are not dragging right now -> nothing to end
  if (!dragging)
    return;

  // Realize that we are not dragging anymore
  dragging = false;

  if (style == Style::OVERLAY) {
    /* let it fade out again, keeping its wide shape; the next mouse
       move restores the hover state if the pointer is still on it */
    overlay_hover = false;
    ReleaseOverlay();
  }

  w->ReleaseCapture();
  w->Invalidate(HasArrowButtons() ? rc_slider : GetTrackRect());
}

unsigned
ScrollBar::DragMove(unsigned size, unsigned view_size, int y) const noexcept
{
  assert(dragging);

  return ToOrigin(size, view_size, y - drag_offset);
}
