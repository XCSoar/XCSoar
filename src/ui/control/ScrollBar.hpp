// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "Renderer/ButtonRenderer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>

class PaintWindow;
class Canvas;
struct ThumbColors;

class ScrollBar {
public:
  /**
   * How a scroll bar presents itself.  This is a global user setting
   * (see #UISettings::scroll_bars), not a per-widget decision.
   */
  enum class Style : uint_least8_t {
    /**
     * A permanently visible scroll bar with arrow buttons.  It
     * reserves a column of the client area and can be operated with
     * the mouse.
     */
    STANDARD,

    /**
     * A permanently visible thin slider without arrow buttons.  It
     * reserves a column like #STANDARD, but a much narrower one, and
     * shows the scroll position at a glance.
     */
    SLIM,

    /**
     * A thin translucent overlay drawn on top of the content while
     * it is being scrolled, fading out afterwards.  It reserves no
     * space; the content itself is dragged, although the overlay
     * can be grabbed while it is visible.
     */
    OVERLAY,
  };

  /**
   * Sets the style for all scroll bars.  It takes effect the next
   * time a scroll bar is laid out (see #SetSize).
   */
  static void SetGlobalStyle(Style style) noexcept;

  [[gnu::pure]]
  static Style GetGlobalStyle() noexcept;

  /**
   * Returns the height of one scroll step ("one line") in pixels.  It
   * does not depend on the current style, so that scrolling by key or
   * mouse wheel is not affected by the appearance of the scroll bar.
   */
  [[gnu::pure]]
  static unsigned GetScrollStep() noexcept;

private:
  /**
   * Returns the width a scroll bar needs for arrow buttons that can
   * be pressed: a control height where there is a pointer, a thin bar
   * where there is none.  #GetScrollStep() uses it as the height of
   * one scroll line.
   */
  [[gnu::pure]]
  static unsigned GetBarWidth() noexcept;

  /** The window owning this scroll bar; repainted while fading out. */
  PaintWindow &window;

  ButtonFrameRenderer button_renderer;

  /** The style this scroll bar was laid out with (see #SetSize) */
  Style style = Style::STANDARD;

  /**
   * The gap between a thumb and the sides of its column, and the
   * smaller one at its two round caps (see #SetSize).
   */
  unsigned thumb_margin = 0, thumb_cap_margin = 0;

  /**
   * #Style::OVERLAY: the alpha of the overlay; 0 means it is
   * currently invisible.
   */
  uint8_t overlay_alpha = 0;

  /**
   * #Style::OVERLAY: when the overlay was last shown.  The fade-out
   * never starts sooner than #OVERLAY_HOLD after this, no matter
   * which of the many paths through a drag scheduled the timer, so
   * the overlay always stands a while before it goes.
   */
  std::chrono::steady_clock::time_point overlay_since{};

  /**
   * The brush for a thumb, shared by #Style::SLIM and
   * #Style::OVERLAY.  It is recreated on every paint, because the
   * overlay changes colour with #overlay_alpha and with its shape.
   */
  Brush thumb_brush;

  /**
   * #Style::OVERLAY: holds the overlay visible after the last scroll
   * movement, and then fades it out.
   */
  UI::PeriodicTimer overlay_timer{[this]{ OnOverlayTimer(); }};

  /**
   * #Style::OVERLAY: is the pointer resting on the overlay?  It then
   * grows to the wide thumb, the way the overlay scroll bars on
   * macOS do, so it can be grabbed.
   */
  bool overlay_hover = false;

  /**
   * #Style::OVERLAY: is a drag in progress?  The fade-out clock must
   * not run while the user is still holding on, or the overlay
   * disappears under the finger and is gone by the time it is let go.
   */
  bool overlay_held = false;

  /**
   * #Style::OVERLAY: paint the overlay in its wide shape?  A hover or
   * a drag sets this, and only the fade-out clears it: the overlay
   * keeps the shape it has until it is gone, instead of snapping back
   * to the thin thumb while the user is still looking.
   */
  bool overlay_wide = false;

protected:
  /** Whether the slider is currently being dragged */
  bool dragging;
  int drag_offset;
  /** Coordinates of the ScrollBar */
  PixelRect rc;
  /** Coordinates of the Slider */
  PixelRect rc_slider;

public:
  /**
   * Constructor of the ScrollBar class
   *
   * @param window the window this scroll bar is painted on; it is
   * invalidated while the #Style::OVERLAY fades out
   */
  ScrollBar(PaintWindow &window, const ButtonLook &button_look) noexcept;

  ~ScrollBar() noexcept;

  /** Returns the width of the ScrollBar */
  int GetWidth() const noexcept {
    return rc.GetWidth();
  }

  /** Returns the height of the ScrollBar */
  int GetHeight() const noexcept {
    return rc.GetHeight();
  }

