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

#include "Form/TabMenuDisplay.hpp"
#include "Form/TabDisplay.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <winuser.h>

static unsigned
GetTabLineHeight()
{
  return Layout::Scale(1);
}

TabMenuDisplay::TabMenuDisplay(TabMenuControl& _theTabBar,
                               const DialogLook &_look,
                               ContainerWindow &parent, PixelRect rc,
                               WindowStyle style)
  :menu(_theTabBar),
   look(_look),
   dragging(false),
   drag_off_button(false),
   down_index(TabMenuControl::MenuTabIndex::None()),
   selected_index(TabMenuControl::MenuTabIndex::None())
{
  Create(parent, rc, style);
}

void
TabMenuDisplay::SetSelectedIndex(TabMenuControl::MenuTabIndex di)
{
  if (di == selected_index)
    return;

  const MainMenuButton &main_button = menu.GetMainMenuButton(di.main_index);
  if (SupportsPartialRedraw() && di.main_index == selected_index.main_index &&
      di.sub_index < main_button.NumSubMenus() &&
      selected_index.sub_index < main_button.NumSubMenus()) {
    Invalidate(menu.GetSubMenuButtonSize(main_button.first_page_index +
                                         selected_index.sub_index));
    Invalidate(menu.GetSubMenuButtonSize(main_button.first_page_index +
                                         di.sub_index));
  } else
    Invalidate();

  selected_index = di;
}

bool
TabMenuDisplay::OnKeyCheck(unsigned key_code) const
{
 switch (key_code) {

 case KEY_RETURN:
 case KEY_LEFT:
 case KEY_RIGHT:
   return true;

 default:
   return false;
 }
}

bool
TabMenuDisplay::OnKeyDown(unsigned key_code)
{
 const unsigned page = menu.GetPageNum(selected_index);

 if (menu.IsCurrentPageTheMenu()) {
   switch (key_code) {

   case KEY_RETURN:
     menu.SetCurrentPage(page);
     return true;

   case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
     menu.HighlightNextMenuItem();
     return true;

   case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
     menu.HighlightPreviousMenuItem();
     return true;
   }
 }
 return PaintWindow::OnKeyDown(key_code);
}

bool
TabMenuDisplay::OnMouseDown(PixelScalar x, PixelScalar y)
{
  DragEnd();
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  SetFocus();

  down_index = GetTabMenuBar().IsPointOverButton(Pos,
                                                selected_index.main_index);

  if (!down_index.IsNone()) {
    dragging = true;
    SetCapture();

#ifdef USE_GDI
    Invalidate(GetTabMenuBar().GetButtonPosition(down_index));
#else
    Invalidate();
#endif
    return true;
  }
  return PaintWindow::OnMouseDown(x, y);
}

bool
TabMenuDisplay::OnMouseUp(PixelScalar x, PixelScalar y)
{
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  if (dragging) {
    DragEnd();
    const TabMenuControl::MenuTabIndex di =
        GetTabMenuBar().IsPointOverButton(Pos, selected_index.main_index);

    if (di == down_index) {

      // sub menu click
      if (di.IsSub())
        GetTabMenuBar().SetCurrentPage(di);

      // main menu click
      else if (di.IsMain() && selected_index != down_index) {
        selected_index = down_index;
        Invalidate();
      } else {
#ifdef USE_GDI
        Invalidate(GetTabMenuBar().GetButtonPosition(down_index));
#else
        Invalidate();
#endif
      }
    }

    down_index = TabMenuControl::MenuTabIndex::None();

    return true;
  } else {
    return PaintWindow::OnMouseUp(x, y);
  }
}

const PixelRect&
TabMenuDisplay::GetDownButtonRC() const
{
  const TabMenuControl &tb = GetTabMenuBar();

  if (down_index.IsSub()) {
    int page = tb.GetPageNum(down_index);
    return tb.GetSubMenuButtonSize(page);
  }
  else
    return tb.GetMainMenuButtonSize(down_index.main_index);
}

