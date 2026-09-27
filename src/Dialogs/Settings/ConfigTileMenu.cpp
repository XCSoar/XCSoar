// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigTileMenu.hpp"
#include "Form/TabMenuData.hpp"
#include "Form/Button.hpp"
#include "Widget/PagerWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/Colors.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Renderer/TextRenderer.hpp"
#include "util/StringFormat.hpp"
#include "Asset.hpp"

#include <algorithm>
#include <cassert>

ConfigTileMenu::ConfigTileMenu(PagerWidget &_pager,
                               const DialogLook &_look) noexcept
  :pager(_pager), look(_look)
{
}

void
ConfigTileMenu::InitMenu(const TabMenuGroup groups[],
                         unsigned n_groups) noexcept
{
  assert(groups != nullptr);
  assert(n_groups > 0);
  assert(n_groups <= MAX_MAIN_MENU_ITEMS);

  main_menu_buttons.resize(n_groups);
  for (unsigned i = 0; i < n_groups; i++) {
    const auto &g = groups[i];
    auto &mb = main_menu_buttons[i];
    mb.caption = gettext(g.caption);
    mb.first_page_index = buttons.size();

    for (auto p = g.pages; p->Load != nullptr; ++p) {
      auto &page_button = buttons.append();
      page_button.main_menu_index = i;
      page_button.caption = gettext(p->menu_caption);

      auto panel = p->Load();
      auto scroll_panel =
        std::make_unique<VScrollWidget>(std::move(panel), look);
      pager.Add(std::move(scroll_panel));
    }

    mb.last_page_index = buttons.size() - 1;
  }
}

const char *
ConfigTileMenu::GetCaption(char buffer[], size_t size) const noexcept
{
  const unsigned page = pager.GetCurrentIndex();
  if (page >= PAGE_OFFSET) {
    const unsigned i = page - PAGE_OFFSET;
    StringFormat(buffer, size, "%s > %s",
                 main_menu_buttons[buttons[i].main_menu_index].caption,
                 buttons[i].caption);
    return buffer;
  }

  if (submenu_main >= 0) {
    assert(unsigned(submenu_main) < main_menu_buttons.size());
    return main_menu_buttons[submenu_main].caption;
  }

  return nullptr;
}

void
ConfigTileMenu::OnPageFlipped() noexcept
{
  const unsigned i = pager.GetCurrentIndex();
  if (i >= PAGE_OFFSET)
    SetCursor(i - PAGE_OFFSET);
}

void
ConfigTileMenu::SetCursor(unsigned i) noexcept
{
  if (i >= buttons.size())
    return;

  cursor = i;
  if (IsDefined())
    Invalidate();
}

bool
ConfigTileMenu::GoBackToMain() noexcept
{
  if (submenu_main < 0)
    return false;

  ShowMainMenu();
  return true;
}

void
ConfigTileMenu::ShowMainMenu() noexcept
{
  submenu_main = -1;
  UpdateLayout();
  Invalidate();
}

void
ConfigTileMenu::ShowSubMenu(unsigned main_index) noexcept
{
  assert(main_index < main_menu_buttons.size());
  submenu_main = int(main_index);
  cursor = main_menu_buttons[main_index].first_page_index;
  UpdateLayout();
  Invalidate();
}

unsigned
ConfigTileMenu::GetNumTiles() const noexcept
{
  if (submenu_main < 0)
    return main_menu_buttons.size();

  return main_menu_buttons[submenu_main].NumSubMenus();
}

void
ConfigTileMenu::UpdateLayout() noexcept
{
  const auto window_size = GetSize();
  if (window_size.width == 0 || window_size.height == 0)
    return;

  const unsigned gap = Layout::Scale(4);
  const auto grid = Layout::GetTileGridGeometry(window_size);
  const unsigned columns = grid.columns;
  const unsigned rows = grid.rows;

  const unsigned cell_w =
    (window_size.width > (columns + 1) * gap)
    ? (window_size.width - (columns + 1) * gap) / columns
    : window_size.width / columns;
  const unsigned cell_h =
    (window_size.height > (rows + 1) * gap)
    ? (window_size.height - (rows + 1) * gap) / rows
    : window_size.height / rows;

  if (submenu_main < 0) {
    for (unsigned i = 0; i < main_menu_buttons.size(); ++i) {
      const unsigned col = i % columns;
      const unsigned row = i / columns;
      auto &mb = main_menu_buttons[i];
      mb.rc.left = int(gap + col * (cell_w + gap));
      mb.rc.top = int(gap + row * (cell_h + gap));
      mb.rc.right = mb.rc.left + int(cell_w);
      mb.rc.bottom = mb.rc.top + int(cell_h);
    }
    return;
  }

  const auto &main = main_menu_buttons[submenu_main];
  for (unsigned page_i = main.first_page_index, tile = 0;
       page_i <= main.last_page_index; ++page_i, ++tile) {
    const unsigned col = tile % columns;
    const unsigned row = tile / columns;
    auto &page = buttons[page_i];
    page.rc.left = int(gap + col * (cell_w + gap));
    page.rc.top = int(gap + row * (cell_h + gap));
    page.rc.right = page.rc.left + int(cell_w);
    page.rc.bottom = page.rc.top + int(cell_h);
  }
}

