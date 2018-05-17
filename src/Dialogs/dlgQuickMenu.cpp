/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "WidgetDialog.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/WindowWidget.hpp"
#include "Form/GridView.hpp"
#include "Form/Button.hpp"
#include "Input/InputEvents.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Event/KeyCode.hpp"
#include "Util/StaticArray.hxx"
#include "Util/Macros.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Menu/MenuData.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

class QuickMenuButtonRenderer final : public ButtonRenderer {
  const DialogLook &look;

  TextRenderer text_renderer;

  const StaticString<64> caption;

public:
  explicit QuickMenuButtonRenderer(const DialogLook &_look,
                                   const TCHAR *_caption)
    :look(_look), caption(_caption) {
    text_renderer.SetCenter();
    text_renderer.SetVCenter();
    text_renderer.SetControl();
  }

  gcc_pure
  unsigned GetMinimumButtonWidth() const override;

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  bool enabled, bool focused, bool pressed) const override;
};

unsigned
QuickMenuButtonRenderer::GetMinimumButtonWidth() const
{
  return 2 * Layout::GetTextPadding() + look.button.font->TextSize(caption).cx;
}

void
QuickMenuButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                    bool enabled, bool focused,
                                    bool pressed) const
{
  // Draw focus rectangle
  if (pressed) {
    canvas.DrawFilledRectangle(rc, look.list.pressed.background_color);
    canvas.SetTextColor(look.list.pressed.text_color);
  } else if (focused) {
    canvas.DrawFilledRectangle(rc, look.focused.background_color);
    canvas.SetTextColor(enabled
                        ? look.focused.text_color
                        : look.button.disabled.color);
  } else {
    if (HaveClipping())
      canvas.DrawFilledRectangle(rc, look.background_brush);
    canvas.SetTextColor(enabled ? look.text_color : look.button.disabled.color);
  }

  canvas.Select(*look.button.font);
  canvas.SetBackgroundTransparent();

  text_renderer.Draw(canvas, rc, caption);
}

class QuickMenu final : public WindowWidget, ActionListener {
  WndForm &dialog;
  const Menu &menu;

  GridView grid_view;

  StaticArray<Window *, GridView::MAX_ITEMS> buttons;

public:
  unsigned clicked_event;

  QuickMenu(WndForm &_dialog, const Menu &_menu)
    :dialog(_dialog), menu(_menu) {}

  void UpdateCaption();

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;
  bool SetFocus() override;
  bool KeyPress(unsigned key_code) override;

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

void
QuickMenu::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle grid_view_style;
  grid_view_style.ControlParent();
  grid_view_style.Hide();

  const auto &dialog_look = UIGlobals::GetDialogLook();

  const auto &font = *dialog_look.button.font;
  const unsigned column_width = Layout::Scale(78u);
  const unsigned row_height =
    std::max(2 * (Layout::GetTextPadding() + font.GetHeight()),
             Layout::GetMaximumControlHeight());

  grid_view.Create(parent, dialog_look, rc, grid_view_style,
                   column_width, row_height);
  SetWindow(&grid_view);

  WindowStyle buttonStyle;
  buttonStyle.TabStop();

  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    if (buttons.full())
      continue;

    const auto &menuItem = menu[i];
    if (!menuItem.IsDefined())
      continue;

    TCHAR buffer[100];
    const auto expanded =
      ButtonLabel::Expand(menuItem.label, buffer, ARRAY_SIZE(buffer));
    if (!expanded.visible)
      continue;

    PixelRect button_rc;
    button_rc.left = 0;
    button_rc.top = 0;
    button_rc.right = 80;
    button_rc.bottom = 30;
    auto *button = new Button(grid_view, button_rc, buttonStyle,
                              new QuickMenuButtonRenderer(dialog_look,
                                                          expanded.text),
                              *this, menuItem.event);
    button->SetEnabled(expanded.enabled);

    buttons.append(button);
    grid_view.AddItem(*button);
  }

  grid_view.RefreshLayout();
  UpdateCaption();
}

void
QuickMenu::Unprepare()
{
  for (auto *button : buttons)
    delete button;
}

void
QuickMenu::UpdateCaption()
{
  StaticString<32> buffer;
  unsigned pageSize = grid_view.GetNumColumns() * grid_view.GetNumRows();
  unsigned lastPage = buttons.size() / pageSize;
  buffer.Format(_T("Quick Menu  %d/%d"),
                grid_view.GetCurrentPage() + 1, lastPage + 1);
  dialog.SetCaption(buffer);
}

bool
QuickMenu::SetFocus()
{
  unsigned numColumns = grid_view.GetNumColumns();
  unsigned pageSize = numColumns * grid_view.GetNumRows();
  unsigned lastPage = buttons.size() / pageSize;
  unsigned currentPage = grid_view.GetCurrentPage();
  unsigned currentPageSize = currentPage == lastPage
    ? buttons.size() % pageSize
    : pageSize;
  unsigned centerCol = currentPageSize < numColumns
    ? currentPageSize / 2
    : numColumns / 2;
  unsigned centerRow = currentPageSize / numColumns / 2;
  unsigned centerPos = currentPage
    * pageSize + centerCol + centerRow * numColumns;

  if (centerPos >= buttons.size())
    return false;

  buttons[centerPos]->SetFocus();
  grid_view.RefreshLayout();
  return true;
}

bool
QuickMenu::KeyPress(unsigned key_code)
{
  switch (key_code) {
  case KEY_LEFT:
    grid_view.MoveFocus(GridView::Direction::LEFT);
    break;

  case KEY_RIGHT:
    grid_view.MoveFocus(GridView::Direction::RIGHT);
    break;

  case KEY_UP:
    grid_view.MoveFocus(GridView::Direction::UP);
    break;

  case KEY_DOWN:
    grid_view.MoveFocus(GridView::Direction::DOWN);
    break;

  case KEY_MENU:
    grid_view.ShowNextPage();
    SetFocus();
    break;

  default:
    return false;
  }

  UpdateCaption();
  return true;
}

void
QuickMenu::OnAction(int id)
{
  clicked_event = id;
  dialog.SetModalResult(mrOK);
}

void
dlgQuickMenuShowModal(SingleWindow &parent)
{
  const auto *menu = InputEvents::GetMenu(_T("RemoteStick"));
  if (menu == nullptr)
    return;

  const auto &dialog_look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(dialog_look);
  QuickMenu quick_menu(dialog, *menu);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _T(""), &quick_menu);

  const auto result = dialog.ShowModal();
  dialog.StealWidget();

  if (result == mrOK)
    InputEvents::ProcessEvent(quick_menu.clicked_event);
}