bool
TabMenuDisplay::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (down_index.IsNone())
    return false;

  const PixelRect rc = GetDownButtonRC();
  const bool tmp = !rc.IsInside({x, y});
  if (drag_off_button != tmp) {
    drag_off_button = tmp;
    Invalidate(rc);
  }
  return true;
}

void
TabMenuDisplay::PaintMainMenuBorder(Canvas &canvas) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  const unsigned bwidth = GetTabLineHeight();

  const PixelRect rcFirst = tb.GetMainMenuButtonSize(0);
  const unsigned menuBottom = tb.GetMainMenuButtonSize(
      tb.GetNumMainMenuItems() - 1).bottom;
  const PixelRect rcBlackBorder(rcFirst.left - bwidth, rcFirst.top - bwidth,
                                rcFirst.right + bwidth, menuBottom + bwidth);

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas,
                                   const unsigned CaptionStyle) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  PaintMainMenuBorder(canvas);

  const bool is_focused = !HasCursorKeys() || HasFocus();

  unsigned main_menu_index = 0;
  for (auto i = tb.GetMainMenuButtons().begin(),
         end = tb.GetMainMenuButtons().end(); i != end;
       ++i, ++main_menu_index) {
    const MainMenuButton &button = **i;

    bool inverse = false;
    const bool isDown = main_menu_index == down_index.main_index &&
      !down_index.IsSub() && !drag_off_button;

    const bool is_selected = isDown ||
      main_menu_index == selected_index.main_index;

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               isDown));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           isDown));

    const PixelRect &rc = tb.GetMainMenuButtonSize(main_menu_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(button.caption), rc,
                            NULL, isDown, inverse);
  }
}

void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas,
                                   const MainMenuButton &main_button) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  const unsigned bwidth = GetTabLineHeight();
  const unsigned subTop =
    tb.GetSubMenuButtonSize(main_button.first_page_index).top;
  const PixelRect bLast = tb.GetSubMenuButtonSize(main_button.last_page_index);
  const PixelRect rcBlackBorder(bLast.left - bwidth, subTop - bwidth,
                                bLast.right + bwidth, bLast.bottom + bwidth);

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas,
                                  const unsigned CaptionStyle) const
{
  const TabMenuControl &tb = GetTabMenuBar();

  if (selected_index.IsNone())
    return;

  const MainMenuButton &main_button =
    tb.GetMainMenuButton(selected_index.main_index);

  PaintSubMenuBorder(canvas, main_button);

  assert(main_button.first_page_index < tb.GetTabButtons().size());
  assert(main_button.last_page_index < tb.GetTabButtons().size());

  const bool is_focused = !HasCursorKeys() || HasFocus();

  for (unsigned first_page_index = main_button.first_page_index,
         last_page_index = main_button.last_page_index,
         page_index = first_page_index;
       page_index <= last_page_index; ++page_index) {
    const SubMenuButton &button = tb.GetSubMenuButton(page_index);
    const unsigned sub_index = page_index - first_page_index;

    bool inverse = false;

    const bool is_pressed = sub_index == down_index.sub_index &&
      !drag_off_button;

    const bool is_selected = is_pressed ||
      sub_index == selected_index.sub_index;

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               is_pressed));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           is_pressed));

    const PixelRect &rc = tb.GetSubMenuButtonSize(page_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(button.caption), rc,
                            NULL, sub_index == selected_index.sub_index,
                            inverse);
  }
}

void
TabMenuDisplay::OnPaint(Canvas &canvas)
{
  canvas.Clear(look.background_color);
  canvas.Select(*look.button.font);

  const unsigned caption_style = DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  PaintMainMenuItems(canvas, caption_style);
  PaintSubMenuItems(canvas, caption_style);
}

void
TabMenuDisplay::OnKillFocus()
{
  Invalidate();
  PaintWindow::OnKillFocus();
}

void
TabMenuDisplay::OnSetFocus()
{
  Invalidate();
  PaintWindow::OnSetFocus();
}

void
TabMenuDisplay::DragEnd()
{
  if (dragging) {
    dragging = false;
    drag_off_button = false;
    ReleaseCapture();
  }
}
