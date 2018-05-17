/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/PaintWindow.hpp"
#include "Renderer/TabRenderer.hpp"
#include "Util/StaticArray.hxx"
#include "Util/StaticString.hxx"

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
  TabButton(const TCHAR *_caption, const MaskedIcon *_icon)
    :icon(_icon)
  {
    caption = _caption;
  };

  void InvalidateLayout() {
    renderer.InvalidateLayout();
  }

  gcc_pure
  unsigned GetRecommendedWidth(const DialogLook &look) const;

  gcc_pure
  unsigned GetRecommendedHeight(const DialogLook &look) const;

  /**
   * Paints one button
   */
  void Draw(Canvas &canvas, const DialogLook &look,
            bool focused, bool pressed, bool selected) const {
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
             WindowStyle style=WindowStyle());

  virtual ~TabDisplay();

  const DialogLook &GetLook() const {
    return look;
  }

  gcc_pure
  unsigned GetRecommendedColumnWidth() const;

  gcc_pure
  unsigned GetRecommendedRowHeight() const;

  bool IsVertical() const {
    return vertical;
  }

  void UpdateLayout(const PixelRect &rc, bool _vertical);

  unsigned GetSize() const {
    return buttons.size();
  }

  void Add(const TCHAR *caption, const MaskedIcon *icon=nullptr);

  gcc_pure
  const TCHAR *GetCaption(unsigned i) const {
    return buttons[i]->caption.c_str();
  }

  /**
   * @return -1 if there is no button at the specified position
   */
  gcc_pure
  int GetButtonIndexAt(PixelPoint p) const;

private:
  void CalculateLayout();

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

  void EndDrag();
};

#endif
