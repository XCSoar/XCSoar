// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Panel.hpp"
#include "ui/control/ScrollBar.hpp"

class VScrollPanelListener {
public:
  virtual void OnVScrollPanelChange() noexcept = 0;
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

public:
  VScrollPanel(ContainerWindow &parent, const DialogLook &look,
               const PixelRect &rc, const WindowStyle style,
               VScrollPanelListener &_listener) noexcept;

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

protected:
  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;

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
