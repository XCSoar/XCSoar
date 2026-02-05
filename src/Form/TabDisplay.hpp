// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "util/StaticArray.hxx"

#include <tchar.h>

struct DialogLook;
class MaskedIcon;
class ContainerWindow;
class TabHandler;

/**
 * TabDisplay class handles onPaint callback for TabBar UI
 * and handles Mouse and key events
 * TabDisplay uses a pointer to TabBarControl
 * to show/hide the appropriate pages in the Container Class
 */
class TabDisplay final : public PaintWindow
{
  TabHandler &handler;
  const DialogLook &look;

  class Button;
  StaticArray<Button *, 32> buttons;

  const unsigned tab_line_height;

  unsigned current_index = 0;

  unsigned down_index; // index of tab where mouse down occurred

  bool vertical;

  bool dragging = false; // tracks that mouse is down and captured
  bool drag_off_button; // set by mouse_move

public:
  TabDisplay(TabHandler &_handler, const DialogLook &look,
             ContainerWindow &parent, PixelRect rc,
             bool vertical,
             WindowStyle style=WindowStyle()) noexcept;

  ~TabDisplay() noexcept override;

  const DialogLook &GetLook() const noexcept {
    return look;
  }

  [[gnu::pure]]
  unsigned GetRecommendedColumnWidth() const noexcept;

  [[gnu::pure]]
  unsigned GetRecommendedRowHeight() const noexcept;

  bool IsVertical() const noexcept {
    return vertical;
  }

  void UpdateLayout(const PixelRect &rc, bool _vertical) noexcept;

  void Add(const char *caption, const MaskedIcon *icon=nullptr) noexcept;

  [[gnu::pure]]
  const char *GetCaption(unsigned i) const noexcept;

  void SetCurrentIndex(unsigned i) noexcept {
    if (i == current_index)
      return;

    current_index = i;
    Invalidate();
  }

private:
  /**
   * @return -1 if there is no button at the specified position
   */
  [[gnu::pure]]
  int GetButtonIndexAt(PixelPoint p) const noexcept;

  void CalculateLayout() noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;

  void OnPaint(Canvas &canvas) noexcept override;

  void OnKillFocus() noexcept override;
  void OnSetFocus() noexcept override;
  void OnCancelMode() noexcept override;

  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;

  void EndDrag() noexcept;
};
