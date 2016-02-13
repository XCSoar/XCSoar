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

#include "TabMenuDisplay.hpp"
#include "TabMenuData.hpp"
#include "Widget/PagerWidget.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Util/StringFormat.hpp"

#include <assert.h>

TabMenuDisplay::TabMenuDisplay(PagerWidget &_pager,
                               const DialogLook &_look)
  :pager(_pager),
   look(_look),
   dragging(false),
   drag_off_button(false),
   down_index(MenuTabIndex::None()),
   cursor(0)
{
}

void
TabMenuDisplay::InitMenu(const TabMenuGroup groups[], unsigned n_groups)
{
  assert(groups != nullptr);
  assert(n_groups > 0);
  assert(n_groups <= MAX_MAIN_MENU_ITEMS);

  main_menu_buttons.resize(n_groups);
  for (unsigned i = 0; i < n_groups; i++) {
    const auto &g = groups[i];
    auto &mb = main_menu_buttons[i];
    mb.caption = gettext(g.caption);
    mb.first_page_index = buttons.size();

    for (auto p = g.pages; p->Load != nullptr; ++p) {
      auto &page_button = buttons.append();
      page_button.main_menu_index = i;
      page_button.caption = gettext(p->menu_caption);

      Widget *w = p->Load();
      assert(w != nullptr);
      pager.Add(w);
    }

    mb.last_page_index = buttons.size() - 1;
  }
}

const TCHAR *
TabMenuDisplay::GetCaption(TCHAR buffer[], size_t size) const
{
  const unsigned page = pager.GetCurrentIndex();
  if (page >= PAGE_OFFSET) {
    const unsigned i = page - PAGE_OFFSET;
    StringFormat(buffer, size, _T("%s > %s"),
                 gettext(GetPageParentCaption(i)),
                 buttons[i].caption);
    return buffer;
  } else
    return nullptr;
}

int
TabMenuDisplay::GetPageNum(MenuTabIndex i) const
{
  assert(i.IsSub());

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

void
TabMenuDisplay::UpdateLayout()
{
  const unsigned window_width = GetWidth();
  const unsigned window_height = GetHeight();
  const unsigned border_width = GetTabLineHeight();
  const unsigned menu_button_height =
    std::min(Layout::GetMaximumControlHeight(), window_height / 7u);
  const unsigned menu_button_width = (window_width - 2 * border_width) / 2;

  const unsigned offset = Layout::Scale(2);
  const unsigned item_height = menu_button_height + border_width;

  for (unsigned main_i = 0, main_y = border_width;
       main_i < main_menu_buttons.size(); ++main_i) {
    MainMenuButton &main = main_menu_buttons[main_i];
    main.rc.left = 0;
    main.rc.right = menu_button_width;
    main.rc.top = main_y;
    main.rc.bottom = main.rc.top + menu_button_height;
    main_y = main.rc.bottom + border_width;

    main.renderer.InvalidateLayout();

    const unsigned group_height =
      item_height * main.NumSubMenus() + border_width;

    unsigned page_y = main.rc.top + offset;
    if (page_y + group_height > window_height)
      page_y = window_height - group_height - offset;

    for (unsigned page_i = main.first_page_index;
         page_i <= main.last_page_index; ++page_i) {
      SubMenuButton &page = buttons[page_i];

      page.rc.left = menu_button_width + border_width;
      page.rc.right = page.rc.left + menu_button_width;

      page.rc.top = page_y;
      page.rc.bottom = page.rc.top + menu_button_height;
      page_y = page.rc.bottom + border_width;

      page.renderer.InvalidateLayout();
    }
  }
}

inline const PixelRect &
TabMenuDisplay::GetButtonPosition(MenuTabIndex i) const
{
  assert(!i.IsNone());

  return i.IsMain()
    ? GetMainMenuButtonSize(i.main_index)
    : GetSubMenuButtonSize(GetPageNum(i));
}

TabMenuDisplay::MenuTabIndex
TabMenuDisplay::IsPointOverButton(PixelPoint Pos, unsigned mainIndex) const
{
  // scan main menu buttons
  for (unsigned i = 0; i < GetNumMainMenuItems(); i++)
    if (GetMainMenuButtonSize(i).Contains(Pos))
      return MenuTabIndex(i);


  // scan visible submenu
  if (mainIndex < GetNumMainMenuItems()) {
    const MainMenuButton &main_button = GetMainMenuButton(mainIndex);
    for (unsigned i = main_button.first_page_index;
         i <= main_button.last_page_index; ++i) {
      if (GetSubMenuButtonSize(i).Contains(Pos))
        return MenuTabIndex(mainIndex, i - main_button.first_page_index);
    }
  }

  return MenuTabIndex::None();
}

void
TabMenuDisplay::OnPageFlipped()
{
  const unsigned i = pager.GetCurrentIndex();
  if (i >= PAGE_OFFSET)
    SetCursor(i - PAGE_OFFSET);
}

void
TabMenuDisplay::SetCursor(unsigned i)
{
  if (i == cursor)
    return;

  if (IsDefined()) {
    if (SupportsPartialRedraw() &&
        GetPageMainIndex(cursor) == GetPageMainIndex(i)) {
      Invalidate(GetSubMenuButtonSize(cursor));
      Invalidate(GetSubMenuButtonSize(i));
    } else
      Invalidate();
  }

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

void
TabMenuDisplay::OnResize(PixelSize new_size)
{
  PaintWindow::OnResize(new_size);
  UpdateLayout();
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
    pager.ClickPage(PAGE_OFFSET + cursor);
    return true;

  case KEY_RIGHT:
    HighlightNext();
    return true;

  case KEY_LEFT:
    HighlightPrevious();
    return true;

  default:
    return false;
  }
}

bool
TabMenuDisplay::OnMouseDown(PixelPoint Pos)
{
  DragEnd();

  // If possible -> Give focus to the Control
  SetFocus();

  down_index = IsPointOverButton(Pos, GetPageMainIndex(cursor));

  if (!down_index.IsNone()) {
    dragging = true;
    SetCapture();

    InvalidateButton(down_index);
    return true;
  }
  return PaintWindow::OnMouseDown(Pos);
}

bool
TabMenuDisplay::OnMouseUp(PixelPoint Pos)
{
  if (dragging) {
    DragEnd();

    const MenuTabIndex di = IsPointOverButton(Pos, GetPageMainIndex(cursor));

    if (di == down_index) {

      // sub menu click
      if (di.IsSub())
        pager.ClickPage(PAGE_OFFSET + GetPageNum(di));

      // main menu click
      else if (di.IsMain()) {
        /* move cursor to first item in this menu */
        cursor = main_menu_buttons[di.main_index].first_page_index;
        Invalidate();
      } else {
        InvalidateButton(down_index);
      }
    }

    down_index = MenuTabIndex::None();

    return true;
  } else {
    return PaintWindow::OnMouseUp(Pos);
  }
}

bool
TabMenuDisplay::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (down_index.IsNone())
    return false;

  const PixelRect &rc = GetButtonPosition(down_index);
  const bool tmp = !rc.Contains(p);
  if (drag_off_button != tmp) {
    drag_off_button = tmp;
    Invalidate(rc);
  }
  return true;
}

