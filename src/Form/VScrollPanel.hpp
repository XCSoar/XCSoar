// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Panel.hpp"
#include "ui/control/ScrollBar.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "UIUtil/KineticManager.hpp"
#include "UIUtil/GestureManager.hpp"

class VScrollPanelListener {
public:
  virtual void OnVScrollPanelChange() noexcept = 0;

  /**
   * Called when a touch gesture (e.g. swipe) is detected on the
   * scroll panel.  The gesture string uses the same format as
   * #GestureManager (e.g. "L", "R", "U", "D").
   *
   * @return true if the gesture was handled
   */
  virtual bool OnVScrollPanelGesture(const TCHAR *gesture) noexcept {
    (void)gesture;
    return false;
  }
};

/**
 * A panel which shows a vertical scroll bar if the virtual height
 * (i.e. the height available for child windows) exceeds the physical
 * height of this window.
 *
 * This window does not actually scroll; it requires implementing a
 * #VScrollPanelListener whose OnVScrollPanelChange() method gets
 * called whenever the scroll position is changed.  It is up to this
 * method to move all child windows around.
 *
 * This class is designed to be used by class #VScrollWidget.
 */
class VScrollPanel final : public PanelControl {
  VScrollPanelListener &listener;

  ScrollBar scroll_bar;

  /**
   * The height of the virtual area which can be scrolled in.
   */
  unsigned virtual_height = 0;

  /**
   * The top-most virtual pixel line visible in the visible area.
   */
  unsigned origin = 0;

  /**
   * Tracks active drag gesture and optional kinetic scrolling.
   */
  bool dragging = false;

  /**
   * Tracks if we're waiting to determine if this is a tap or drag.
   */
  bool potential_tap = false;
  PixelPoint drag_start = {0, 0};

  /**
   * The vertical distance from the start of the drag relative to the
   * top of the list (not the top of the screen)
   */
  int drag_y = 0;

  KineticManager kinetic;
  UI::PeriodicTimer kinetic_timer{[this]{ OnKineticTimer(); }};

  /**
   * Detects swipe gestures (L/R/U/D) from mouse/touch movement.
   */
  GestureManager gestures;

  /**
   * Target position for smooth keyboard scrolling (-1 = no animation).
   */
  int smooth_scroll_target = -1;

  /**
   * Timer for smooth scroll animation (~60fps).
   */
  UI::PeriodicTimer smooth_scroll_timer{[this]{ OnSmoothScrollTimer(); }};

public:
  VScrollPanel(ContainerWindow &parent, const DialogLook &look,
               const PixelRect &rc, const WindowStyle style,
               VScrollPanelListener &_listener) noexcept;
  ~VScrollPanel() noexcept override;

  /**
   * Sets the virtual height and initialises the scroll bar.  This
   * method does not call to #VScrollPanelListener.
   */
  void SetVirtualHeight(unsigned _virtual_height) noexcept;

  /**
   * Returns the position of the virtual rectangle within this window.
   * If the user has scrolled down, then its #top field is negative.
   */
  [[gnu::pure]]
  PixelRect GetVirtualRect() const noexcept {
    return PixelRect{
      PixelPoint{0, -int(origin)},
      PixelSize{
        scroll_bar.GetLeft(GetSize()),
        virtual_height,
      },
    };
  }

  /**
   * Returns the rectangle that is available for child windows,
   * i.e. the client area minus the scroll bar.
   */
  [[gnu::pure]]
  PixelRect GetPhysicalRect(PixelSize size) const noexcept {
    return PixelRect{PixelSize{
        scroll_bar.GetLeft(size),
        size.height,
      }};
  }

private:
  void SetupScrollBar() noexcept;
  void SetOriginClamped(int new_origin) noexcept;
  void OnKineticTimer() noexcept;
  void OnSmoothScrollTimer() noexcept;

  /**
   * Start smooth scrolling to a target position with easing.
   */
  void SmoothScrollTo(int target) noexcept;

public:
  /**
   * Can the panel scroll further down?
   */
  [[gnu::pure]]
  bool CanScrollDown() const noexcept {
    const unsigned physical_height = GetSize().height;
    return virtual_height > physical_height &&
           origin + physical_height < virtual_height;
  }

  /**
   * Can the panel scroll further up?
   */
  [[gnu::pure]]
  bool CanScrollUp() const noexcept {
    return origin > 0;
  }

  /**
   * Scroll the panel by the specified delta.
   *
   * @param delta The scroll amount (positive = down, negative = up).
   */
  void ScrollBy(int delta) noexcept;

  /**
   * Get the scroll step size based on scroll bar width.
   *
   * @return The scroll step size, minimum 1.
   */
  [[gnu::pure]]
  int GetScrollStep() const noexcept;

protected:
  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;
  void OnDestroy() noexcept override;

  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseWheel(PixelPoint p, int delta) noexcept override;

  void OnCancelMode() noexcept override;

  void OnPaint(Canvas &canvas) noexcept override;

public:
  void ScrollTo(const PixelRect &rc) noexcept override;
};
