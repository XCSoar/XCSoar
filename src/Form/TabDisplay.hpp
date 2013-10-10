/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/StaticArray.hpp"
#include "Util/StaticString.hpp"

#include <tchar.h>

struct DialogLook;
class Bitmap;
class ContainerWindow;
class TabBarControl;

/**
 * TabButton class holds display and callbacks data for a single tab
 */
class TabButton {
public:
  StaticString<32> caption;
  const Bitmap *bitmap;
  PixelRect rc;

public:
  TabButton(const TCHAR* _caption, const Bitmap *_bitmap)
    :bitmap(_bitmap)
  {
    caption = _caption;
    InvalidateLayout();
  };

  void InvalidateLayout() {
    rc.left = rc.right = 0;
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
  TabBarControl& tab_bar;
  const DialogLook &look;

  StaticArray<TabButton *, 32> buttons;

  bool vertical;

  bool dragging; // tracks that mouse is down and captured
  bool drag_off_button; // set by mouse_move
  unsigned down_index; // index of tab where mouse down occurred

  const UPixelScalar tab_line_height;

public:
  /**
   *
   * @param parent
   * @param _theTabBar. An existing TabBar object
   */
  TabDisplay(TabBarControl& _theTabBar, const DialogLook &look,
             ContainerWindow &parent, PixelRect rc,
             bool vertical);

  virtual ~TabDisplay();

  const DialogLook &GetLook() const {
    return look;
  }

  bool IsVertical() const {
    return vertical;
  }

  void UpdateLayout(const PixelRect &rc, bool _vertical);

  /**
   * Paints one button
   */
  static void PaintButton(Canvas &canvas, const unsigned CaptionStyle,
                          const TCHAR *caption, const PixelRect &rc,
                          const Bitmap *bmp, const bool isDown, bool inverse);

  unsigned GetSize() const {
    return buttons.size();
  }

  void Add(const TCHAR *caption, const Bitmap *bmp=nullptr);

  gcc_pure
  const TCHAR *GetCaption(unsigned i) const {
    return buttons[i]->caption.c_str();
  }

  /**
   * @return -1 if there is no button at the specified position
   */
  gcc_pure
  int GetButtonIndexAt(RasterPoint p) const;

private:
  /**
   * calculates the size and position of ith button
   * works in landscape or portrait mode
   * @param i index of button
   * @return Rectangle of button coordinates
   */
  gcc_pure
  const PixelRect &GetButtonSize(unsigned i) const;

protected:
  virtual void OnResize(PixelSize new_size) override;

  virtual void OnPaint(Canvas &canvas) override;

  virtual void OnKillFocus() override;
  virtual void OnSetFocus() override;
  virtual void OnCancelMode() override;

  virtual bool OnKeyCheck(unsigned key_code) const override;
  virtual bool OnKeyDown(unsigned key_code) override;

  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y,
                           unsigned keys) override;

  void EndDrag();
};

#endif
