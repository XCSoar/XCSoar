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
#include "TabDisplay.hpp"
#include "Screen/PaintWindow.hpp"

struct DialogLook;
class ContainerWindow;

/**
 * class that holds the child menu button and info for the menu
 */
class SubMenuButton : public TabButton {
public:
  SubMenuButton(const TCHAR* _Caption)
    :TabButton(_Caption, NULL)
  {
  }
};

/**
 * class that holds the main menu button and info
 */
class MainMenuButton : public TabButton {
public:
  /* index to Pages array of first page in submenu */
  const unsigned first_page_index;

  /* index to Pages array of last page in submenu */
  const unsigned last_page_index;

  MainMenuButton(const TCHAR* _Caption,
                    unsigned _first_page_index,
                    unsigned _last_page_index)
    :TabButton(_Caption, NULL),
     first_page_index(_first_page_index),
     last_page_index(_last_page_index)
  {
  }

public:
  unsigned NumSubMenus() const { return last_page_index - first_page_index + 1; };
};

class TabMenuDisplay final : public PaintWindow
{
  /* excludes "Main Menu" which is a "super menu" */
  static constexpr unsigned MAX_MAIN_MENU_ITEMS = 7;

  /* internally used structure for tracking menu down and selection status */
  struct MenuTabIndex {
    static constexpr unsigned NO_MAIN_MENU = 997;
    static constexpr unsigned NO_SUB_MENU = 998;

    unsigned main_index;
    unsigned sub_index;

    constexpr
    explicit MenuTabIndex(unsigned mainNum, unsigned subNum=NO_SUB_MENU)
      :main_index(mainNum), sub_index(subNum) {}

    constexpr
    static MenuTabIndex None() {
      return MenuTabIndex(NO_MAIN_MENU, NO_SUB_MENU);
    }

    constexpr
    bool IsNone() const {
      return main_index == NO_MAIN_MENU;
    }

    constexpr
    bool IsMain() const {
      return main_index != NO_MAIN_MENU && sub_index == NO_SUB_MENU;
    }

    constexpr
    bool IsSub() const {
      return sub_index != NO_SUB_MENU;
    }

    constexpr
    bool operator==(const MenuTabIndex &other) const {
      return main_index == other.main_index &&
        sub_index == other.sub_index;
    }

    constexpr
    bool operator!=(const MenuTabIndex &other) const {
      return !(*this == other);
    }
  };

  TabMenuControl &menu;
  const DialogLook &look;

  StaticArray<SubMenuButton *, 32> buttons;

  /* holds info and buttons for the main menu.  not on child menus */
  StaticArray<MainMenuButton *, MAX_MAIN_MENU_ITEMS> main_menu_buttons;

  /* holds pointer to array of menus info (must be sorted by MenuGroup) */
  const TabMenuControl::PageItem *pages;

  bool dragging; // tracks that mouse is down and captured
  bool drag_off_button; // set by mouse_move

  /* used to track mouse down/up clicks */
  MenuTabIndex down_index;

  /**
   * Which page is currently selected by the cursor?
   */
  unsigned cursor;

  /* used to render which submenu is drawn and which item is highlighted */

public:
  TabMenuDisplay(TabMenuControl &_menu, const DialogLook &look,
                 ContainerWindow &parent, PixelRect rc, WindowStyle style);
  virtual ~TabMenuDisplay();

  /**
   * Initializes the menu and buids it from the Menuitem[] array
   *
   * @param pages_in Array of PageItem elements to be displayed in the menu
   * @param num_pages Size the menus array
   * @param main_menu_captions Array of captions for main menu items
   * @param num_menu_captions Aize of main_menu_captions array
   */
  void InitMenu(const TCHAR *caption,
                const TabMenuControl::PageItem pages_in[], unsigned num_pages,
                const TCHAR *main_menu_captions[],
                unsigned num_menu_captions);

  void SetCursor(unsigned i);

private:
  bool HighlightNext();
  bool HighlightPrevious();

public:
  /**
   * Returns index of selected (highlighted) tab
   * @return
   */
  unsigned GetCursor() const {
    return cursor;
  }

private:
  unsigned GetNumPages() const {
    return menu.GetNumPages();
  }