  /** Returns the height of the slider */
  int GetSliderHeight() const noexcept {
    return rc_slider.GetHeight();
  }

  /**
   * Returns the height of one arrow button.  Only #Style::STANDARD has
   * them; the other two scroll over the full height.
   */
  int GetArrowHeight() const noexcept {
    return HasArrowButtons() ? GetWidth() : 0;
  }

  /**
   * Returns the height of the scrollable area of the ScrollBar.  The
   * classic bar keeps a pixel for the line below its up arrow; a
   * thumb has no frame and travels the full height, so that its gap
   * to the bottom edge matches the one to the top.
   */
  int GetNettoHeight() const noexcept {
    const int frame = HasArrowButtons() ? 1 : 0;
    return std::max(GetHeight() - 2 * GetArrowHeight() - frame, 0);
  }

  /**
   * Returns the height of the visible scroll area of the ScrollBar
   * (the area thats not covered with the slider)
   */
  int GetScrollHeight() const noexcept {
    return std::max(GetNettoHeight() - GetSliderHeight(), 1);
  }

  /**
   * Returns whether the ScrollBar is defined or has to be set up first
   * @return True if the ScrollBar is defined,
   * False if it has to be set up first
   */
  bool IsDefined() const noexcept {
    return GetWidth() > 0;
  }

  /**
   * Returns whether this scroll bar occupies a column of the client
   * area.  A #Style::OVERLAY floats above the content and does not.
   */
  constexpr bool IsReservingSpace() const noexcept {
    return style != Style::OVERLAY;
  }

  /**
   * Returns whether this scroll bar has arrow buttons, i.e. whether a
   * press beside the slider may step a row instead of moving it.
   */
  constexpr bool HasArrowButtons() const noexcept {
    return style == Style::STANDARD;
  }

  /**
   * Returns the x-Coordinate of the ScrollBar
   * (remaining client area aside the ScrollBar)
   * @param size Size of the client area including the ScrollBar
   * @return The x-Coordinate of the ScrollBar
   */
  unsigned GetLeft(const PixelSize size) const noexcept {
    if (!IsDefined() || !IsReservingSpace())
      return size.width;

    /* a thumb reserves the column it reacts in, so that it never
       swallows a press meant for the content */
    return HasArrowButtons()
      ? (unsigned)rc.left
      : (unsigned)GetHitRect().left;
  }

  /**
   * Returns whether the given PixelPoint is in the ScrollBar area.
   * A thumb reacts in a column wider than itself, so that a finger
   * can hit it; the #Style::OVERLAY does so only while it is
   * visible, otherwise the content owns the whole width.
   *
   * @param pt PixelPoint to check
   */
  [[gnu::pure]]
  bool IsInside(PixelPoint pt) const noexcept;

  /**
   * Returns whether the given PixelPoint grabs the slider.  A thumb
   * is only a few pixels wide, but it can be grabbed from anywhere
   * in its column, so that a finger can hit it too; the
   * #Style::OVERLAY must be visible at the time.
   *
   * @param pt PixelPoint to check
   */
  [[gnu::pure]]
  bool IsInsideSlider(PixelPoint pt) const noexcept;

  /**
   * Returns whether the given y-Coordinate is on the up arrow
   * @param y y-Coordinate to check
   * @return True if the given y-Coordinate is on the up arrow,
   * False otherwise
   */
  bool IsInsideUpArrow(int y) const noexcept {
    return y < rc.top + GetWidth();
  }

  /**
   * Returns whether the given y-Coordinate is on the down arrow
   * @param y y-Coordinate to check
   * @return True if the given y-Coordinate is on the down arrow,
   * False otherwise
   */
  bool IsInsideDownArrow(int y) const noexcept {
    return y >= rc.bottom - GetWidth();
  }

  /**
   * Returns whether the given y-Coordinate is above the slider area
   * @param y y-Coordinate to check
   * @return True if the given y-Coordinate is above the slider area,
   * False otherwise
   */
  bool IsAboveSlider(int y) const noexcept {
    return y < rc_slider.top;
  }

  /**
   * Returns whether the given y-Coordinate is below the slider area
   * @param y y-Coordinate to check
   * @return True if the given y-Coordinate is below the slider area,
   * False otherwise
   */
  bool IsBelowSlider(int y) const noexcept {
    return y >= rc_slider.bottom;
  }

  /**
   * Sets the size of the ScrollBar
   * (actually just the height, width is automatically set)
   * @param size Size of the Control the ScrollBar is used with
   */
  void SetSize(const PixelSize size) noexcept;

  /** Resets the ScrollBar (undefines it) */
  void Reset() noexcept;

  /**
   * Must be called whenever the content was scrolled.  In
   * #Style::OVERLAY, this shows the overlay and restarts its
   * fade-out; in the other styles it does nothing.
   */
  void NotifyScroll() noexcept;

