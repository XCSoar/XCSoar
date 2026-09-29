// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "util/StaticArray.hxx"

struct TabMenuGroup;
struct DialogLook;
class PagerWidget;
class Canvas;

/**
 * Tile-grid Configuration menu (text captions only): top-level groups,
 * then the pages of the selected group.  Rounded tiles with light/dark
 * styling.
 */
class ConfigTileMenu final : public PaintWindow {
  static constexpr unsigned MAX_MAIN_MENU_ITEMS = 9;
  static constexpr unsigned PAGE_OFFSET = 1;

  struct PageButton {
    unsigned main_menu_index;
    const char *caption;
    PixelRect rc;
  };

  struct MainMenuButton {
    const char *caption;
    unsigned first_page_index;
    unsigned last_page_index;
    PixelRect rc;

    unsigned NumSubMenus() const noexcept {
      return last_page_index - first_page_index + 1;
    }
  };

  PagerWidget &pager;
  const DialogLook &look;

  StaticArray<PageButton, 48> buttons;
  StaticArray<MainMenuButton, MAX_MAIN_MENU_ITEMS> main_menu_buttons;

  /** -1 = top-level group tiles; else index into main_menu_buttons. */
  int submenu_main = -1;

  unsigned cursor = 0;

  bool dragging = false;
  bool drag_off_button = false;
  int down_index = -1;

public:
  ConfigTileMenu(PagerWidget &_pager, const DialogLook &_look) noexcept;

  void InitMenu(const TabMenuGroup groups[], unsigned n_groups) noexcept;

  const char *GetCaption(char buffer[], size_t size) const noexcept;

  void OnPageFlipped() noexcept;

  void SetCursor(unsigned i) noexcept;

  unsigned GetCursor() const noexcept {
    return cursor;
  }

  /**
   * If a group submenu is open, return to the group tiles.
   * @return true if the close/back was consumed
   */
  bool GoBackToMain() noexcept;

  bool IsShowingMain() const noexcept {
    return submenu_main < 0;
  }

private:
  void UpdateLayout() noexcept;
  void ShowMainMenu() noexcept;
  void ShowSubMenu(unsigned main_index) noexcept;

  void DrawTile(Canvas &canvas, const PixelRect &rc, const char *caption,
                bool focused, bool pressed, bool selected) const noexcept;

  [[gnu::pure]]
  unsigned GetNumTiles() const noexcept;

  [[gnu::pure]]
  int HitTest(PixelPoint pos) const noexcept;

  void InvalidateTile(int index) noexcept;

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;
  bool OnMouseDown(PixelPoint pos) noexcept override;
  bool OnMouseUp(PixelPoint pos) noexcept override;
  bool OnMouseMove(PixelPoint pos, unsigned keys) noexcept override;
  bool OnKeyCheck(unsigned key_code) const noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  void OnKillFocus() noexcept override;
  void OnSetFocus() noexcept override;
};
