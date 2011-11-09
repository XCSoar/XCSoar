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

#ifndef XCSOAR_FORM_TABMENU_HPP
#define XCSOAR_FORM_TABMENU_HPP

#include "Util/StaticArray.hpp"
#include "Form/Tabbed.hpp"
#include "Form/TabBar.hpp"
#include "Dialogs/XML.hpp"

struct DialogLook;
class WndForm;
class TabMenuDisplay;
class OneSubMenuButton;
class OneMainMenuButton;
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
class TabMenuControl : public TabBarControl {
public:

  /**List of all submenu items in array of PageItem[0 to (n-1)]
   * The menus must be sorrted by main_menu_index and the order to appear
   */
  struct PageItem {
    const TCHAR *menu_caption;

     /* The main menu page Enter menu page into the array
      * 0 to (GetNumMainMenuCaptions() - 1) */
    const unsigned main_menu_index;

    /* If the page needs to perform actions each time before it is displayed */
    const TabBarControl::PreShowNotifyCallback_t PreShowCallback;

    /* If the page needs to perform cleanup after each time it is hidden */
    const TabBarControl::PreHideNotifyCallback_t PreHideCallback;

    /* Portrait XML resource.  Assumes "_L" converts name to landscape */
    const TCHAR *XML_PortraitResource;
  };

  enum tab_menu_values {  // fix indent
      MAX_MAIN_MENU_ITEMS = 7, // excludes "Main Menu" which is a "super menu"
      LARGE_VALUE = 1000,
      MAIN_MENU_PAGE = 999,
      NO_SUB_MENU = 998,
      NO_MAIN_MENU = 997,
      UNINITIALIZED = 996,
  };

  /* internally used structure for tracking menu down and selection status */
  struct menu_tab_index {
    unsigned MainIndex;
    unsigned SubIndex;

    gcc_constexpr_ctor
    menu_tab_index(unsigned mainNum, unsigned subNum)
      :MainIndex(mainNum), SubIndex(subNum) {}
  };

protected:
  /* holds info and buttons for the main menu.  not on child menus */
  StaticArray<OneMainMenuButton *, MAX_MAIN_MENU_ITEMS> MainMenuButtons;

  TabMenuDisplay *theTabDisplay;

  /* holds pointer to array of menus info (must be sorted by MenuGroup) */
  PageItem const *Pages;

  unsigned NumPages;
  menu_tab_index LastContent;

  /* Index of last page selected by menu */
  unsigned MainMenuIndex;

  /* Sub index of last page selected by menu */
  unsigned SubMenuIndex;

  StaticString<256u> Caption;

  /* value of last displayed content (non-menu) page */
  unsigned CurrentPageNum;

  WndForm &form;
  const CallBackTableEntry *LookUpTable;

public:
  /**
   * @param parent
   * @param Caption the page caption shown on the menu page
   * @param x, y Location of the tab bar. (frame of the content windows)
   * @param width, height.  Size of the tab bar
   * @param style
   * @return
   */
  TabMenuControl(ContainerWindow &parent,
                 WndForm &_form, const CallBackTableEntry *_look_up_table,
                 const DialogLook &look, const TCHAR * Caption,
                 PixelScalar x, PixelScalar y,
                 UPixelScalar width, UPixelScalar height,
                 const WindowStyle style = WindowStyle());
  ~TabMenuControl();

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

  unsigned GetNumMainMenuItems() const {
    return MainMenuButtons.size();
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
  const OneMainMenuButton *GetMainMenuButton(unsigned main_menu_index) const;

  /**
   * @param page
   * @return pointer to button or NULL if index is out of range
   */
  gcc_pure
  const OneSubMenuButton *GetSubMenuButton(unsigned page) const;

  /**
   * @param Pos position of pointer
   * @param mainIndex main menu whose submenu buttons are visible
   * @return menu_tab_index w/ location of item
   */
  gcc_pure
  menu_tab_index IsPointOverButton(RasterPoint Pos, unsigned mainIndex) const;

  /**
   * @return number of pages excluding the menu page
   */
  unsigned GetNumPages() { return NumPages; }

  /**
   *  returns menu item from data array of pages
   */
  const PageItem &GetPageItem(unsigned page) const {
    assert(page < NumPages);
    return Pages[page];
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
  int GetPageNum(const unsigned main_menu_index,
                 const unsigned sub_menu_index) const;

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
    return GetPageNum(LastContent.MainIndex, LastContent.SubIndex);
  }

  const StaticArray<OneMainMenuButton *, MAX_MAIN_MENU_ITEMS>
      &GetMainMenuButtons() { return MainMenuButtons; }

protected:
  /**
   * @returns Page Index being displayed (including NumPages for Menu)
   */
  unsigned GetCurrentPage() const {
    return CurrentPageNum;
  }

protected:

  /**
   * @return virtual menu page -- one greater than size of the menu array
   */
  unsigned GetMenuPage() const {
    return NumPages;
  }

  TabMenuDisplay *GetTabMenuDisplay() {
    return theTabDisplay;
  }

  /**
   * appends a submenu button to the buttons array and
   * loads it's XML file
   *
   * @param sub_menu_index Index of submenu item within in submenu
   * @param item The PageItem to be created
   * @param page Index to the page array
   */
  void CreateSubMenuItem(const unsigned sub_menu_index,
                         const PageItem &item,
                         const unsigned page);

  /** for each main menu item:
   *   adds to the MainMenuButtons array
   *   loops through all submenu items and loads them
   *
   * @param main_menu_caption The caption of the parent menu item
   * @param MainMenuIndex 0 to (MAX_SUB_MENUS-1)
   */
  void
  CreateSubMenu(const TCHAR *main_menu_caption,
                const unsigned main_menu_index);

  /**Adds a child tab to the menu
   * Same as TabBarControls verions except that
   * creates a OneTabMenuButton instead of a OneTabButton
   * and positions the windows differently
   * @param sub_menu_index Index with submenu
   * @param page Index to page array
   */
  unsigned AddClient(Window *w, const PageItem& item,
                     const unsigned sub_menu_index,
                     const unsigned page);

  /**
   * @return Height of any item in Main or Sub menu
   */
  UPixelScalar GetMenuButtonHeight() const;

  /**
   * @return Width of any item in Main or Sub menu
   */
  UPixelScalar GetMenuButtonWidth() const;

  /**
   * @return for portrait mode, puts menu near vertical center of screen
   */
  UPixelScalar GetButtonVerticalOffset() const;

  /**
   * hides all buttons etc for all content pages
   */
  void HideAllPageExtras();

public:
  void NextPage();
  void PreviousPage();
  /* assumes array is sorted by menugroup, then pagenum */
  void SetCurrentPage(unsigned page);
  void SetCurrentPage(menu_tab_index menuIndex);
};

class TabMenuDisplay : public TabDisplay {
public:
  TabMenuDisplay(TabBarControl& _theTabBar, const DialogLook &look,
                 PixelScalar left, PixelScalar top,
                 UPixelScalar width, UPixelScalar height,
                 bool _flipOrientation = false);

