/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  pager_style.control_parent();
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

 if (page < GetNumPages() - 1) // don't wrap
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

const OneMainMenuButton *
TabMenuControl::GetMainMenuButton(unsigned main_menu_index) const
{
  if (main_menu_index >= main_menu_buttons.size())
    return NULL;
  else
    return main_menu_buttons[main_menu_index];
}

const OneSubMenuButton *
TabMenuControl::GetSubMenuButton(unsigned page) const
{
  assert(page < GetNumPages() && page < buttons.size());
  if (page >= buttons.size())
    return NULL;
  else
    return buttons[page];
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
    const OneMainMenuButton *butMain =
      GetMainMenuButton(last_content.main_index);
    assert(butMain);
    StaticString<128> caption;
    caption.Format(_T("%s > %s"),
                   gettext(butMain->caption), gettext(theitem.menu_caption));
    form.SetCaption(caption);
  }
}

void TabMenuControl::SetLastContentPage(unsigned page)
{
  if (page == GetMenuPage())
    last_content = MenuTabIndex::None();
  else {
    const PageItem& theitem = GetPageItem(page);
    last_content.main_index = theitem.main_menu_index;
    last_content.sub_index = GetSubMenuButton(page)->menu.sub_index;
  }
  GetTabMenuDisplay()->SetSelectedIndex(last_content);
}

int
TabMenuControl::GetPageNum(MenuTabIndex i) const
{
  if (!i.IsSub())
    return this->GetMenuPage();

  assert(i.main_index < main_menu_buttons.size());
  assert(i.sub_index < GetNumPages());

  const OneMainMenuButton *butMain = GetMainMenuButton(i.main_index);
  if (butMain)
    return butMain->first_page_index + i.sub_index;
  else
    return GetMenuPage();
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
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (i >= main_menu_buttons.size())
    return rcFallback;

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
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (page >= buttons.size())
    return rcFallback;

  if (buttons[page]->but_size.left < buttons[page]->but_size.right)
    return buttons[page]->but_size;

  const PageItem &item = this->GetPageItem(page);
  const OneMainMenuButton *butMain = GetMainMenuButton(item.main_menu_index);
  const OneSubMenuButton *butSub = GetSubMenuButton(page);

  if (!butMain || !butSub)
    return rcFallback;

  PixelRect &rc = buttons[page]->but_size;

  const UPixelScalar margin = Layout::Scale(1);
  const UPixelScalar finalmargin = Layout::Scale(1);
  const unsigned subMenuItemCount = butMain->NumSubMenus();
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

  rc.top = subMenuTop + butSub->menu.sub_index * itemHeight;
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
    const OneMainMenuButton *butMain = GetMainMenuButton(mainIndex);
    if (butMain) {
      for (unsigned i = butMain->first_page_index; i <= butMain->last_page_index;
           i++) {
        if (PtInRect(&GetSubMenuButtonSize(i), Pos))
          return MenuTabIndex(mainIndex, i - butMain->first_page_index);
      }
    }
  }

  return MenuTabIndex::None();
}

void
TabMenuControl::CreateSubMenuItem(const unsigned sub_menu_index,
                                  const PageItem &item, const unsigned page)
{
  assert(item.main_menu_index < MAX_MAIN_MENU_ITEMS);

  assert(item.Load != NULL);

  Widget *widget = item.Load();
  pager.AddPage(widget);

  OneSubMenuButton *b =
    new OneSubMenuButton(item.menu_caption,
                         MenuTabIndex(item.main_menu_index,
                                      sub_menu_index),
                         page);
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
      CreateSubMenuItem(subMenuIndex, item, i);
      firstPageIndex = min(i, firstPageIndex);
      subMenuIndex++;
    }
  }
  OneMainMenuButton *b =
      new OneMainMenuButton(main_menu_caption, firstPageIndex,
                            firstPageIndex + subMenuIndex - 1,
                            main_menu_index);
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
  buttons.append(new OneSubMenuButton(caption, MenuTabIndex::None(), 0));

  assert(GetNumPages() == num_pages);
}

unsigned
TabMenuControl::GotoMenuPage()
{
  SetCurrentPage(GetMenuPage());
  return GetMenuPage();
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
  mystyle.tab_stop();
  set(parent, left, top, width, height, mystyle);
}

void
TabMenuDisplay::SetSelectedIndex(TabMenuControl::MenuTabIndex di)
{
  selected_index = di;
  invalidate();
}

bool
TabMenuDisplay::on_key_check(unsigned key_code) const
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
TabMenuDisplay::on_key_down(unsigned key_code)
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
 return PaintWindow::on_key_down(key_code);
}

bool
TabMenuDisplay::on_mouse_down(PixelScalar x, PixelScalar y)
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
  return PaintWindow::on_mouse_down(x, y);
}

