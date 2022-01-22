/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/window/PaintWindow.hpp"
#include "Renderer/TabRenderer.hpp"
#include "util/StaticArray.hxx"

struct TabMenuGroup;
struct DialogLook;
class PagerWidget;

class TabMenuDisplay final : public PaintWindow
{
  /* excludes "Main Menu" which is a "super menu" */
  static constexpr unsigned MAX_MAIN_MENU_ITEMS = 7;

  /**
   * The offset from a page number in the #TabMenuDisplay to a page
   * number in the #PagerWidget.
   */
  static constexpr unsigned PAGE_OFFSET = 1;

  /**
   * class that holds the child menu button and info for the menu
   */
  struct SubMenuButton {
    //TODO MainMenuButton *group;
    unsigned main_menu_index;
    const TCHAR *caption;

    PixelRect rc;

    TabRenderer renderer;

    void Draw(Canvas &canvas, const DialogLook &look,
              bool focused, bool pressed, bool selected) const noexcept {
      renderer.Draw(canvas, rc, look, caption, nullptr,
                    focused, pressed, selected);
    }
};

  /**
   * class that holds the main menu button and info
   */
  struct MainMenuButton {
    const TCHAR *caption;

    PixelRect rc;

    TabRenderer renderer;

    /* index to Pages array of first page in submenu */
    unsigned first_page_index;

    /* index to Pages array of last page in submenu */
    unsigned last_page_index;

    unsigned NumSubMenus() const noexcept {
      return last_page_index - first_page_index + 1;
    };

    void Draw(Canvas &canvas, const DialogLook &look,
              bool focused, bool pressed, bool selected) const noexcept {
      renderer.Draw(canvas, rc, look, caption, nullptr,
                    focused, pressed, selected);
    }
  };

  /* internally used structure for tracking menu down and selection status */
  struct MenuTabIndex {
    static constexpr unsigned NO_MAIN_MENU = 997;
    static constexpr unsigned NO_SUB_MENU = 998;

    unsigned main_index;
    unsigned sub_index;

    constexpr
    explicit MenuTabIndex(unsigned mainNum,
                          unsigned subNum=NO_SUB_MENU) noexcept
      :main_index(mainNum), sub_index(subNum) {}

    constexpr
    static MenuTabIndex None() noexcept {
      return MenuTabIndex(NO_MAIN_MENU, NO_SUB_MENU);
    }

    constexpr
    bool IsNone() const noexcept {
      return main_index == NO_MAIN_MENU;
    }

    constexpr
    bool IsMain() const noexcept {
      return main_index != NO_MAIN_MENU && sub_index == NO_SUB_MENU;
    }

    constexpr
    bool IsSub() const noexcept {
      return sub_index != NO_SUB_MENU;
    }

    constexpr
    bool operator==(const MenuTabIndex &other) const noexcept {
      return main_index == other.main_index &&
        sub_index == other.sub_index;
    }

    constexpr
    bool operator!=(const MenuTabIndex &other) const noexcept {
      return !(*this == other);
    }
  };

  PagerWidget &pager;
  const DialogLook &look;

  StaticArray<SubMenuButton, 32> buttons;

  /* holds info and buttons for the main menu.  not on child menus */
  StaticArray<MainMenuButton, MAX_MAIN_MENU_ITEMS> main_menu_buttons;

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
  TabMenuDisplay(PagerWidget &pager, const DialogLook &look) noexcept;

  /**
   * Initializes the menu and buids it from the Menuitem[] array
   *
   * @param pages_in array of TabMenuPage elements to be
   * displayed in the menu
   * @param num_pages Size the menus array
   */
  void InitMenu(const TabMenuGroup groups[], unsigned n_groups) noexcept;

  const TCHAR *GetCaption(TCHAR buffer[], size_t size) const noexcept;

  /**
   * Call this from PagerWidget's OnPageFlipped callback.  It moves
   * the cursor to the newly selected page.
   */
  void OnPageFlipped() noexcept;

  void SetCursor(unsigned i) noexcept;

private:
  void UpdateLayout() noexcept;

