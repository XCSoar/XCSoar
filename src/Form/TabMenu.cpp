/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
  :TabBarControl(_parent, 1, look, x, y, _width, _height, style),
   NumPages(0),
   LastContent(NO_MAIN_MENU, NO_SUB_MENU),
   Caption(_caption),
   CurrentPageNum(UNINITIALIZED),
   form(_form),
   LookUpTable(_look_up_table)

{
  theTabDisplay = new TabMenuDisplay(*this, look, *this,
                                     0, y, _width, _height,
                                     flipOrientation);
}

TabMenuControl::~TabMenuControl()
{
  StaticArray<OneMainMenuButton *, MAX_MAIN_MENU_ITEMS>::const_iterator i,
      end = MainMenuButtons.end();
  for (i = MainMenuButtons.begin(); i != end; ++i)
    delete *i;
  delete theTabDisplay;
}

unsigned
TabMenuControl::AddClient(Window *w, const PageItem& item,
                          const unsigned sub_menu_index,
                          const unsigned page)
{
  TabbedControl::AddClient(w);
  OneSubMenuButton *b =
      new OneSubMenuButton(item.menu_caption, item.main_menu_index,
                           sub_menu_index,
                           page,
                           NULL,
                           item.PreShowCallback,
                           NULL, NULL, NULL);
  buttons.append(b);
  return buttons.size() - 1;
}

void
TabMenuControl::NextPage()
{
  const unsigned NumAllPages = NumPages + 1; // add one for menu page
  if (NumAllPages < 2)
    return;
  SetCurrentPage((GetCurrentPage() + 1) % NumAllPages);
}

void
TabMenuControl::PreviousPage()
{
  const unsigned NumAllPages = NumPages + 1; // add one for menu page

  if (NumAllPages < 2)
    return;
  SetCurrentPage((GetCurrentPage() + NumAllPages - 1) % NumAllPages);
}

void
TabMenuControl::HideAllPageExtras()
{
  for (unsigned i = 0; i < NumPages; i++) {
    const PageItem& theitem = GetPageItem(i);
    if (theitem.PreHideCallback != NULL)
      theitem.PreHideCallback();
  }
}

const OneMainMenuButton *
TabMenuControl::GetMainMenuButton(unsigned main_menu_index) const
{
  if (main_menu_index >= MainMenuButtons.size())
    return NULL;
  else
    return MainMenuButtons[main_menu_index];
}

const OneSubMenuButton *
TabMenuControl::GetSubMenuButton(unsigned page) const
{
  assert(page < NumPages && page < buttons.size());
  if (page >= buttons.size())
    return NULL;
  else
    return (OneSubMenuButton *)buttons[page];
}

void
TabMenuControl::SetCurrentPage(TabMenuControl::menu_tab_index menuIndex)
{
  assert(!((menuIndex.SubIndex == NO_SUB_MENU) &&
      (menuIndex.MainIndex == NO_MAIN_MENU)));

  SetCurrentPage(GetPageNum(menuIndex.MainIndex, menuIndex.SubIndex));
}

void
TabMenuControl::SetCurrentPage(unsigned page)
{
  HideAllPageExtras();

  TabBarControl::SetCurrentPage(page);

  if (page == GetMenuPage()) {
    form.SetCaption(Caption);
    const menu_tab_index di = LastContent;
    this->GetTabMenuDisplay()->SetSelectedIndex(di);

  } else {
    const PageItem& theitem = GetPageItem(page);
    LastContent.MainIndex = theitem.main_menu_index;
    LastContent.SubIndex = GetSubMenuButton(page)->Menu.SubIndex;
    const OneMainMenuButton *butMain =
      GetMainMenuButton(LastContent.MainIndex);
    assert(butMain);
    TCHAR caption[128];
    _sntprintf(caption, 128, _T("%s > %s"),
               gettext(butMain->Caption), theitem.menu_caption);
    form.SetCaption(caption);
  }

  CurrentPageNum = page;
}

int
TabMenuControl::GetPageNum(const unsigned main_menu_index,
                           const unsigned sub_menu_index) const
{
  if (sub_menu_index == TabMenuControl::NO_SUB_MENU ||
      main_menu_index == TabMenuControl::NO_MAIN_MENU)
    return this->GetMenuPage();

  assert(main_menu_index < MainMenuButtons.size());
  assert(sub_menu_index < NumPages);

  const OneMainMenuButton *butMain = GetMainMenuButton(main_menu_index);
  if (butMain)
    return butMain->FirstPageIndex + sub_menu_index;
  else
    return GetMenuPage();
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
  return (theTabDisplay->GetTabWidth() - TabLineHeight) / 2;
}