bool
TabMenuDisplay::on_mouse_up(PixelScalar x, PixelScalar y)
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
    return PaintWindow::on_mouse_up(x, y);
  }
}

const PixelRect&
TabMenuDisplay::GetDownButtonRC()
{
  TabMenuControl &tb = GetTabMenuBar();

  if (down_index.IsSub()) {
    int page = tb.GetPageNum(down_index);
    return tb.GetSubMenuButtonSize(page);
  }
  else
    return tb.GetMainMenuButtonSize(down_index.main_index);
}

bool
TabMenuDisplay::on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys)
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
TabMenuDisplay::PaintMainMenuBorder(Canvas &canvas)
{
  TabMenuControl &tb = GetTabMenuBar();
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
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas, const unsigned CaptionStyle)
{
  TabMenuControl &tb = GetTabMenuBar();
  PaintMainMenuBorder(canvas);

  for (auto i = tb.GetMainMenuButtons().begin(),
         end = tb.GetMainMenuButtons().end(); i != end; ++i) {
    bool inverse = false;
    const bool isDown = (*i)->main_menu_index == down_index.main_index &&
      !down_index.IsSub() && !drag_off_button;
    if (isDown) {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_YELLOW);

    } else if ((*i)->main_menu_index == selected_index.main_index) {
        canvas.SetTextColor(COLOR_WHITE);
        if (has_focus() && !HasPointer()) {
          canvas.SetBackgroundColor(COLOR_GRAY.Highlight());
        } else {
          canvas.SetBackgroundColor(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_WHITE);
    }
    const PixelRect rc = tb.GetMainMenuButtonSize((*i)->main_menu_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext((*i)->caption), rc,
                            false, NULL, isDown, inverse);
  }
  if (has_focus()) {
    PixelRect rcFocus;
    rcFocus.top = rcFocus.left = 0;
    rcFocus.right = canvas.get_width();
    rcFocus.bottom = canvas.get_height();
    canvas.DrawFocusRectangle(rcFocus);
  }
}

void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas, const OneMainMenuButton *butMain)
{
  TabMenuControl &tb = GetTabMenuBar();
  const UPixelScalar bwidth =  GetTabLineHeight();
  const UPixelScalar subTop = tb.GetSubMenuButtonSize(butMain->first_page_index).top;
  const PixelRect bLast = tb.GetSubMenuButtonSize(butMain->last_page_index);
  const PixelRect rcBlackBorder = { PixelScalar(bLast.left - bwidth),
                                    PixelScalar(subTop - bwidth),
                                    PixelScalar(bLast.right + bwidth),
                                    PixelScalar(bLast.bottom + bwidth) };

  canvas.DrawFilledRectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas, const unsigned CaptionStyle)
{
  TabMenuControl &tb = GetTabMenuBar();

  if (selected_index.IsNone())
    return;

  const OneMainMenuButton *butMain =
    tb.GetMainMenuButton(selected_index.main_index);
  if (!butMain)
    return;

  PaintSubMenuBorder(canvas, butMain);

  assert(butMain->first_page_index < tb.GetTabButtons().size());
  assert(butMain->last_page_index < tb.GetTabButtons().size());

  for (auto j = std::next(tb.GetTabButtons().begin(), butMain->first_page_index),
         end = std::next(tb.GetTabButtons().begin(), butMain->last_page_index + 1);
       j != end; ++j) {
    OneSubMenuButton *i = *j;

    bool inverse = false;
    if ((i->menu.sub_index == down_index.sub_index)
        && (drag_off_button == false)) {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_YELLOW);

    } else if (i->menu.sub_index == selected_index.sub_index) {
        canvas.SetTextColor(COLOR_WHITE);
        if (has_focus() && !HasPointer()) {
          canvas.SetBackgroundColor(COLOR_GRAY.Highlight());
        } else {
          canvas.SetBackgroundColor(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundColor(COLOR_WHITE);
    }
    const PixelRect &rc = tb.GetSubMenuButtonSize(i->page_index);
    TabDisplay::PaintButton(canvas, CaptionStyle, gettext(i->caption), rc,
                            false, NULL,
                            i->menu.sub_index == selected_index.sub_index,
                            inverse);
  }
}

void
TabMenuDisplay::on_paint(Canvas &canvas)
{
  canvas.clear(look.background_color);
  canvas.Select(*look.button.font);

  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  PaintMainMenuItems(canvas, CaptionStyle);
  PaintSubMenuItems(canvas, CaptionStyle);
}

void
TabMenuDisplay::on_killfocus()
{
  invalidate();
  PaintWindow::on_killfocus();
}

void
TabMenuDisplay::on_setfocus()
{
  invalidate();
  PaintWindow::on_setfocus();
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
