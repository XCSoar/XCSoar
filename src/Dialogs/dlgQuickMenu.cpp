/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Renderer/ButtonRenderer.hpp"
#include "Form/GridView.hpp"
#include "Form/Button.hpp"
#include "Input/InputEvents.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Key.h"
#include "Screen/SingleWindow.hpp"
#include "Form/Form.hpp"
#include "Util/TrivialArray.hpp"
#include "Util/Macros.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Menu/MenuData.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

class QuickMenuButtonRenderer final : public ButtonRenderer {
  const DialogLook &look;

  const StaticString<64> caption;

public:
  explicit QuickMenuButtonRenderer(const DialogLook &_look,
                                   const TCHAR *_caption)
    :look(_look), caption(_caption) {}

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

#ifndef USE_GDI
  unsigned style = DT_CENTER | DT_VCENTER | DT_WORDBREAK;

  if (IsDithered())
    style |= DT_UNDERLINE;
#else
  unsigned style = DT_CENTER | DT_NOCLIP | DT_WORDBREAK;
#endif

  PixelRect text_rc = rc;
  canvas.DrawFormattedText(&text_rc, caption, style);
}

static WndForm *wf;
static GridView *grid_view;

static TrivialArray<Window *, GridView::MAX_ITEMS> buttons;

class QuickMenu : public ActionListener {
public:
  unsigned clicked_event;

  virtual void OnAction(int id) override;
};

static void
SetFormCaption()
{
  StaticString<32> buffer;
  unsigned pageSize = grid_view->GetNumColumns() * grid_view->GetNumRows();
  unsigned lastPage = buttons.size() / pageSize;
  buffer.Format(_T("Quick Menu  %d/%d"),
                grid_view->GetCurrentPage() + 1, lastPage + 1);
  wf->SetCaption(buffer);
}

static void
SetFormDefaultFocus()
{
  unsigned numColumns = grid_view->GetNumColumns();
  unsigned pageSize = numColumns * grid_view->GetNumRows();
  unsigned lastPage = buttons.size() / pageSize;
  unsigned currentPage = grid_view->GetCurrentPage();
  unsigned currentPageSize = currentPage == lastPage
    ? buttons.size() % pageSize
    : pageSize;
  unsigned centerCol = currentPageSize < numColumns
    ? currentPageSize / 2
    : numColumns / 2;
  unsigned centerRow = currentPageSize / numColumns / 2;
  unsigned centerPos = currentPage
    * pageSize + centerCol + centerRow * numColumns;

  if (centerPos < buttons.size()) {
    if (wf->IsVisible()) {
      buttons[centerPos]->SetFocus();
      grid_view->RefreshLayout();
    } else if (buttons[centerPos]->IsEnabled())
      wf->SetDefaultFocus(buttons[centerPos]);
  }
}

static bool
FormKeyDown(unsigned key_code)
{
  switch (key_code) {
  case KEY_LEFT:
    grid_view->MoveFocus(GridView::Direction::LEFT);
    break;

  case KEY_RIGHT:
    grid_view->MoveFocus(GridView::Direction::RIGHT);
    break;

  case KEY_UP:
    grid_view->MoveFocus(GridView::Direction::UP);
    break;

  case KEY_DOWN:
    grid_view->MoveFocus(GridView::Direction::DOWN);
    break;

  case KEY_MENU:
    grid_view->ShowNextPage();
    SetFormDefaultFocus();
    break;

#ifdef GNAV
  // Altair RemoteStick
  case KEY_F11:
    grid_view->MoveFocus(GridView::Direction::UP);
    break;

  case KEY_F12:
    grid_view->MoveFocus(GridView::Direction::DOWN);
    break;

  case KEY_F13:
    grid_view->MoveFocus(GridView::Direction::RIGHT);
    break;

  case KEY_F14:
    grid_view->MoveFocus(GridView::Direction::LEFT);
    break;

  case KEY_F15:
       wf->SetModalResult(mrCancel);
  break;
#endif

  default:
    return false;
  }

  SetFormCaption();
  return true;
}

void
QuickMenu::OnAction(int id)
{
  clicked_event = id;
  wf->SetModalResult(mrOK);
}

void
dlgQuickMenuShowModal(SingleWindow &parent)
{
  const Menu *menu = InputEvents::GetMenu(_T("RemoteStick"));
  if (menu == NULL)
    return;

  QuickMenu quick_menu;

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  WindowStyle dialogStyle;
  dialogStyle.Hide();
  dialogStyle.ControlParent();

  wf = new WndForm(parent, dialog_look, parent.GetClientRect(),
                   _T("Quick Menu"), dialogStyle);

  ContainerWindow &client_area = wf->GetClientAreaWindow();

  PixelRect r = client_area.GetClientRect();

  WindowStyle grid_view_style;
  grid_view_style.ControlParent();

  grid_view = new GridView(client_area, r,
                           dialog_look, grid_view_style);

  WindowStyle buttonStyle;
  buttonStyle.TabStop();

  for (unsigned i = 0; i < menu->MAX_ITEMS; ++i) {
    if (buttons.full())
      continue;

    const MenuItem &menuItem = (*menu)[i];
    if (!menuItem.IsDefined())
      continue;

    TCHAR buffer[100];
    ButtonLabel::Expanded expanded =
      ButtonLabel::Expand(menuItem.label, buffer, ARRAY_SIZE(buffer));
    if (!expanded.visible)
      continue;

    PixelRect button_rc;
    button_rc.left = 0;
    button_rc.top = 0;
    button_rc.right = 80;
    button_rc.bottom = 30;
    WndButton *button =
      new WndButton(*grid_view, button_rc, buttonStyle,
                    new QuickMenuButtonRenderer(dialog_look,
                                                expanded.text),
                    quick_menu, menuItem.event);
    button->SetEnabled(expanded.enabled);

    buttons.append(button);
  }

  grid_view->SetItems(buttons);
  SetFormDefaultFocus();
  SetFormCaption();

  wf->SetKeyDownFunction(FormKeyDown);

  int result = wf->ShowModal();

  for (auto it = buttons.begin(), end = buttons.end(); it != end; ++it)
    delete *it;

  buttons.clear();

  delete wf;
  delete grid_view;

  if (result == mrOK)
    InputEvents::ProcessEvent(quick_menu.clicked_event);
}