inline void
TabMenuDisplay::PaintMainMenuBorder(Canvas &canvas) const
{
  PixelRect rc = GetMainMenuButtonSize(0);
  rc.bottom = GetMainMenuButtonSize(GetNumMainMenuItems() - 1).bottom;
  rc.Grow(GetTabLineHeight());

  canvas.DrawFilledRectangle(rc, COLOR_BLACK);
}

inline void
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas) const
{
  PaintMainMenuBorder(canvas);

  const bool is_focused = !HasCursorKeys() || HasFocus();

  unsigned main_menu_index = 0;
  for (auto i = main_menu_buttons.begin(),
         end = main_menu_buttons.end(); i != end;
       ++i, ++main_menu_index) {
    const bool isDown = main_menu_index == down_index.main_index &&
      !down_index.IsSub() && !drag_off_button;

    const bool is_selected = isDown ||
      main_menu_index == GetPageMainIndex(cursor);

    main_menu_buttons[main_menu_index].Draw(canvas, look,
                                            is_focused, isDown, is_selected);
  }
}

inline void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas,
                                   const MainMenuButton &main_button) const
{
  PixelRect rc = GetSubMenuButtonSize(main_button.first_page_index);
  rc.bottom = GetSubMenuButtonSize(main_button.last_page_index).bottom;
  rc.Grow(GetTabLineHeight());

  canvas.DrawFilledRectangle(rc, COLOR_BLACK);
}

inline void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas) const
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
    const unsigned sub_index = page_index - first_page_index;

    const bool is_pressed = sub_index == down_index.sub_index &&
      !drag_off_button;

    const bool is_cursor = page_index == cursor;
    const bool is_selected = is_pressed || is_cursor;

    buttons[page_index].Draw(canvas, look,
                             is_focused, is_pressed, is_selected);
  }
}

void
TabMenuDisplay::OnPaint(Canvas &canvas)
{
  canvas.Clear(look.background_color);

  PaintMainMenuItems(canvas);
  PaintSubMenuItems(canvas);
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