const PixelRect&
TabMenuControl::GetMainMenuButtonSize(unsigned i) const
{
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (i >= MainMenuButtons.size())
    return rcFallback;

  if (MainMenuButtons[i]->butSize.left < MainMenuButtons[i]->butSize.right)
    return MainMenuButtons[i]->butSize;
  PixelRect &rc = MainMenuButtons[i]->butSize;
  const UPixelScalar margin = Layout::Scale(1);
  const UPixelScalar finalmargin = Layout::Scale(1);
  const UPixelScalar butHeight = GetMenuButtonHeight();
  rc.top = finalmargin + (margin + butHeight) * i;
  rc.top += GetButtonVerticalOffset();
  rc.bottom = rc.top + butHeight;

  rc.left = 0;
  rc.right = GetMenuButtonWidth();

  return MainMenuButtons[i]->butSize;
}

const PixelRect&
TabMenuControl::GetSubMenuButtonSize(unsigned page) const
{
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (page >= buttons.size())
    return rcFallback;

  if (buttons[page]->butSize.left < buttons[page]->butSize.right)
    return buttons[page]->butSize;

  const PageItem &item = this->GetPageItem(page);
  const OneMainMenuButton *butMain = GetMainMenuButton(item.main_menu_index);
  const OneSubMenuButton *butSub = GetSubMenuButton(page);

  if (!butMain || !butSub)
    return rcFallback;

  PixelRect &rc = buttons[page]->butSize;

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

  rc.top = subMenuTop + butSub->Menu.SubIndex * itemHeight;
  rc.top += GetButtonVerticalOffset();
  rc.bottom = rc.top + butHeight;

  rc.left = GetMenuButtonWidth() + TabLineHeight;
  rc.right = rc.left + GetMenuButtonWidth();

  return buttons[page]->butSize;
}

TabMenuControl::menu_tab_index
TabMenuControl::IsPointOverButton(RasterPoint Pos, unsigned mainIndex) const
{
  // scan main menu buttons
  for (unsigned i = 0; i < GetNumMainMenuItems(); i++)
    if (PtInRect(&GetMainMenuButtonSize(i), Pos))
      return menu_tab_index(i, TabMenuControl::NO_SUB_MENU);


  // scan visible submenu
  if (mainIndex < GetNumMainMenuItems()) {
    const OneMainMenuButton *butMain = GetMainMenuButton(mainIndex);
    if (butMain) {
      for (unsigned i = butMain->FirstPageIndex; i <= butMain->LastPageIndex;
           i++) {
        if (PtInRect(&GetSubMenuButtonSize(i), Pos))
          return menu_tab_index(mainIndex, i - butMain->FirstPageIndex);
      }
    }
  }
  return menu_tab_index(TabMenuControl::NO_MAIN_MENU,
                        TabMenuControl::NO_SUB_MENU);
}

void
TabMenuControl::CreateSubMenuItem(const unsigned sub_menu_index,
                                  const PageItem &item, const unsigned page)
{
  assert(item.main_menu_index < MAX_MAIN_MENU_ITEMS);

  TCHAR xml_resource[100];
  _sntprintf(xml_resource, 100, _T("%s%s"),
             item.XML_PortraitResource,
             Layout::landscape ? _T("_L") : _T(""));

  Window *w = LoadWindow(LookUpTable, &form, *this, xml_resource);
  AddClient(w, item, sub_menu_index, page);
}

void
TabMenuControl::CreateSubMenu(const TCHAR *main_menu_caption,
                              const unsigned main_menu_index)
{
  assert(main_menu_index < MAX_MAIN_MENU_ITEMS);
  unsigned firstPageIndex = LARGE_VALUE;
  unsigned subMenuIndex = 0;

  for (unsigned i = 0; i < NumPages; i++) {
    const PageItem& item = GetPageItem(i);
    if (item.main_menu_index == main_menu_index) {
      CreateSubMenuItem(subMenuIndex, item, i);
      firstPageIndex = min(i, firstPageIndex);
      subMenuIndex++;
    }
  }
  OneMainMenuButton *b =
      new OneMainMenuButton(main_menu_caption, firstPageIndex,
                            firstPageIndex + subMenuIndex - 1,
                            main_menu_index,
                            NULL, NULL, NULL, NULL, NULL);
  MainMenuButtons.append(b);
}

