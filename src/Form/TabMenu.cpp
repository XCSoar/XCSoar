/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Form/TabMenu.hpp"
#include "Form/TabDisplay.hpp"
#include "Form/Form.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/PaintWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/XML.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <winuser.h>

TabMenuControl::TabMenuControl(ContainerWindow &_parent,
                               WndForm &_form,
                               const CallBackTableEntry *_look_up_table,
                               const DialogLook &look, const TCHAR * _caption,
                               PixelScalar x, PixelScalar y,
                               UPixelScalar _width, UPixelScalar _height,
                               const WindowStyle style)
  :last_content(MenuTabIndex::None()),
   caption(_caption),
   setting_up(true),
   form(_form),
   look_up_table(_look_up_table)

{
  set(_parent, x, 0, _parent.get_width() - x, _parent.get_height(), style);

  const PixelRect rc = get_client_rect();
  WindowStyle pager_style;
  pager_style.ControlParent();
  pager.set(*this, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            pager_style);

  tab_display = new TabMenuDisplay(*this, look, pager,
                                     0, y, _width, _height);
}

TabMenuControl::~TabMenuControl()
{
  for (auto i = buttons.begin(), end = buttons.end(); i != end; ++i)
    delete *i;

  for (auto i = main_menu_buttons.begin(), end = main_menu_buttons.end();
       i != end; ++i)
    delete *i;

  delete tab_display;
}

void
TabMenuControl::NextPage()
{
  const unsigned NumAllPages = pager.GetTabCount();
  if (NumAllPages < 2)
    return;
  SetCurrentPage((GetCurrentPage() + 1) % NumAllPages);
}

void
TabMenuControl::PreviousPage()
{
  const unsigned NumAllPages = pager.GetTabCount();

  if (NumAllPages < 2)
    return;
  SetCurrentPage((GetCurrentPage() + NumAllPages - 1) % NumAllPages);
}

void
TabMenuControl::HighlightNextMenuItem()
{
  const unsigned page = GetPageNum(GetTabMenuDisplay()->GetSelectedIndex());

 if (page + 1 < GetNumPages()) // don't wrap
   SetLastContentPage(page + 1);
 else
   if (page >= GetNumPages()) // initial state is menu (page == NunPages)
     SetLastContentPage(0);
}

void
TabMenuControl::HighlightPreviousMenuItem()
{
 const unsigned page = GetPageNum(GetTabMenuDisplay()->GetSelectedIndex());
 if (page > 0) {
   SetLastContentPage(page - 1);
 }
}

void
TabMenuControl::SetCurrentPage(TabMenuControl::MenuTabIndex menuIndex)
{
  assert(!menuIndex.IsNone());

  SetCurrentPage(GetPageNum(menuIndex));
}

void
TabMenuControl::SetCurrentPage(unsigned page)
{
  assert(page < buttons.size());

  if (!pager.ClickPage(page)) {
    assert(!setting_up);
    return;
  }

  setting_up = false;

  if (page == GetMenuPage()) {
    form.SetCaption(caption);
    const MenuTabIndex di = last_content;
    this->GetTabMenuDisplay()->SetSelectedIndex(di);

  } else {
    const PageItem& theitem = GetPageItem(page);
    SetLastContentPage(page);
    const OneMainMenuButton &main_button =
      GetMainMenuButton(last_content.main_index);
    StaticString<128> caption;
    caption.Format(_T("%s > %s"),
                   gettext(main_button.caption),
                   gettext(theitem.menu_caption));
    form.SetCaption(caption);
  }
}

void TabMenuControl::SetLastContentPage(unsigned page)
{
  last_content = FindPage(page);
  GetTabMenuDisplay()->SetSelectedIndex(last_content);
}

int
TabMenuControl::GetPageNum(MenuTabIndex i) const
{
  if (!i.IsSub())
    return this->GetMenuPage();

  assert(i.main_index < main_menu_buttons.size());
  assert(i.sub_index < GetNumPages());

  const OneMainMenuButton &main_button = GetMainMenuButton(i.main_index);
  return main_button.first_page_index + i.sub_index;
}

static UPixelScalar
GetTabLineHeight()
{
  return Layout::Scale(1);
}

UPixelScalar
TabMenuControl::GetTabHeight() const
{
  return GetMenuButtonHeight() * TabMenuControl::MAX_MAIN_MENU_ITEMS
      + GetTabLineHeight() * 2;
}