  void SetSelectedIndex(TabMenuControl::menu_tab_index di);
protected:
  TabMenuControl& GetTabMenuBar() { return (TabMenuControl&)theTabBar; }

  /**
   * @return Rect of button holding down pointer capture
   */
  const PixelRect& GetDownButtonRC();
  virtual bool on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool on_mouse_up(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);

  /**
   * canvas is the tabmenu which is the full content window, no content
   * @param canvas
   * Todo: support icons and "ButtonOnly" style
   */
  virtual void on_paint(Canvas &canvas);

  /**
   * draw border around main menu
   */
  void PaintMainMenuBorder(Canvas &canvas);
  void PaintMainMenuItems(Canvas &canvas, const unsigned CaptionStyle);
  void PaintSubMenuBorder(Canvas &canvas, const OneMainMenuButton *butMain);
  void PaintSubMenuItems(Canvas &canvas, const unsigned CaptionStyle);

  /* used to track mouse down/up clicks */
  TabMenuControl::menu_tab_index DownIndex;
  /* used to render which submenu is drawn and which item is highlighted */
  TabMenuControl::menu_tab_index SelectedIndex;
};

/**
 * class that holds the child menu button and info for the menu
 */
class OneSubMenuButton : public OneTabButton {
public:
  OneSubMenuButton(const TCHAR* _Caption,
                   unsigned _main_menu_index,
                   unsigned _sub_menu_index,
                   unsigned _page_index,
                   TabBarControl::PreHideNotifyCallback_t _PreHideFunction,
                   TabBarControl::PreShowNotifyCallback_t _PreShowFunction,
                   TabBarControl::PostShowNotifyCallback_t _PostShowFunction,
                   TabBarControl::ClickNotifyCallback_t _ClickFunction,
                   TabBarControl::ReClickNotifyCallback_t _ReClickFunction)
    :OneTabButton(_Caption, false, NULL,
                  _PreHideFunction, _PreShowFunction, _PostShowFunction,
                  _ClickFunction, _ReClickFunction),
     Menu(TabMenuControl::menu_tab_index(_main_menu_index, _sub_menu_index)),
     PageIndex(_page_index)
  {
  }

public:
  const TabMenuControl::menu_tab_index Menu;
  const unsigned PageIndex;
};

/**
 * class that holds the main menu button and info
 */
class OneMainMenuButton : public OneTabButton {
public:
  /* index to Pages array of first page in submenu */
  const unsigned FirstPageIndex;

  /* index to Pages array of last page in submenu */
  const unsigned LastPageIndex;

  /* index of button in MainMenu */
  const unsigned MainMenuIndex;

  OneMainMenuButton(const TCHAR* _Caption,
                    unsigned _FirstPageIndex,
                    unsigned _LastPageIndex,
                    unsigned _MainMenuIndex,
                    TabBarControl::PreHideNotifyCallback_t _PreHideFunction,
                    TabBarControl::PreShowNotifyCallback_t _PreShowFunction,
                    TabBarControl::PostShowNotifyCallback_t _PostShowFunction,
                    TabBarControl::ClickNotifyCallback_t _ClickFunction,
                    TabBarControl::ReClickNotifyCallback_t _ReClickFunction)
    :OneTabButton(_Caption, false, NULL,
                  _PreHideFunction, _PreShowFunction, _PostShowFunction,
                  _ClickFunction, _ReClickFunction),
     FirstPageIndex(_FirstPageIndex),
     LastPageIndex(_LastPageIndex),
     MainMenuIndex(_MainMenuIndex)
  {
  }

public:
  unsigned NumSubMenus() const { return LastPageIndex - FirstPageIndex + 1; };
};
#endif