  /**
   *  returns menu item from data array of pages
   */
  const TabMenuControl::PageItem &GetPageItem(unsigned page) const {
    assert(page < GetNumPages());
    return pages[page];
  }

public:
  const TCHAR *GetPageCaption(unsigned page) const {
    assert(page < GetNumPages());

    return pages[page].menu_caption;
  }

  const TCHAR *GetPageParentCaption(unsigned page) const {
    assert(page < GetNumPages());

    const unsigned main_menu_index = pages[page].main_menu_index;
    assert(main_menu_index < main_menu_buttons.size());

    return main_menu_buttons[main_menu_index]->caption;
  }

  unsigned GetNumMainMenuItems() const {
    return main_menu_buttons.size();
  }

  gcc_pure
  unsigned GetPageMainIndex(unsigned page) const {
    assert(page < GetNumPages());

    return pages[page].main_menu_index;
  }

  /**
   * Looks up the page id from the menu table
   * based on the main menu and sub menu index parameters
   *
   * @MainMenuIndex Index from main menu
   * @SubMenuIndex Index within submenu
   * returns page number of selected sub menu item base on menus indexes
   *  or GetMenuPage() if index is not a valid page
   */
  gcc_pure
  int GetPageNum(MenuTabIndex i) const;

private:
  void AddMenu(const TCHAR *caption, unsigned first, unsigned last,
               unsigned main_menu_index) {
    assert(main_menu_index == main_menu_buttons.size());
    assert(main_menu_index < MAX_MAIN_MENU_ITEMS);

    main_menu_buttons.append(new MainMenuButton(caption, first, last));
  }

  void AddMenuItem(const TCHAR *caption) {
    buttons.append(new SubMenuButton(caption));
  }

  /**
   * overloads from TabBarControl.
   */
  gcc_pure
  unsigned GetTabHeight() const;

  /**
   * @return Height of any item in Main or Sub menu
   */
  gcc_pure
  unsigned GetMenuButtonHeight() const;

  /**
   * @return Width of any item in Main or Sub menu
   */
  gcc_pure
  unsigned GetMenuButtonWidth() const;

  /**
   * Calculates and caches the size and position of ith sub menu button
   * All menus (landscape or portrait) are drawn vertically
   * @param i Index of button
   * @return Rectangle of button coordinates,
   *   or {0,0,0,0} if index out of bounds
   */
  gcc_pure
  const PixelRect &GetSubMenuButtonSize(unsigned i) const;

  /**
   * Calculates and caches the size and position of ith main menu button
   * All menus (landscape or portrait) are drawn vertically
   * @param i Index of button
   * @return Rectangle of button coordinates,
   *   or {0,0,0,0} if index out of bounds
   */
  gcc_pure
  const PixelRect &GetMainMenuButtonSize(unsigned i) const;

  gcc_pure
  const PixelRect &GetButtonPosition(MenuTabIndex i) const;

  /**
   * @param main_menu_index
   * @return pointer to button or NULL if index is out of range
   */
  gcc_pure
  const MainMenuButton &GetMainMenuButton(unsigned main_menu_index) const {
    assert(main_menu_index < main_menu_buttons.size());
    assert(main_menu_buttons[main_menu_index] != NULL);

    return *main_menu_buttons[main_menu_index];
  }

  /**
   * @param page
   * @return pointer to button or NULL if index is out of range
   */
  gcc_pure
  const SubMenuButton &GetSubMenuButton(unsigned page) const {
    assert(page < GetNumPages() && page < buttons.size());
    assert(buttons[page] != NULL);

    return *buttons[page];
  }

  /**
   * @param Pos position of pointer
   * @param mainIndex main menu whose submenu buttons are visible
   * @return MenuTabIndex w/ location of item
   */
  gcc_pure
  MenuTabIndex IsPointOverButton(RasterPoint Pos, unsigned mainIndex) const;

  void DragEnd();

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
  void InvalidateButton(MenuTabIndex i) {
    if (SupportsPartialRedraw())
      Invalidate(GetButtonPosition(i));
    else
      Invalidate();
  }

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