UPixelScalar
TabMenuControl::GetMenuButtonHeight() const
{
  return Layout::Scale(31);
}

UPixelScalar
TabMenuControl::GetButtonVerticalOffset() const
{
  if (!Layout::landscape) {
    PixelRect rc = get_client_rect();
    const UPixelScalar spaceUnderButtons = Layout::Scale(80);
    return (rc.bottom - spaceUnderButtons - GetTabHeight()) / 2;
  }
  else
    return 0;
}

UPixelScalar
TabMenuControl::GetMenuButtonWidth() const
{
  return (tab_display->GetTabWidth() - GetTabLineHeight()) / 2;
}

const PixelRect&
TabMenuControl::GetMainMenuButtonSize(unsigned i) const
{
  assert(i < main_menu_buttons.size());

  if (main_menu_buttons[i]->but_size.left < main_menu_buttons[i]->but_size.right)
    return main_menu_buttons[i]->but_size;
  PixelRect &rc = main_menu_buttons[i]->but_size;
  const UPixelScalar margin = Layout::Scale(1);
  const UPixelScalar finalmargin = Layout::Scale(1);
  const UPixelScalar butHeight = GetMenuButtonHeight();
  rc.top = finalmargin + (margin + butHeight) * i;
  rc.top += GetButtonVerticalOffset();
  rc.bottom = rc.top + butHeight;

  rc.left = 0;
  rc.right = GetMenuButtonWidth();

  return main_menu_buttons[i]->but_size;
}

const PixelRect&
TabMenuControl::GetSubMenuButtonSize(unsigned page) const
{
  assert(page < buttons.size());

  if (buttons[page]->but_size.left < buttons[page]->but_size.right)
    return buttons[page]->but_size;

  const PageItem &item = this->GetPageItem(page);
  const OneMainMenuButton &main_button = GetMainMenuButton(item.main_menu_index);
  const unsigned sub_index = page - main_button.first_page_index;

  PixelRect &rc = buttons[page]->but_size;

  const UPixelScalar margin = Layout::Scale(1);
  const UPixelScalar finalmargin = Layout::Scale(1);
  const unsigned subMenuItemCount = main_button.NumSubMenus();
  const UPixelScalar tabHeight = GetTabHeight();
  const UPixelScalar butHeight = GetMenuButtonHeight();
  const UPixelScalar itemHeight = butHeight + margin;
  const UPixelScalar SubMenuHeight = itemHeight * subMenuItemCount + finalmargin;
  const UPixelScalar topMainMenuItem = item.main_menu_index * itemHeight +
      finalmargin;
  const UPixelScalar offset = Layout::Scale(2);
  const UPixelScalar topMainMenuItemWOffset = topMainMenuItem + offset;
  const UPixelScalar subMenuTop =
      (topMainMenuItemWOffset + SubMenuHeight <= tabHeight) ?
       topMainMenuItemWOffset : tabHeight - SubMenuHeight - offset;

  rc.top = subMenuTop + sub_index * itemHeight;
  rc.top += GetButtonVerticalOffset();
  rc.bottom = rc.top + butHeight;

  rc.left = GetMenuButtonWidth() + GetTabLineHeight();
  rc.right = rc.left + GetMenuButtonWidth();

  return buttons[page]->but_size;
}

TabMenuControl::MenuTabIndex
TabMenuControl::IsPointOverButton(RasterPoint Pos, unsigned mainIndex) const
{
  // scan main menu buttons
  for (unsigned i = 0; i < GetNumMainMenuItems(); i++)
    if (PtInRect(&GetMainMenuButtonSize(i), Pos))
      return MenuTabIndex(i);


  // scan visible submenu
  if (mainIndex < GetNumMainMenuItems()) {
    const OneMainMenuButton &main_button = GetMainMenuButton(mainIndex);
    for (unsigned i = main_button.first_page_index;
         i <= main_button.last_page_index; ++i) {
      if (PtInRect(&GetSubMenuButtonSize(i), Pos))
        return MenuTabIndex(mainIndex, i - main_button.first_page_index);
    }
  }

  return MenuTabIndex::None();
}

void
TabMenuControl::CreateSubMenuItem(const PageItem &item)
{
  assert(item.main_menu_index < MAX_MAIN_MENU_ITEMS);

  assert(item.Load != NULL);

  Widget *widget = item.Load();
  pager.AddPage(widget);

  OneSubMenuButton *b =
    new OneSubMenuButton(item.menu_caption);
  buttons.append(b);
}

