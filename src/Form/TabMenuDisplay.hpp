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

#ifndef XCSOAR_FORM_TABMENU_DISPLAY_HPP
#define XCSOAR_FORM_TABMENU_DISPLAY_HPP

#include "TabMenu.hpp"
#include "Screen/PaintWindow.hpp"

struct DialogLook;
class ContainerWindow;

class TabMenuDisplay final : public PaintWindow
{
  TabMenuControl &menu;
  const DialogLook &look;
  bool dragging; // tracks that mouse is down and captured
  bool drag_off_button; // set by mouse_move

  /* used to track mouse down/up clicks */
  TabMenuControl::MenuTabIndex down_index;
  /* used to render which submenu is drawn and which item is highlighted */
  TabMenuControl::MenuTabIndex selected_index;

public:
  TabMenuDisplay(TabMenuControl &_menu, const DialogLook &look,
                 ContainerWindow &parent, PixelRect rc, WindowStyle style);

  void SetSelectedIndex(TabMenuControl::MenuTabIndex di);

private:
  bool HighlightNext();
  bool HighlightPrevious();

public:
  /**
   * Returns index of selected (highlighted) tab
   * @return
   */
  const TabMenuControl::MenuTabIndex GetSelectedIndex() { return selected_index; }

private:
  TabMenuControl &GetTabMenuBar() {
    return menu;
  }

  const TabMenuControl &GetTabMenuBar() const {
    return menu;
  }

  void DragEnd();

  /**
   * @return Rect of button holding down pointer capture
   */
  gcc_pure
  const PixelRect &GetDownButtonRC() const;

protected:
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y,
                           unsigned keys) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnKeyCheck(unsigned key_code) const override;
  virtual bool OnKeyDown(unsigned key_code) override;

  /**
   * canvas is the tabmenu which is the full content window, no content
   * @param canvas
   * Todo: support icons and "ButtonOnly" style
   */
  virtual void OnPaint(Canvas &canvas) override;

  virtual void OnKillFocus() override;
  virtual void OnSetFocus() override;

private:
  /**
   * draw border around main menu
   */
  void PaintMainMenuBorder(Canvas &canvas) const;
  void PaintMainMenuItems(Canvas &canvas, const unsigned CaptionStyle) const;
  void PaintSubMenuBorder(Canvas &canvas,
                          const MainMenuButton &main_button) const;
  void PaintSubMenuItems(Canvas &canvas, const unsigned CaptionStyle) const;
};

#endif