void
TabMenuControl::InitMenu(const PageItem pages[],
                         unsigned num_pages,
                         const TCHAR *main_menu_captions[],
                         unsigned num_menu_captions)
{
  assert(pages);
  assert(main_menu_captions);

  Pages = pages;
  NumPages = num_pages;

  for (unsigned i = 0; i < num_menu_captions; i++)
    CreateSubMenu(main_menu_captions[i], i);

  CurrentPageNum = GetMenuPage();

  TabBarControl::AddClient(theTabDisplay, Caption);
}

unsigned
TabMenuControl::GotoMenuPage()
{
  SetCurrentPage(GetMenuPage());
  return GetMenuPage();
}

// TabMenuDisplay Functions
TabMenuDisplay::TabMenuDisplay(TabBarControl& _theTabBar,
                               const DialogLook &_look,
                               ContainerWindow &parent,
                               PixelScalar left, PixelScalar top,
                               UPixelScalar width, UPixelScalar height,
                               bool _flipOrientation)
  :TabDisplay(_theTabBar, _look,
              parent, left, top, width, height, _flipOrientation),
   DownIndex(TabMenuControl::NO_MAIN_MENU, TabMenuControl::NO_SUB_MENU),
   SelectedIndex(TabMenuControl::NO_MAIN_MENU, TabMenuControl::NO_SUB_MENU)
{
}

void
TabMenuDisplay::SetSelectedIndex(TabMenuControl::menu_tab_index di)
{
  SelectedIndex = di;
  invalidate();
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

  DownIndex = GetTabMenuBar().IsPointOverButton(Pos,
                                                SelectedIndex.MainIndex);

  if (DownIndex.MainIndex != TabMenuControl::NO_MAIN_MENU
      || DownIndex.SubIndex != TabMenuControl::NO_SUB_MENU) {
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
    const TabMenuControl::menu_tab_index di =
        GetTabMenuBar().IsPointOverButton(Pos, SelectedIndex.MainIndex);

    if (di.MainIndex == DownIndex.MainIndex &&
        di.SubIndex == DownIndex.SubIndex) {

      // sub menu click
      if (di.SubIndex != TabMenuControl::NO_SUB_MENU)
        GetTabMenuBar().SetCurrentPage(di);

      // main menu click
      else if (DownIndex.MainIndex != TabMenuControl::NO_MAIN_MENU)
        SelectedIndex = DownIndex;
      invalidate();
    }
    DownIndex.MainIndex = TabMenuControl::NO_MAIN_MENU;
    DownIndex.SubIndex = TabMenuControl::NO_SUB_MENU;

    return true;
  } else {
    return PaintWindow::on_mouse_up(x, y);
  }
}

const PixelRect&
TabMenuDisplay::GetDownButtonRC()
{
  TabMenuControl &tb = GetTabMenuBar();

  if (DownIndex.SubIndex != TabMenuControl::NO_SUB_MENU) {
    int page = tb.GetPageNum(DownIndex.MainIndex, DownIndex.SubIndex);
    return tb.GetSubMenuButtonSize(page);
  }
  else
    return tb.GetMainMenuButtonSize(DownIndex.MainIndex);
}

bool
TabMenuDisplay::on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (DownIndex.MainIndex == TabMenuControl::NO_MAIN_MENU
      && DownIndex.SubIndex == TabMenuControl::NO_SUB_MENU)
    return false;

  const PixelRect rc = GetDownButtonRC();
  RasterPoint Pos;
  Pos.x = x;
  Pos.y = y;

  const bool tmp = !PtInRect(&rc, Pos);
  if (dragoffbutton != tmp) {
    dragoffbutton = tmp;
    invalidate(rc);
  }
  return true;
}