void
TabMenuControl::CreateSubMenu(const PageItem pages_in[], unsigned NumPages,
                              const TCHAR *main_menu_caption,
                              const unsigned main_menu_index)
{
  assert(main_menu_index < MAX_MAIN_MENU_ITEMS);
  unsigned firstPageIndex = LARGE_VALUE;
  unsigned subMenuIndex = 0;

  for (unsigned i = 0; i < NumPages; i++) {
    const PageItem& item = pages_in[i];
    if (item.main_menu_index == main_menu_index) {
      CreateSubMenuItem(item);
      firstPageIndex = min(i, firstPageIndex);
      subMenuIndex++;
    }
  }
  OneMainMenuButton *b =
      new OneMainMenuButton(main_menu_caption, firstPageIndex,
                            firstPageIndex + subMenuIndex - 1);
  main_menu_buttons.append(b);
}

void
TabMenuControl::InitMenu(const PageItem pages_in[],
                         unsigned num_pages,
                         const TCHAR *main_menu_captions[],
                         unsigned num_menu_captions)
{
  assert(pages_in);
  assert(main_menu_captions);

  pages = pages_in;

  for (unsigned i = 0; i < num_menu_captions; i++)
    CreateSubMenu(pages_in, num_pages, main_menu_captions[i], i);

  pager.AddClient(tab_display);
  buttons.append(new OneSubMenuButton(caption));

  assert(GetNumPages() == num_pages);
}

TabMenuControl::MenuTabIndex
TabMenuControl::FindPage(unsigned page) const
{
  for (auto begin = main_menu_buttons.begin(),
         end = main_menu_buttons.end(), i = begin;
       i != end; ++i) {
    const OneMainMenuButton &button = **i;
    if (page >= button.first_page_index && page <= button.last_page_index)
      return MenuTabIndex(std::distance(begin, i),
                          page - button.first_page_index);
  }

  return MenuTabIndex::None();
}

unsigned
TabMenuControl::GotoMenuPage()
{
  SetCurrentPage(GetMenuPage());
  return GetMenuPage();
}

void
TabMenuControl::FocusMenuPage()
{
  GotoMenuPage();
  tab_display->set_focus();
}

// TabMenuDisplay Functions
TabMenuDisplay::TabMenuDisplay(TabMenuControl& _theTabBar,
                               const DialogLook &_look,
                               ContainerWindow &parent,
                               PixelScalar left, PixelScalar top,
                               UPixelScalar width, UPixelScalar height)
  :menu(_theTabBar),
   look(_look),
   dragging(false),
   drag_off_button(false),
   down_index(TabMenuControl::MenuTabIndex::None()),
   selected_index(TabMenuControl::MenuTabIndex::None())
{
  WindowStyle mystyle;
  mystyle.TabStop();
  set(parent, left, top, width, height, mystyle);
}

void
TabMenuDisplay::SetSelectedIndex(TabMenuControl::MenuTabIndex di)
{
  if (di == selected_index)
    return;

  const OneMainMenuButton &main_button = menu.GetMainMenuButton(di.main_index);
  if (SupportsPartialRedraw() && di.main_index == selected_index.main_index &&
      di.sub_index < main_button.NumSubMenus() &&
      selected_index.sub_index < main_button.NumSubMenus()) {
    invalidate(menu.GetSubMenuButtonSize(main_button.first_page_index +
                                         selected_index.sub_index));
    invalidate(menu.GetSubMenuButtonSize(main_button.first_page_index +
                                         di.sub_index));
  } else
    invalidate();

  selected_index = di;
}

bool
TabMenuDisplay::OnKeyCheck(unsigned key_code) const
{
 switch (key_code) {

 case VK_RETURN:
 case VK_LEFT:
 case VK_RIGHT:
   return menu.IsCurrentPageTheMenu();

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

   case VK_RETURN:
     menu.SetCurrentPage(page);
     return true;

   case VK_RIGHT:
     menu.HighlightNextMenuItem();
     return true;

   case VK_LEFT:
     menu.HighlightPreviousMenuItem();
     return true;
   }
 }
 return PaintWindow::OnKeyDown(key_code);
}

