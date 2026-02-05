// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "Form/VScrollPanel.hpp"

#include <functional>

struct DialogLook;

/**
 * A #Widget implementation which shows a vertical scroll bar if the
 * hosted #Widget is larger than the available space.
 *
 * When @a reserve_scrollbar is true the child widget's width is
 * reduced by the scrollbar width so content never hides behind the
 * scrollbar.  This mode also enables enhanced keyboard handling
 * (Up/Down scroll only when scrollable) and the gesture callback
 * for horizontal swipes.
 *
 * When @a reserve_scrollbar is false (the default) the child widget
 * receives the full rectangle and scrolling only activates when the
 * widget cannot shrink to fit (min height > viewport).  This is
 * appropriate for flexible form-style widgets such as RowFormWidget.
 */
class VScrollWidget final : public WindowWidget, VScrollPanelListener {
  const DialogLook &look;

  std::unique_ptr<Widget> widget;

  bool visible = false;

  /** Reserve horizontal space for the scrollbar. */
  bool reserve_scrollbar;

  /**
   * Optional callback for horizontal swipe gestures.
   * Called with true for swipe-right (next), false for swipe-left
   * (previous).  Only used when @a reserve_scrollbar is true.
   */
  std::function<void(bool)> gesture_callback;

public:
  /**
   * @param _widget        The child widget to host
   * @param _look          Dialog look for scrollbar rendering
   * @param _reserve_scrollbar  If true, reserve space for scrollbar
   *                            and enable enhanced keyboard/gesture
   *                            handling (for rich-text / prose content)
   */
  explicit VScrollWidget(std::unique_ptr<Widget> &&_widget,
                         const DialogLook &_look,
                         bool _reserve_scrollbar = false) noexcept
    :look(_look),
     widget(std::move(_widget)),
     reserve_scrollbar(_reserve_scrollbar) {}

  Widget &GetWidget() noexcept {
    return *widget;
  }

  const Widget &GetWidget() const noexcept {
    return *widget;
  }

  /**
   * Set a callback for horizontal swipe gestures.
   * Only effective when reserve_scrollbar is true.
   *
   * @param cb Called with true for swipe-right (next page),
   *           false for swipe-left (previous page)
   */
  void SetGestureCallback(std::function<void(bool)> cb) noexcept {
    gesture_callback = std::move(cb);
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
  bool Click() noexcept override;
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Leave() noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

private:
  VScrollPanel &GetWindow() noexcept {
    return (VScrollPanel &)WindowWidget::GetWindow();
  }

  [[gnu::pure]]
  static unsigned GetScrollbarWidth() noexcept;

  /**
   * Shrink the rectangle by the scrollbar width (right side).
   * Returns @a rc unchanged when reserve_scrollbar is false.
   */
  [[gnu::pure]]
  PixelRect AdjustForScrollbar(PixelRect rc) const noexcept;

  [[gnu::pure]]
  unsigned CalcVirtualHeight(const PixelRect &rc) const noexcept;

  void UpdateVirtualHeight(const PixelRect &rc) noexcept;

  /* virtual methods from class VScrollPanelListener */
  void OnVScrollPanelChange() noexcept override;
  bool OnVScrollPanelGesture(const char *gesture) noexcept override;
};