  bool HighlightNext() noexcept;
  bool HighlightPrevious() noexcept;

public:
  /**
   * Returns index of selected (highlighted) tab
   * @return
   */
  unsigned GetCursor() const noexcept {
    return cursor;
  }

private:
  unsigned GetNumPages() const noexcept {
    return buttons.size();
  }

  const TCHAR *GetPageParentCaption(unsigned page) const noexcept {
    assert(page < GetNumPages());

    return main_menu_buttons[buttons[page].main_menu_index].caption;
  }

  unsigned GetNumMainMenuItems() const noexcept {
    return main_menu_buttons.size();
  }

  [[gnu::pure]]
  unsigned GetPageMainIndex(unsigned page) const noexcept {
    assert(page < GetNumPages());

    return buttons[page].main_menu_index;
  }

  /**
   * Looks up the page id from the menu table
   * based on the main menu and sub menu index parameters
   *
   * @MainMenuIndex Index from main menu
   * @SubMenuIndex Index within submenu
   * returns page number of selected sub menu item base on menus indexes
   */
  [[gnu::pure]]
  int GetPageNum(MenuTabIndex i) const noexcept;

  [[gnu::pure]]
  const PixelRect &GetButtonPosition(MenuTabIndex i) const noexcept;

  /**
   * @param main_menu_index
   * @return pointer to button or nullptr if index is out of range
   */
  [[gnu::pure]]
  const MainMenuButton &GetMainMenuButton(unsigned main_menu_index) const noexcept {
    assert(main_menu_index < main_menu_buttons.size());

    return main_menu_buttons[main_menu_index];
  }

  /**
   * Calculates and caches the size and position of ith sub menu button
   * All menus (landscape or portrait) are drawn vertically
   * @param i Index of button
   * @return Rectangle of button coordinates,
   *   or {0,0,0,0} if index out of bounds
   */
  [[gnu::pure]]
  const PixelRect &GetSubMenuButtonSize(unsigned i) const noexcept {
    return buttons[i].rc;
  }

  /**
   * Calculates and caches the size and position of ith main menu button
   * All menus (landscape or portrait) are drawn vertically
   * @param i Index of button
   * @return Rectangle of button coordinates,
   *   or {0,0,0,0} if index out of bounds
   */
  [[gnu::pure]]
  const PixelRect &GetMainMenuButtonSize(unsigned i) const noexcept {
    return main_menu_buttons[i].rc;
  }

  /**
   * @param page
   * @return pointer to button or nullptr if index is out of range
   */
  [[gnu::pure]]
  const SubMenuButton &GetSubMenuButton(unsigned page) const noexcept {
    assert(page < GetNumPages() && page < buttons.size());

    return buttons[page];
  }

  /**
   * @param Pos position of pointer
   * @param mainIndex main menu whose submenu buttons are visible
   * @return MenuTabIndex w/ location of item
   */
  [[gnu::pure]]
  MenuTabIndex IsPointOverButton(PixelPoint Pos,
                                 unsigned mainIndex) const noexcept;

  void DragEnd() noexcept;

protected:
  void OnResize(PixelSize new_size) override;

  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseUp(PixelPoint p) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;

  /**
   * canvas is the tabmenu which is the full content window, no content
   * @param canvas
   * Todo: support icons and "ButtonOnly" style
   */
  void OnPaint(Canvas &canvas) override;

  void OnKillFocus() override;
  void OnSetFocus() override;

private:
  void InvalidateButton(MenuTabIndex i) noexcept {
    if (SupportsPartialRedraw())
      Invalidate(GetButtonPosition(i));
    else
      Invalidate();
  }

  /**
   * draw border around main menu
   */
  void PaintMainMenuBorder(Canvas &canvas) const noexcept;
  void PaintMainMenuItems(Canvas &canvas) const noexcept;
  void PaintSubMenuBorder(Canvas &canvas,
                          const MainMenuButton &main_button) const noexcept;
  void PaintSubMenuItems(Canvas &canvas) const noexcept;
};

#endif
