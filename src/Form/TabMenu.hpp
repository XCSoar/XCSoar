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

#ifndef XCSOAR_FORM_TABMENU_HPP
#define XCSOAR_FORM_TABMENU_HPP

#include "Util/StaticArray.hpp"
#include "Form/Tabbed.hpp"
#include "Form/TabDisplay.hpp"

struct DialogLook;
class WndForm;
class TabMenuDisplay;
class SubMenuButton;
class MainMenuButton;
class ContainerWindow;

/** TabMenuControl is a two-level menu structure.
 * The menu items are linked to Tab panels
 * The tabbed panels are loaded via code, not via XML
 *
 * The menu structure is initialized via an array parameter
 * from the client.
 * Creates a vertical menu in both Portrait and in landscape modes
 * ToDo: lazy loading of panels (XML and Init() routines)
 */
class TabMenuControl : public ContainerWindow {
public:

  /**List of all submenu items in array of PageItem[0 to (n-1)]
   * The menus must be sorrted by main_menu_index and the order to appear
   */
  struct PageItem {
    const TCHAR *menu_caption;

     /* The main menu page Enter menu page into the array
      * 0 to (GetNumMainMenuCaptions() - 1) */
    unsigned main_menu_index;

    Widget *(*Load)();
  };

  enum tab_menu_values {  // fix indent
      MAX_MAIN_MENU_ITEMS = 7, // excludes "Main Menu" which is a "super menu"
      LARGE_VALUE = 1000,
      MAIN_MENU_PAGE = 999,
  };

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
  };

protected:
  TabbedControl pager;

  StaticArray<SubMenuButton *, 32> buttons;

  /* holds info and buttons for the main menu.  not on child menus */
  StaticArray<MainMenuButton *, MAX_MAIN_MENU_ITEMS> main_menu_buttons;

  TabMenuDisplay *tab_display;

  /* holds pointer to array of menus info (must be sorted by MenuGroup) */
  PageItem const *pages;

  unsigned last_content_page;

  StaticString<256u> caption;

  WndForm &form;

public:
  /**
   * @param parent
   * @param Caption the page caption shown on the menu page
   * @param style
   * @return
   */
  TabMenuControl(ContainerWindow &parent, WndForm &_form,
                 const DialogLook &look, const TCHAR * Caption,
                 PixelRect rc,
                 const WindowStyle style = WindowStyle());
  ~TabMenuControl();

  void UpdateLayout() {
    pager.UpdateLayout();
  }

  const StaticArray<SubMenuButton *, 32> &GetTabButtons() const {
    return buttons;
  }

public:
  /**
   * Initializes the menu and buids it from the Menuitem[] array
   *
   * @param menus[] Array of PageItem elements to be displayed in the menu
   * @param num_menus Size the menus array
   * @param main_menu_captions Array of captions for main menu items
   * @param num_menu_captions Aize of main_menu_captions array
   */
  void InitMenu(const PageItem menus[],
                unsigned num_menus,
                const TCHAR *main_menu_captions[],
                unsigned num_menu_captions);

  /**
   * Locate the page index in the menu.
   */
  gcc_pure
  MenuTabIndex FindPage(unsigned page) const;

  /**
   * @return true if currently displaying the menu page
   */
  gcc_pure
  bool IsCurrentPageTheMenu() const {
    return GetCurrentPage() == GetMenuPage();
  }

  /**
   * Sets the current page to the menu page
   * @return Page index of menu page
   */
  unsigned GotoMenuPage();

  /**
   * Set the keyboard focus on the menu page.  Switches to the menu
   * page if necessary.
   */
  void FocusMenuPage();

  unsigned GetNumMainMenuItems() const {
    return main_menu_buttons.size();
  }

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

  /**
   * @return number of pages excluding the menu page
   */
  unsigned GetNumPages() const{
    return pager.GetTabCount() - 1;
  }

  /**
   *  returns menu item from data array of pages
   */
  const PageItem &GetPageItem(unsigned page) const {
    assert(page < GetNumPages());
    return pages[page];
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

  /**
   * overloads from TabBarControl.
   */
  UPixelScalar GetTabHeight() const;

  /**
   * @return last content page shown (0 to (NumPages-1))
   * or MAIN_MENU_PAGE if no content pages have been shown
   */
  gcc_pure
  unsigned GetLastContentPage() const {
    return last_content_page;
  }

  const StaticArray<MainMenuButton *, MAX_MAIN_MENU_ITEMS>
      &GetMainMenuButtons() const { return main_menu_buttons; }

protected:
  /**
   * @returns Page Index being displayed (including NumPages for Menu)
   */
  unsigned GetCurrentPage() const {
    return pager.GetCurrentPage();
  }

protected:

  /**
   * @return virtual menu page -- one greater than size of the menu array
   */
  unsigned GetMenuPage() const {
    return GetNumPages();
  }

  TabMenuDisplay *GetTabMenuDisplay() {
    return tab_display;
  }

  /**
   * appends a submenu button to the buttons array and
   * loads it's XML file
   *
   * @param item The PageItem to be created
   */
  void CreateSubMenuItem(const PageItem &item);

  /** for each main menu item:
   *   adds to the MainMenuButtons array
   *   loops through all submenu items and loads them
   *
   * @param main_menu_caption The caption of the parent menu item
   * @param MainMenuIndex 0 to (MAX_SUB_MENUS-1)
   */
  void
  CreateSubMenu(const PageItem pages_in[], unsigned num_pages,
                const TCHAR *main_menu_caption,
                const unsigned main_menu_index);

  /**
   * @return Height of any item in Main or Sub menu
   */
  UPixelScalar GetMenuButtonHeight() const;

  /**
   * @return Width of any item in Main or Sub menu
   */
  UPixelScalar GetMenuButtonWidth() const;

public:
  void NextPage();
  void PreviousPage();
  void SetCurrentPage(unsigned page);
  void SetCurrentPage(MenuTabIndex menuIndex);

  bool Save(bool &changed) {
    return pager.Save(changed);
  }

  /**
   * Pass a key press event to the active widget.
   */
  bool InvokeKeyPress(unsigned key_code) {
    return pager.InvokeKeyPress(key_code);
  }

  /**
   * sets page that determines which menu item is displayed as selected
   * @param page
   */
  void SetLastContentPage(unsigned page);

  /**
   * if displaying the menu page, will select/highlight the next menu item
   */
  void HighlightNextMenuItem();

  /**
   * if displaying the menu page, will select/highlight the previous menu item
   */
  void HighlightPreviousMenuItem();
};

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
#endif
