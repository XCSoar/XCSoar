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
   cursor(0)
{
  Create(parent, rc, style);
}

void
TabMenuDisplay::SetCursor(unsigned i)
{
  if (i == cursor)
    return;

  if (SupportsPartialRedraw() &&
      menu.GetPageMainIndex(cursor) == menu.GetPageMainIndex(i)) {
    Invalidate(menu.GetSubMenuButtonSize(cursor));
    Invalidate(menu.GetSubMenuButtonSize(i));
  } else
    Invalidate();

  cursor = i;
}

inline bool
TabMenuDisplay::HighlightNext()
{
  const unsigned i = cursor + 1;
  if (i >= menu.GetNumPages())
    return false;

  SetCursor(i);
  return true;
}

inline bool
TabMenuDisplay::HighlightPrevious()
{
  if (cursor == 0)
    return false;

  const unsigned i = cursor - 1;
  assert(i < menu.GetNumPages());

  SetCursor(i);
  return true;
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
  switch (key_code) {
  case KEY_RETURN:
    menu.SetCurrentPage(cursor);
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    HighlightNext();
    return true;

  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    HighlightPrevious();
    return true;

  default:
    return false;
  }
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

  const TabMenuControl::MenuTabIndex selected_index = menu.FindPage(cursor);

  down_index = GetTabMenuBar().IsPointOverButton(Pos,
                                                selected_index.main_index);

  if (!down_index.IsNone()) {
    dragging = true;
    SetCapture();

    InvalidateButton(down_index);
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

    const TabMenuControl::MenuTabIndex selected_index = menu.FindPage(cursor);
    const TabMenuControl::MenuTabIndex di =
        GetTabMenuBar().IsPointOverButton(Pos, selected_index.main_index);

    if (di == down_index) {

      // sub menu click
      if (di.IsSub())
        GetTabMenuBar().SetCurrentPage(di);

      // main menu click
      else if (di.IsMain() && selected_index != down_index) {
        /* move cursor to first item in this menu */
        const TabMenuControl::MenuTabIndex first(di.main_index, 0);
        cursor = menu.GetPageNum(first);
        Invalidate();
      } else {
        InvalidateButton(down_index);
      }
    }

    down_index = TabMenuControl::MenuTabIndex::None();

    return true;
  } else {
    return PaintWindow::OnMouseUp(x, y);
  }
}

bool
TabMenuDisplay::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (down_index.IsNone())
    return false;

  const PixelRect rc = GetTabMenuBar().GetButtonPosition(down_index);
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
      main_menu_index == menu.GetPageMainIndex(cursor);

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

  const MainMenuButton &main_button =
    tb.GetMainMenuButton(menu.GetPageMainIndex(cursor));

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

    const bool is_cursor = page_index == cursor;
    const bool is_selected = is_pressed || is_cursor;

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               is_pressed));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           is_pressed));

    const PixelRect &rc = tb.GetSubMenuButtonSize(page_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(button.caption), rc,
                            NULL, is_cursor,
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
