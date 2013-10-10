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
#include "Util/StaticString.hpp"
#include "Widget/PagerWidget.hpp"
#include "Screen/ContainerWindow.hpp"

#include <tchar.h>

struct DialogLook;
struct TabMenuPage;
class WndForm;
class TabMenuDisplay;

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
  PagerWidget pager;

  TabMenuDisplay *tab_display;

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
    pager.Move(GetClientRect());
  }

private:
  void OnPageFlipped();

public:
  /**
   * Initializes the menu and buids it from the Menuitem[] array
   *
   * @param menus[] array of TabMenuPage elements to be
   * displayed in the menu
   * @param num_menus Size the menus array
   * @param main_menu_captions Array of captions for main menu items
   * @param num_menu_captions Aize of main_menu_captions array
   */
  void InitMenu(const TabMenuPage menus[],
                unsigned num_menus,
                const TCHAR *main_menu_captions[],
                unsigned num_menu_captions);

  /**
   * @return true if currently displaying the menu page
   */
  gcc_pure
  bool IsCurrentPageTheMenu() const {
    return pager.GetCurrentIndex() == GetMenuPage();
  }

  /**
   * Sets the current page to the menu page
   * @return Page index of menu page
   */
  void GotoMenuPage() {
    SetCurrentPage(GetMenuPage());
  }

  /**
   * @return number of pages excluding the menu page
   */
  unsigned GetNumPages() const{
    return pager.GetSize() - 1;
  }

  /**
   * @return last content page shown (0 to (NumPages-1))
   * or MAIN_MENU_PAGE if no content pages have been shown
   */
  gcc_pure
  unsigned GetLastContentPage() const;

public:
  /**
   * @return virtual menu page -- one greater than size of the menu array
   */
  unsigned GetMenuPage() const {
    return GetNumPages();
  }

private:
  /**
   * appends a submenu button to the buttons array and
   * loads it's XML file
   *
   * @param item the TabMenuPage to be created
   */
  void CreateSubMenuItem(const TabMenuPage &item);

public:
  void NextPage();
  void PreviousPage();
  void SetCurrentPage(unsigned page);

  bool Save(bool &changed) {
    return pager.Save(changed);
  }

  /**
   * Pass a key press event to the active widget.
   */
  bool InvokeKeyPress(unsigned key_code);

  /**
   * sets page that determines which menu item is displayed as selected
   * @param page
   */
  void SetLastContentPage(unsigned page);

protected:
  /* virtual methods from Window */
  virtual void OnCreate() override;
  virtual void OnDestroy() override;
};

#endif