  /**
   * Reports the current mouse position, so the #Style::OVERLAY can
   * appear and grow while the pointer rests on it (this is what
   * macOS does).  Call this only while the content is not being
   * dragged, because a touch drag passing the column is not a
   * hover.
   */
  void NotifyMouseMove(PixelPoint pt) noexcept;

  /**
   * A drag has begun: hold the #Style::OVERLAY visible until
   * #ReleaseOverlay(), so that it does not fade out while the user
   * is still holding on.
   */
  void HoldOverlay() noexcept;

  /**
   * The drag has ended: let the overlay fade out from here, so that
   * it always stays a while after the user lets go.
   */
  void ReleaseOverlay() noexcept;

  /**
   * Hides the #Style::OVERLAY immediately and stops the fade-out
   * animation.  Call this from OnDestroy(), because the animation
   * repaints the window.
   */
  void HideOverlay() noexcept;

  /** Calculates the size and position of the slider */
  void SetSlider(unsigned size, unsigned view_size, unsigned origin) noexcept;

  /** Calculates the new origin out of the given y-Coordinate of the drag */
  unsigned ToOrigin(unsigned size, unsigned view_size, int y) const noexcept;

  /** Paints the ScollBar */
  void Paint(Canvas &canvas) noexcept;

  /**
   * Paints the ScrollBar with explicit button states.
   *
   * @param up_state State of the up arrow button.
   * @param down_state State of the down arrow button.
   */
  void Paint(Canvas &canvas, ButtonState up_state,
             ButtonState down_state) noexcept;

  /**
   * Returns whether the slider is currently being dragged
   * @return True if the slider is currently being dragged, False otherwise
   */
  bool IsDragging() const noexcept {
    return dragging;
  }

  /**
   * Should be called when beginning to drag
   * (Called by ListControl::OnMouseDown)
   * @param w The Window object the ScrollBar is belonging to
   * @param y y-Coordinate
   */
  void DragBegin(PaintWindow *w, unsigned y) noexcept;

  /**
   * Should be called when beginning to drag with the slider centred
   * on the pointer instead of grabbed where it was hit.  A finger
   * cannot hit a slider that is only a few pixels tall, so a touch
   * anywhere on the track picks the slider up.
   *
   * @param w The Window object the ScrollBar is belonging to
   */
  void DragBeginCentred(PaintWindow *w) noexcept;

  /**
   * Should be called when stopping to drag
   * (Called by ListControl::OnMouseUp)
   * @param w The Window object the ScrollBar is belonging to
   */
  void DragEnd(PaintWindow *w) noexcept;

  /**
   * Should be called while dragging
   * @param size Size of the Scrollbar (not pixelwise)
   * @param view_size Visible size of the Scrollbar (not pixelwise)
   * @param y y-Coordinate
   * @return "Value" of the ScrollBar
   */
  unsigned DragMove(unsigned  size, unsigned view_size, int y) const noexcept;

private:
  /** The grays a thumb is painted in, for the current theme */
  [[gnu::pure]]
  const ThumbColors &GetThumbColors() const noexcept;

  /**
   * Scales a full alpha by how far the #Style::OVERLAY has faded
   * out, so that everything it paints disappears together.
   */
  [[gnu::pure]]
  uint8_t FadedAlpha(uint8_t full) const noexcept;

  /**
   * The track a thumb lives in: the area the #Style::OVERLAY paints
   * its track in, and the one the pointer hovers it in.
   */
  [[gnu::pure]]
  PixelRect GetTrackRect() const noexcept;

  /**
   * The column a thumb reacts to presses in, and where the content
   * beside it ends (see #GetLeft).  It is wider than the thumb, so
   * that a finger can hit it.
   */
  [[gnu::pure]]
  PixelRect GetHitRect() const noexcept;

  /** The shape a thumb is painted in */
  [[gnu::pure]]
  PixelRect GetThumbRect() const noexcept;

  /**
   * Shows the #Style::OVERLAY and restarts its hold.
   */
  void ShowOverlay() noexcept;

  /** Repaints the area the overlay may occupy */
  void InvalidateOverlay() noexcept;

  /**
   * Can a thumb be operated right now?  The #Style::OVERLAY only
   * takes presses while it is visible; otherwise the content owns
   * the whole width.
   */
  [[gnu::pure]]
  bool IsThumbOperable() const noexcept;

  /**
   * The part both #DragBegin and #DragBeginCentred share, once
   * #drag_offset has been picked.
   */
  void StartDrag(PaintWindow *w) noexcept;

  /** Paints the #Style::STANDARD scroll bar */
  void PaintBar(Canvas &canvas, ButtonState up_state,
                ButtonState down_state) noexcept;

  /** Paints the #Style::SLIM bar */
  void PaintSlim(Canvas &canvas) noexcept;

  /** Paints the #Style::OVERLAY */
  void PaintOverlay(Canvas &canvas) noexcept;

  void OnOverlayTimer() noexcept;
};