void
TabMenuDisplay::PaintMainMenuBorder(Canvas &canvas)
{
  TabMenuControl &tb = GetTabMenuBar();
  const UPixelScalar bwidth = tb.GetTabLineHeight();

  const PixelRect rcFirst = tb.GetMainMenuButtonSize(0);
  const UPixelScalar menuBottom = tb.GetMainMenuButtonSize(
      tb.GetNumMainMenuItems() - 1).bottom;
  const PixelRect rcBlackBorder = { PixelScalar(rcFirst.left - bwidth),
                                    PixelScalar(rcFirst.top - bwidth),
                                    PixelScalar(rcFirst.right + bwidth),
                                    PixelScalar(menuBottom + bwidth) };

  canvas.fill_rectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintMainMenuItems(Canvas &canvas, const unsigned CaptionStyle)
{
  TabMenuControl &tb = GetTabMenuBar();
  PaintMainMenuBorder(canvas);

  StaticArray<OneMainMenuButton *,
      TabMenuControl::MAX_MAIN_MENU_ITEMS>::const_iterator i,
      end = tb.GetMainMenuButtons().end();

  for (i = tb.GetMainMenuButtons().begin(); i != end; ++i) {
    bool inverse = false;
    const bool isDown = ((*i)->MainMenuIndex == DownIndex.MainIndex &&
                         DownIndex.SubIndex == TabMenuControl::NO_SUB_MENU)
                         && (dragoffbutton == false);
    if (isDown) {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_YELLOW);

    } else if ((*i)->MainMenuIndex == SelectedIndex.MainIndex) {
        canvas.set_text_color(COLOR_WHITE);
        if (has_focus() && !has_pointer()) {
          canvas.set_background_color(COLOR_GRAY.Highlight());
        } else {
          canvas.set_background_color(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_WHITE);
    }
    const PixelRect rc = tb.GetMainMenuButtonSize((*i)->MainMenuIndex);
    PaintButton(canvas, CaptionStyle, gettext((*i)->Caption), rc,
                false, NULL, isDown, inverse);
  }
  if (has_focus()) {
    PixelRect rcFocus;
    rcFocus.top = rcFocus.left = 0;
    rcFocus.right = canvas.get_width();
    rcFocus.bottom = canvas.get_height();
    canvas.draw_focus(rcFocus);
  }
}

void
TabMenuDisplay::PaintSubMenuBorder(Canvas &canvas, const OneMainMenuButton *butMain)
{
  TabMenuControl &tb = GetTabMenuBar();
  const UPixelScalar bwidth =  tb.GetTabLineHeight();
  const UPixelScalar subTop = tb.GetSubMenuButtonSize(butMain->FirstPageIndex).top;
  const PixelRect bLast = tb.GetSubMenuButtonSize(butMain->LastPageIndex);
  const PixelRect rcBlackBorder = { PixelScalar(bLast.left - bwidth),
                                    PixelScalar(subTop - bwidth),
                                    PixelScalar(bLast.right + bwidth),
                                    PixelScalar(bLast.bottom + bwidth) };

  canvas.fill_rectangle(rcBlackBorder, COLOR_BLACK);
}

void
TabMenuDisplay::PaintSubMenuItems(Canvas &canvas, const unsigned CaptionStyle)
{
  TabMenuControl &tb = GetTabMenuBar();

  const unsigned mainIndex = SelectedIndex.MainIndex;
  if (mainIndex == TabMenuControl::NO_MAIN_MENU)
    return;
  const OneMainMenuButton *butMain = tb.GetMainMenuButton(mainIndex);
  if (!butMain)
    return;

  PaintSubMenuBorder(canvas, butMain);

  assert(butMain->FirstPageIndex < tb.GetTabButtons().size());
  assert(butMain->LastPageIndex < tb.GetTabButtons().size());

  StaticArray<OneTabButton *, 32>::const_iterator j,
      end = &(tb.GetTabButtons())[butMain->LastPageIndex];

  for (j = &(tb.GetTabButtons())[butMain->FirstPageIndex]; j <= end; ++j) {
    OneSubMenuButton *i = (OneSubMenuButton *)*j;

    bool inverse = false;
    if ((i->Menu.SubIndex == DownIndex.SubIndex)
        && (dragoffbutton == false)) {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_YELLOW);

    } else if (i->Menu.SubIndex == SelectedIndex.SubIndex) {
        canvas.set_text_color(COLOR_WHITE);
        if (has_focus() && !has_pointer()) {
          canvas.set_background_color(COLOR_GRAY.Highlight());
        } else {
          canvas.set_background_color(COLOR_BLACK);
        }
        inverse = true;

    } else {
      canvas.set_text_color(COLOR_BLACK);
      canvas.set_background_color(COLOR_WHITE);
    }
    const PixelRect &rc = tb.GetSubMenuButtonSize(i->PageIndex);
    PaintButton(canvas, CaptionStyle, gettext(i->Caption), rc, false, NULL,
                i->Menu.SubIndex == SelectedIndex.SubIndex, inverse);
  }
}

void
TabMenuDisplay::on_paint(Canvas &canvas)
{
  canvas.clear(look.background_color);
  canvas.select(*look.button_font);

  const unsigned CaptionStyle = DT_EXPANDTABS | DT_CENTER | DT_NOCLIP
      | DT_WORDBREAK;

  PaintMainMenuItems(canvas, CaptionStyle);
  PaintSubMenuItems(canvas, CaptionStyle);
}
