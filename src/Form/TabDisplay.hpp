/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FORM_TAB_DISPLAY_HPP
#define XCSOAR_FORM_TAB_DISPLAY_HPP

#include "ui/window/PaintWindow.hpp"
#include "Renderer/TabRenderer.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

#include <tchar.h>

struct DialogLook;
class MaskedIcon;
class ContainerWindow;
class TabWidget;

/**
 * TabButton class holds display and callbacks data for a single tab
 */
class TabButton {
  TabRenderer renderer;

public:
  StaticString<32> caption;
  const MaskedIcon *icon;
  PixelRect rc;

public:
  TabButton(const TCHAR *_caption, const MaskedIcon *_icon) noexcept
    :icon(_icon)
  {
    caption = _caption;
  };

  void InvalidateLayout() {
    renderer.InvalidateLayout();
  }

  [[gnu::pure]]
  unsigned GetRecommendedWidth(const DialogLook &look) const noexcept;

  [[gnu::pure]]
  unsigned GetRecommendedHeight(const DialogLook &look) const noexcept;

  /**
   * Paints one button
   */
  void Draw(Canvas &canvas, const DialogLook &look,
            bool focused, bool pressed, bool selected) const noexcept {
    renderer.Draw(canvas, rc, look, caption, icon, focused, pressed, selected);
  }
};

/**
 * TabDisplay class handles onPaint callback for TabBar UI
 * and handles Mouse and key events
 * TabDisplay uses a pointer to TabBarControl
 * to show/hide the appropriate pages in the Container Class
 */
class TabDisplay final : public PaintWindow
{
  TabWidget &pager;
  const DialogLook &look;

  StaticArray<TabButton *, 32> buttons;

  bool vertical;

  bool dragging; // tracks that mouse is down and captured
  bool drag_off_button; // set by mouse_move
  unsigned down_index; // index of tab where mouse down occurred

  const unsigned tab_line_height;

public:
  TabDisplay(TabWidget &_pager, const DialogLook &look,
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

  unsigned GetSize() const noexcept {
    return buttons.size();
  }

  void Add(const TCHAR *caption, const MaskedIcon *icon=nullptr) noexcept;

  [[gnu::pure]]
  const TCHAR *GetCaption(unsigned i) const noexcept {
    return buttons[i]->caption.c_str();
  }

  /**
   * @return -1 if there is no button at the specified position
   */
  [[gnu::pure]]
  int GetButtonIndexAt(PixelPoint p) const noexcept;

private:
  void CalculateLayout() noexcept;

protected:
  void OnResize(PixelSize new_size) override;

  void OnPaint(Canvas &canvas) override;

  void OnKillFocus() override;
  void OnSetFocus() override;
  void OnCancelMode() override;

  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;

  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;

  void EndDrag() noexcept;
};

#endif
