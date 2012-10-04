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
#include "Form/TabMenuDisplay.hpp"
#include "Form/Form.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Dialogs/XML.hpp"
#include "Language/Language.hpp"

#include <assert.h>
#include <winuser.h>

TabMenuControl::TabMenuControl(ContainerWindow &_parent, WndForm &_form,
                               const DialogLook &look, const TCHAR * _caption,
                               PixelRect rc,
                               const WindowStyle style)
  :last_content_page(-1),
   caption(_caption),
   form(_form)
{
  Create(_parent, rc, style);

  rc = GetClientRect();
  WindowStyle pager_style;
  pager_style.ControlParent();
  pager.Create(*this, rc, pager_style);

  tab_display = new TabMenuDisplay(*this, look, pager, rc);
}

TabMenuControl::~TabMenuControl()
{
  for (const auto i : buttons)
    delete i;

  for (const auto i : main_menu_buttons)
    delete i;

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

  if (!pager.ClickPage(page))
    return;

  if (page == GetMenuPage()) {
    form.SetCaption(caption);
    const MenuTabIndex di = FindPage(last_content_page);
    this->GetTabMenuDisplay()->SetSelectedIndex(di);

  } else {
    const PageItem& theitem = GetPageItem(page);
    SetLastContentPage(page);
    const MainMenuButton &main_button =
      GetMainMenuButton(theitem.main_menu_index);
    StaticString<128> caption;
    caption.Format(_T("%s > %s"),
                   gettext(main_button.caption),
                   gettext(theitem.menu_caption));
    form.SetCaption(caption);
  }
}

void TabMenuControl::SetLastContentPage(unsigned page)
{
  last_content_page = page;
  GetTabMenuDisplay()->SetSelectedIndex(FindPage(page));
}

int
TabMenuControl::GetPageNum(MenuTabIndex i) const
{
  if (!i.IsSub())
    return this->GetMenuPage();

  assert(i.main_index < main_menu_buttons.size());
  assert(i.sub_index < GetNumPages());

  const MainMenuButton &main_button = GetMainMenuButton(i.main_index);
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
TabMenuControl::GetMenuButtonWidth() const
{
  return (tab_display->GetTabWidth() - GetTabLineHeight()) / 2;
}

const PixelRect&
TabMenuControl::GetMainMenuButtonSize(unsigned i) const
{
  assert(i < main_menu_buttons.size());

  if (main_menu_buttons[i]->rc.left < main_menu_buttons[i]->rc.right)
    return main_menu_buttons[i]->rc;
  PixelRect &rc = main_menu_buttons[i]->rc;
  const UPixelScalar margin = Layout::Scale(1);
  const UPixelScalar finalmargin = Layout::Scale(1);
  const UPixelScalar butHeight = GetMenuButtonHeight();
  rc.top = finalmargin + (margin + butHeight) * i;
  rc.bottom = rc.top + butHeight;

  rc.left = 0;
  rc.right = GetMenuButtonWidth();

  return main_menu_buttons[i]->rc;
}

const PixelRect&
TabMenuControl::GetSubMenuButtonSize(unsigned page) const
{
  assert(page < buttons.size());

  if (buttons[page]->rc.left < buttons[page]->rc.right)
    return buttons[page]->rc;

  const PageItem &item = this->GetPageItem(page);
  const MainMenuButton &main_button = GetMainMenuButton(item.main_menu_index);
  const unsigned sub_index = page - main_button.first_page_index;

  PixelRect &rc = buttons[page]->rc;

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
  rc.bottom = rc.top + butHeight;

  rc.left = GetMenuButtonWidth() + GetTabLineHeight();
  rc.right = rc.left + GetMenuButtonWidth();

  return buttons[page]->rc;
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
    const MainMenuButton &main_button = GetMainMenuButton(mainIndex);
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

  SubMenuButton *b =
    new SubMenuButton(item.menu_caption);
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
  MainMenuButton *b =
      new MainMenuButton(main_menu_caption, firstPageIndex,
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
  buttons.append(new SubMenuButton(caption));

  assert(GetNumPages() == num_pages);
}

TabMenuControl::MenuTabIndex
TabMenuControl::FindPage(unsigned page) const
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
  tab_display->SetFocus();
}