bool
TabMenuDisplay::OnMouseDown(PixelScalar x, PixelScalar y)
{
  drag_end();
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  // If possible -> Give focus to the Control
  set_focus();

  down_index = GetTabMenuBar().IsPointOverButton(Pos,
                                                selected_index.main_index);

  if (!down_index.IsNone()) {
    dragging = true;
    set_capture();
    invalidate();
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
    drag_end();
    const TabMenuControl::MenuTabIndex di =
        GetTabMenuBar().IsPointOverButton(Pos, selected_index.main_index);

    if (di == down_index) {

      // sub menu click
      if (di.IsSub())
        GetTabMenuBar().SetCurrentPage(di);

      // main menu click
      else if (di.IsMain())
        selected_index = down_index;
      invalidate();
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
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  const bool tmp = !PtInRect(&rc, Pos);
  if (drag_off_button != tmp) {
    drag_off_button = tmp;
    invalidate(rc);
  }
  return true;
}

void
TabMenuDisplay::PaintMainMenuBorder(Canvas &canvas) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  const UPixelScalar bwidth = GetTabLineHeight();

  const PixelRect rcFirst = tb.GetMainMenuButtonSize(0);
  const UPixelScalar menuBottom = tb.GetMainMenuButtonSize(
      tb.GetNumMainMenuItems() - 1).bottom;
  const PixelRect rcBlackBorder = { PixelScalar(rcFirst.left - bwidth),
                                    PixelScalar(rcFirst.top - bwidth),
                                    PixelScalar(rcFirst.right + bwidth),
                                    PixelScalar(menuBottom + bwidth) };

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas,
                                   const unsigned CaptionStyle) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  PaintMainMenuBorder(canvas);

  const bool is_focused = has_focus();

  unsigned main_menu_index = 0;
  for (auto i = tb.GetMainMenuButtons().begin(),
         end = tb.GetMainMenuButtons().end(); i != end;
       ++i, ++main_menu_index) {
    const OneMainMenuButton &button = **i;

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

    const PixelRect rc = tb.GetMainMenuButtonSize(main_menu_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(button.caption), rc,
                            false, NULL, isDown, inverse);
  }
}

void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas,
                                   const OneMainMenuButton &main_button) const
{
  const TabMenuControl &tb = GetTabMenuBar();
  const UPixelScalar bwidth =  GetTabLineHeight();
  const UPixelScalar subTop =
    tb.GetSubMenuButtonSize(main_button.first_page_index).top;
  const PixelRect bLast = tb.GetSubMenuButtonSize(main_button.last_page_index);
  const PixelRect rcBlackBorder = { PixelScalar(bLast.left - bwidth),
                                    PixelScalar(subTop - bwidth),
                                    PixelScalar(bLast.right + bwidth),
                                    PixelScalar(bLast.bottom + bwidth) };

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas,
                                  const unsigned CaptionStyle) const
{
  const TabMenuControl &tb = GetTabMenuBar();

  if (selected_index.IsNone())
    return;

  const OneMainMenuButton &main_button =
    tb.GetMainMenuButton(selected_index.main_index);

  PaintSubMenuBorder(canvas, main_button);

  assert(main_button.first_page_index < tb.GetTabButtons().size());
  assert(main_button.last_page_index < tb.GetTabButtons().size());

  const bool is_focused = has_focus();

  for (unsigned first_page_index = main_button.first_page_index,
         last_page_index = main_button.last_page_index,
         page_index = first_page_index;
       page_index <= last_page_index; ++page_index) {
    const OneSubMenuButton &button = tb.GetSubMenuButton(page_index);
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
                            false, NULL,
                            sub_index == selected_index.sub_index,
                            inverse);
  }
}

void
TabMenuDisplay::OnPaint(Canvas &canvas)
{
  canvas.clear(look.background_color);
  canvas.Select(*look.button.font);

  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  PaintMainMenuItems(canvas, CaptionStyle);
  PaintSubMenuItems(canvas, CaptionStyle);
}

void
TabMenuDisplay::OnKillFocus()
{
  invalidate();
  PaintWindow::OnKillFocus();
}

void
TabMenuDisplay::OnSetFocus()
{
  invalidate();
  PaintWindow::OnSetFocus();
}

void
TabMenuDisplay::drag_end()
{
  if (dragging) {
    dragging = false;
    drag_off_button = false;
    release_capture();
  }
}
