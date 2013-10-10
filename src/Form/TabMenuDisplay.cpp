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

TabMenuDisplay::TabMenuDisplay(TabMenuControl& _theTabBar,
                               const DialogLook &_look,
                               ContainerWindow &parent, PixelRect rc,
                               WindowStyle style)
  :menu(_theTabBar),
   look(_look),
   dragging(false),
   drag_off_button(false),
   down_index(MenuTabIndex::None()),
   cursor(0)
{
  Create(parent, rc, style);
}

TabMenuDisplay::~TabMenuDisplay()
{
  for (const auto i : buttons)
    delete i;

  for (const auto i : main_menu_buttons)
    delete i;
}

void
TabMenuDisplay::InitMenu(const TCHAR *caption,
                         const TabMenuControl::PageItem pages_in[],
                         unsigned num_pages,
                         const TCHAR *main_menu_captions[],
                         unsigned num_menu_captions)
{
  pages = pages_in;

  for (unsigned i = 0; i < num_pages; ++i)
    AddMenuItem(pages_in[i].menu_caption);

  for (unsigned i = 0; i < num_menu_captions; i++) {
    unsigned first = 0;
    while (pages_in[first].main_menu_index != i) {
      ++first;
      assert(first < num_pages);
    }

    unsigned last = first + 1;
    while (last < num_pages && pages_in[last].main_menu_index == i)
      ++last;

    AddMenu(main_menu_captions[i], first, last - 1, i);
  }

  buttons.append(new SubMenuButton(caption));
}

TabMenuDisplay::MenuTabIndex
TabMenuDisplay::FindPage(unsigned page) const
{
  if (page >= GetNumPages())
    return MenuTabIndex::None();

  const unsigned main_index = pages[page].main_menu_index;
  const unsigned first_page_index =
    main_menu_buttons[main_index]->first_page_index;
  assert(page >= first_page_index);
  assert(page <= main_menu_buttons[main_index]->last_page_index);
  const unsigned sub_index = page - first_page_index;

  return MenuTabIndex(main_index, sub_index);
}

int
TabMenuDisplay::GetPageNum(MenuTabIndex i) const
{
  if (!i.IsSub())
    return menu.GetMenuPage();

  assert(i.main_index < main_menu_buttons.size());
  assert(i.sub_index < GetNumPages());

  const MainMenuButton &main_button = GetMainMenuButton(i.main_index);
  return main_button.first_page_index + i.sub_index;
}

static unsigned
GetTabLineHeight()
{
  return Layout::Scale(1);
}

unsigned
TabMenuDisplay::GetTabHeight() const
{
  return GetMenuButtonHeight() * MAX_MAIN_MENU_ITEMS + GetTabLineHeight() * 2;
}

unsigned
TabMenuDisplay::GetMenuButtonHeight() const
{
  return std::min(Layout::GetMaximumControlHeight(),
                  unsigned(GetHeight()) / 7u);
}

unsigned
TabMenuDisplay::GetMenuButtonWidth() const
{
  return (GetWidth() - GetTabLineHeight()) / 2;
}

const PixelRect&
TabMenuDisplay::GetMainMenuButtonSize(unsigned i) const
{
  assert(i < main_menu_buttons.size());

  if (main_menu_buttons[i]->rc.left < main_menu_buttons[i]->rc.right)
    return main_menu_buttons[i]->rc;
  PixelRect &rc = main_menu_buttons[i]->rc;
  const unsigned margin = Layout::Scale(1);
  const unsigned finalmargin = Layout::Scale(1);
  const unsigned butHeight = GetMenuButtonHeight();
  rc.top = finalmargin + (margin + butHeight) * i;
  rc.bottom = rc.top + butHeight;

  rc.left = 0;
  rc.right = GetMenuButtonWidth();

  return main_menu_buttons[i]->rc;
}

const PixelRect&
TabMenuDisplay::GetSubMenuButtonSize(unsigned page) const
{
  assert(page < buttons.size());

  if (buttons[page]->rc.left < buttons[page]->rc.right)
    return buttons[page]->rc;

  const TabMenuControl::PageItem &item = GetPageItem(page);
  const MainMenuButton &main_button = GetMainMenuButton(item.main_menu_index);
  const unsigned sub_index = page - main_button.first_page_index;

  PixelRect &rc = buttons[page]->rc;

  const unsigned margin = Layout::Scale(1);
  const unsigned finalmargin = Layout::Scale(1);
  const unsigned subMenuItemCount = main_button.NumSubMenus();
  const unsigned tabHeight = GetTabHeight();
  const unsigned butHeight = GetMenuButtonHeight();
  const unsigned itemHeight = butHeight + margin;
  const unsigned SubMenuHeight = itemHeight * subMenuItemCount + finalmargin;
  const unsigned topMainMenuItem = item.main_menu_index * itemHeight +
      finalmargin;
  const unsigned offset = Layout::Scale(2);
  const unsigned topMainMenuItemWOffset = topMainMenuItem + offset;
  const unsigned subMenuTop =
      (topMainMenuItemWOffset + SubMenuHeight <= tabHeight) ?
       topMainMenuItemWOffset : tabHeight - SubMenuHeight - offset;

  rc.top = subMenuTop + sub_index * itemHeight;
  rc.bottom = rc.top + butHeight;

  rc.left = GetMenuButtonWidth() + GetTabLineHeight();
  rc.right = rc.left + GetMenuButtonWidth();

  return buttons[page]->rc;
}

