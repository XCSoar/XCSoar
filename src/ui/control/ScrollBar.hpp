// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "Renderer/ButtonRenderer.hpp"

#include <algorithm>

class PaintWindow;
class Canvas;

class ScrollBar {
  ButtonFrameRenderer button_renderer;

protected:
  /** Whether the slider is currently being dragged */
  bool dragging;
  int drag_offset;
  /** Coordinates of the ScrollBar */
  PixelRect rc;
  /** Coordinates of the Slider */
  PixelRect rc_slider;

public:
  /** Constructor of the ScrollBar class */
  explicit ScrollBar(const ButtonLook &button_look) noexcept;

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

  /** Returns the height of the scrollable area of the ScrollBar */
  int GetNettoHeight() const noexcept {
    return std::max(GetHeight() - 2 * GetWidth() - 1, 0);
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
   * Returns the x-Coordinate of the ScrollBar
   * (remaining client area aside the ScrollBar)
   * @param size Size of the client area including the ScrollBar
   * @return The x-Coordinate of the ScrollBar
   */
  unsigned GetLeft(const PixelSize size) const noexcept {
    return IsDefined() ? rc.left : size.width;
  }

  /**
   * Returns whether the given PixelPoint is in the ScrollBar area
   * @param pt PixelPoint to check
   * @return True if the given PixelPoint is in the ScrollBar area,
   * False otherwise
   */
  bool IsInside(const PixelPoint &pt) const noexcept {
    return rc.Contains(pt);
  }

  /**
   * Returns whether the given PixelPoint is in the slider area
   * @param pt PixelPoint to check
   * @return True if the given PixelPoint is in the slider area,
   * False otherwise
   */
  bool IsInsideSlider(const PixelPoint pt) const noexcept {
    return rc_slider.Contains(pt);
  }

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

  /** Calculates the size and position of the slider */
  void SetSlider(unsigned size, unsigned view_size, unsigned origin) noexcept;

  /** Calculates the new origin out of the given y-Coordinate of the drag */
  unsigned ToOrigin(unsigned size, unsigned view_size, int y) const noexcept;

  /** Paints the ScollBar */
  void Paint(Canvas &canvas) const noexcept;

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
};