int
ConfigTileMenu::HitTest(PixelPoint pos) const noexcept
{
  if (submenu_main < 0) {
    for (unsigned i = 0; i < main_menu_buttons.size(); ++i)
      if (main_menu_buttons[i].rc.Contains(pos))
        return int(i);
    return -1;
  }

  const auto &main = main_menu_buttons[submenu_main];
  for (unsigned page_i = main.first_page_index;
       page_i <= main.last_page_index; ++page_i)
    if (buttons[page_i].rc.Contains(pos))
      return int(page_i - main.first_page_index);

  return -1;
}

void
ConfigTileMenu::InvalidateTile(int index) noexcept
{
  if (index < 0)
    return;

  if (submenu_main < 0) {
    if (unsigned(index) < main_menu_buttons.size())
      Invalidate(main_menu_buttons[index].rc);
    return;
  }

  const auto &main = main_menu_buttons[submenu_main];
  const unsigned page_i = main.first_page_index + unsigned(index);
  if (page_i <= main.last_page_index)
    Invalidate(buttons[page_i].rc);
}

void
ConfigTileMenu::OnResize(PixelSize new_size) noexcept
{
  PaintWindow::OnResize(new_size);
  UpdateLayout();
}

bool
ConfigTileMenu::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RETURN:
  case KEY_LEFT:
  case KEY_RIGHT:
  case KEY_UP:
  case KEY_DOWN:
    return true;

  default:
    return false;
  }
}

bool
ConfigTileMenu::OnKeyDown(unsigned key_code) noexcept
{
  const unsigned n = GetNumTiles();
  if (n == 0)
    return false;

  unsigned focus = 0;
  if (submenu_main < 0) {
    focus = std::min(cursor < main_menu_buttons.size()
                     ? buttons[cursor].main_menu_index
                     : 0u,
                     n - 1);
  } else {
    const auto &main = main_menu_buttons[submenu_main];
    focus = cursor >= main.first_page_index && cursor <= main.last_page_index
      ? cursor - main.first_page_index
      : 0;
  }

  const auto size = GetSize();
  const unsigned columns = Layout::GetTileGridGeometry(size).columns;

  switch (key_code) {
  case KEY_RETURN:
    if (submenu_main < 0) {
      const auto &main = main_menu_buttons[focus];
      if (main.NumSubMenus() == 1)
        pager.ClickPage(PAGE_OFFSET + main.first_page_index);
      else
        ShowSubMenu(focus);
    } else {
      const auto &main = main_menu_buttons[submenu_main];
      pager.ClickPage(PAGE_OFFSET + main.first_page_index + focus);
    }
    return true;

  case KEY_RIGHT:
    if (focus + 1 < n) {
      ++focus;
      break;
    }
    return true;

  case KEY_LEFT:
    if (focus > 0) {
      --focus;
      break;
    }
    return true;

  case KEY_DOWN:
    if (focus + columns < n) {
      focus += columns;
      break;
    }
    return true;

  case KEY_UP:
    if (focus >= columns) {
      focus -= columns;
      break;
    }
    return true;

  default:
    return false;
  }

  if (submenu_main < 0)
    cursor = main_menu_buttons[focus].first_page_index;
  else
    cursor = main_menu_buttons[submenu_main].first_page_index + focus;

  Invalidate();
  return true;
}

bool
ConfigTileMenu::OnMouseDown(PixelPoint pos) noexcept
{
  SetFocus();

  down_index = HitTest(pos);
  if (down_index < 0)
    return PaintWindow::OnMouseDown(pos);

#ifdef HAVE_VIBRATOR
  PlayHapticFeedback(HapticFeedbackType::PRESS);
#endif

  dragging = true;
  drag_off_button = false;
  SetCapture();
  InvalidateTile(down_index);
  return true;
}

bool
ConfigTileMenu::OnMouseUp(PixelPoint pos) noexcept
{
  if (!dragging)
    return PaintWindow::OnMouseUp(pos);

  dragging = false;
  ReleaseCapture();

  const int up_index = HitTest(pos);
  const int pressed = down_index;
  down_index = -1;
  drag_off_button = false;

  if (up_index < 0 || up_index != pressed) {
    Invalidate();
    return true;
  }

  if (submenu_main < 0) {
    const auto &main = main_menu_buttons[unsigned(up_index)];
    if (main.NumSubMenus() == 1)
      pager.ClickPage(PAGE_OFFSET + main.first_page_index);
    else
      ShowSubMenu(unsigned(up_index));
  } else {
    const auto &main = main_menu_buttons[submenu_main];
    pager.ClickPage(PAGE_OFFSET + main.first_page_index +
                    unsigned(up_index));
  }

  return true;
}