const PixelRect &
TabMenuDisplay::GetButtonPosition(MenuTabIndex i) const
{
  assert(!i.IsNone());

  return i.IsMain()
    ? GetMainMenuButtonSize(i.main_index)
    : GetSubMenuButtonSize(GetPageNum(i));
}

TabMenuDisplay::MenuTabIndex
TabMenuDisplay::IsPointOverButton(RasterPoint Pos, unsigned mainIndex) const
{
  // scan main menu buttons
  for (unsigned i = 0; i < GetNumMainMenuItems(); i++)
    if (GetMainMenuButtonSize(i).IsInside(Pos))
      return MenuTabIndex(i);


  // scan visible submenu
  if (mainIndex < GetNumMainMenuItems()) {
    const MainMenuButton &main_button = GetMainMenuButton(mainIndex);
    for (unsigned i = main_button.first_page_index;
         i <= main_button.last_page_index; ++i) {
      if (GetSubMenuButtonSize(i).IsInside(Pos))
        return MenuTabIndex(mainIndex, i - main_button.first_page_index);
    }
  }

  return MenuTabIndex::None();
}

void
TabMenuDisplay::SetCursor(unsigned i)
{
  if (i == cursor)
    return;

  if (SupportsPartialRedraw() &&
      GetPageMainIndex(cursor) == GetPageMainIndex(i)) {
    Invalidate(GetSubMenuButtonSize(cursor));
    Invalidate(GetSubMenuButtonSize(i));
  } else
    Invalidate();

  cursor = i;
}

inline bool
TabMenuDisplay::HighlightNext()
{
  const unsigned i = cursor + 1;
  if (i >= GetNumPages())
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
  assert(i < GetNumPages());

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

  const MenuTabIndex selected_index = FindPage(cursor);

  down_index = IsPointOverButton(Pos, selected_index.main_index);

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

    const MenuTabIndex selected_index = FindPage(cursor);
    const MenuTabIndex di = IsPointOverButton(Pos, selected_index.main_index);

    if (di == down_index) {

      // sub menu click
      if (di.IsSub())
        GetTabMenuBar().SetCurrentPage(GetPageNum(di));

      // main menu click
      else if (di.IsMain() && selected_index != down_index) {
        /* move cursor to first item in this menu */
        const MenuTabIndex first(di.main_index, 0);
        cursor = GetPageNum(first);
        Invalidate();
      } else {
        InvalidateButton(down_index);
      }
    }

    down_index = MenuTabIndex::None();

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

  const PixelRect rc = GetButtonPosition(down_index);
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
  const unsigned bwidth = GetTabLineHeight();

  const PixelRect rcFirst = GetMainMenuButtonSize(0);
  const unsigned menuBottom =
    GetMainMenuButtonSize(GetNumMainMenuItems() - 1).bottom;
  const PixelRect rcBlackBorder(rcFirst.left - bwidth, rcFirst.top - bwidth,
                                rcFirst.right + bwidth, menuBottom + bwidth);

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas,
                                   const unsigned CaptionStyle) const
{
  PaintMainMenuBorder(canvas);

  const bool is_focused = !HasCursorKeys() || HasFocus();

  unsigned main_menu_index = 0;
  for (auto i = GetMainMenuButtons().begin(),
         end = GetMainMenuButtons().end(); i != end;
       ++i, ++main_menu_index) {
    const MainMenuButton &button = **i;

    bool inverse = false;
    const bool isDown = main_menu_index == down_index.main_index &&
      !down_index.IsSub() && !drag_off_button;

    const bool is_selected = isDown ||
      main_menu_index == GetPageMainIndex(cursor);

    canvas.SetTextColor(look.list.GetTextColor(is_selected, is_focused,
                                               isDown));
    canvas.SetBackgroundColor(look.list.GetBackgroundColor(is_selected,
                                                           is_focused,
                                                           isDown));

    const PixelRect &rc = GetMainMenuButtonSize(main_menu_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(button.caption), rc,
                            NULL, isDown, inverse);
  }
}

void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas,
                                   const MainMenuButton &main_button) const
{
  const unsigned bwidth = GetTabLineHeight();
  const unsigned subTop =
    GetSubMenuButtonSize(main_button.first_page_index).top;
  const PixelRect bLast = GetSubMenuButtonSize(main_button.last_page_index);
  const PixelRect rcBlackBorder(bLast.left - bwidth, subTop - bwidth,
                                bLast.right + bwidth, bLast.bottom + bwidth);

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas,
                                  const unsigned CaptionStyle) const
{
  const MainMenuButton &main_button =
    GetMainMenuButton(GetPageMainIndex(cursor));

  PaintSubMenuBorder(canvas, main_button);

  assert(main_button.first_page_index < buttons.size());
  assert(main_button.last_page_index < buttons.size());

  const bool is_focused = !HasCursorKeys() || HasFocus();

  for (unsigned first_page_index = main_button.first_page_index,
         last_page_index = main_button.last_page_index,
         page_index = first_page_index;
       page_index <= last_page_index; ++page_index) {
    const SubMenuButton &button = GetSubMenuButton(page_index);
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

    const PixelRect &rc = GetSubMenuButtonSize(page_index);
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