bool
ConfigTileMenu::OnMouseMove(PixelPoint pos,
                            [[maybe_unused]] unsigned keys) noexcept
{
  if (down_index < 0)
    return false;

  const bool off = HitTest(pos) != down_index;
  if (off != drag_off_button) {
    drag_off_button = off;
    InvalidateTile(down_index);
  }
  return true;
}

void
ConfigTileMenu::DrawTile(Canvas &canvas, const PixelRect &rc,
                         const char *caption,
                         bool focused, bool pressed,
                         bool selected) const noexcept
{
  const unsigned inset = Layout::Scale(4);
  PixelRect tile = rc;
  if (tile.GetWidth() > 2 * inset && tile.GetHeight() > 2 * inset)
    tile.Grow(-int(inset));

  const unsigned radius = Layout::Scale(12);
  const unsigned diameter = std::min(2u * radius,
                                     std::min(unsigned(tile.GetWidth()),
                                              unsigned(tile.GetHeight())));

  Color fill, border, text;

  if (IsDithered() || !HasColors()) {
    fill = COLOR_WHITE;
    border = COLOR_BLACK;
    text = COLOR_BLACK;
    if (pressed || (focused && selected))
      fill = COLOR_LIGHT_GRAY;
  } else if (look.dark_mode) {
    fill = COLOR_CONFIG_MENU_TILE;
    border = COLOR_CONFIG_MENU_TILE_BORDER;
    text = COLOR_WHITE;
    if (pressed)
      fill = COLOR_CONFIG_MENU_TILE_PRESSED;
    else if (focused && selected)
      fill = COLOR_CONFIG_MENU_TILE_FOCUSED;
  } else {
    fill = COLOR_CONFIG_MENU_TILE_LIGHT;
    border = COLOR_CONFIG_MENU_TILE_BORDER_LIGHT;
    text = COLOR_BLACK;
    if (pressed)
      fill = COLOR_CONFIG_MENU_TILE_PRESSED_LIGHT;
    else if (focused && selected) {
      fill = COLOR_CONFIG_MENU_TILE_FOCUSED_LIGHT;
      text = COLOR_WHITE;
    }
  }

  canvas.SelectNullPen();
  canvas.Select(Brush(border));
  canvas.DrawRoundRectangle(tile, PixelSize{diameter, diameter});

  const unsigned border_w = Layout::ScaleFinePenWidth(2);
  PixelRect inner = tile;
  if (inner.GetWidth() > 2 * border_w &&
      inner.GetHeight() > 2 * border_w) {
    inner.Grow(-int(border_w));
    const unsigned inner_diameter =
      std::min(diameter,
               std::min(unsigned(inner.GetWidth()),
                        unsigned(inner.GetHeight())));
    canvas.Select(Brush(fill));
    canvas.DrawRoundRectangle(inner,
                              PixelSize{inner_diameter, inner_diameter});
  }

  canvas.Select(*look.button.font);
  canvas.SetTextColor(text);
  canvas.SetBackgroundTransparent();

  TextRenderer text_renderer;
  text_renderer.SetCenter();
  text_renderer.SetVCenter();
  text_renderer.SetControl();
  text_renderer.Draw(canvas, inner, caption);
}

void
ConfigTileMenu::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(look.background_color);

  const bool is_focused = !HasCursorKeys() || HasFocus();

  if (submenu_main < 0) {
    for (unsigned i = 0; i < main_menu_buttons.size(); ++i) {
      const bool pressed = int(i) == down_index && !drag_off_button;
      const bool selected =
        pressed ||
        (cursor < buttons.size() &&
         buttons[cursor].main_menu_index == i);
      DrawTile(canvas, main_menu_buttons[i].rc,
               main_menu_buttons[i].caption,
               is_focused, pressed, selected);
    }
    return;
  }

  const auto &main = main_menu_buttons[submenu_main];
  for (unsigned page_i = main.first_page_index, tile = 0;
       page_i <= main.last_page_index; ++page_i, ++tile) {
    const bool pressed = int(tile) == down_index && !drag_off_button;
    const bool selected = pressed || page_i == cursor;
    DrawTile(canvas, buttons[page_i].rc, buttons[page_i].caption,
             is_focused, pressed, selected);
  }
}

void
ConfigTileMenu::OnKillFocus() noexcept
{
  Invalidate();
  PaintWindow::OnKillFocus();
}

void
ConfigTileMenu::OnSetFocus() noexcept
{
  Invalidate();
  PaintWindow::OnSetFocus();
}
