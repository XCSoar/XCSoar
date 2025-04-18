// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include "Dialogs/Dialogs.h"
#include "Form/Button.hpp"
#include "Form/GridView.hpp"
#include "Input/InputEvents.hpp"
#include "Look/DialogLook.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Menu/MenuData.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Widget/WindowWidget.hpp"
#include "WidgetDialog.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "util/StaticString.hxx"

#include <boost/container/static_vector.hpp>

#include <stdio.h>

class QuickMenuButtonRenderer final : public ButtonRenderer {
  const DialogLook &look;

  TextRenderer text_renderer;

  const StaticString<64> caption;

public:
  explicit QuickMenuButtonRenderer(const DialogLook &_look,
                                   const TCHAR *_caption) noexcept
    :look(_look), caption(_caption) {
    text_renderer.SetCenter();
    text_renderer.SetVCenter();
    text_renderer.SetControl();
  }

  [[gnu::pure]]
  unsigned GetMinimumButtonWidth() const noexcept override;

  void DrawButton(Canvas &canvas, const PixelRect &rc,
                  ButtonState state) const noexcept override;
};

unsigned
QuickMenuButtonRenderer::GetMinimumButtonWidth() const noexcept
{
  return 2 * Layout::GetTextPadding() + look.button.font->TextSize(caption).width;
}

void
QuickMenuButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                    ButtonState state) const noexcept
{
  // Draw focus rectangle
  switch (state) {
  case ButtonState::PRESSED:
    canvas.DrawFilledRectangle(rc, look.list.pressed.background_color);
    canvas.SetTextColor(look.list.pressed.text_color);
    break;

  case ButtonState::FOCUSED:
    canvas.DrawFilledRectangle(rc, look.focused.background_color);
    canvas.SetTextColor(look.focused.text_color);
    break;

  case ButtonState::SELECTED:
  case ButtonState::ENABLED:
    if (HaveClipping())
      canvas.DrawFilledRectangle(rc, look.background_brush);
    canvas.SetTextColor(look.text_color);
    break;

  case ButtonState::DISABLED:
    if (HaveClipping())
      canvas.DrawFilledRectangle(rc, look.background_brush);
    canvas.SetTextColor(look.button.disabled.color);
    break;
  }

  canvas.Select(*look.button.font);
  canvas.SetBackgroundTransparent();

  text_renderer.Draw(canvas, rc, caption);
}

class QuickMenu final : public WindowWidget {
  WndForm &dialog;
  const Menu &menu;

  boost::container::static_vector<Button, GridView::MAX_ITEMS> buttons;

public:
  unsigned clicked_event;

  QuickMenu(WndForm &_dialog, const Menu &_menu) noexcept
    :dialog(_dialog), menu(_menu) {}

  auto &GetWindow() noexcept {
    return (GridView &)WindowWidget::GetWindow();
  }

  void UpdateCaption() noexcept;

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void AddNavigationButtons(GridView *grid_view,
                            const WindowStyle &buttonStyle,
                            const DialogLook &);
  bool SetFocus() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};
void
QuickMenu::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle grid_view_style;
  grid_view_style.ControlParent();
  grid_view_style.Hide();

  const auto &dialog_look = UIGlobals::GetDialogLook();

  const auto &font = *dialog_look.button.font;

  const unsigned min_column_width = 2 * Layout::GetMaximumControlHeight();
  const unsigned min_columns = 3;
  const unsigned max_column_width = rc.GetWidth() / min_columns;
  const unsigned desired_column_width = Layout::PtScale(160);
  const unsigned column_width = std::clamp(desired_column_width,
                                           min_column_width, max_column_width);

  const unsigned row_height =
    std::max(2 * (Layout::GetTextPadding() + font.GetHeight()),
             Layout::GetMaximumControlHeight());

  auto grid_view = std::make_unique<GridView>();
  grid_view->Create(parent, dialog_look, rc, grid_view_style,
                    column_width, row_height);

  WindowStyle buttonStyle;
  buttonStyle.TabStop();

  // Calculate the number of buttons that fit on a single page
  unsigned num_columns = rc.GetWidth() / column_width;
  unsigned num_rows = rc.GetHeight() / row_height;
  unsigned page_size = num_columns * num_rows - 3;

  unsigned buttons_added = 0;

  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    if (buttons.size() >= buttons.max_size())
      continue;

    const auto &menuItem = menu[i];
    if (!menuItem.IsDefined())
      continue;

    TCHAR buffer[100];
    const auto expanded =
      ButtonLabel::Expand(menuItem.label, std::span{buffer});
    if (!expanded.visible)
      continue;

    PixelRect button_rc;
    button_rc.left = 0;
    button_rc.top = 0;
    button_rc.right = 80;
    button_rc.bottom = 30;

    auto &button = buttons.emplace_back(*grid_view, button_rc, buttonStyle,
                                        std::make_unique<QuickMenuButtonRenderer>(dialog_look,
                                                                                  expanded.text),
                                        [this, &menuItem](){
                                          clicked_event = menuItem.event;
                                          dialog.SetModalResult(mrOK);
                                        });
    button.SetEnabled(expanded.enabled);

    grid_view->AddItem(button);
    buttons_added++;

    if (buttons_added % page_size == 0 && HasTouchScreen())
    {
      AddNavigationButtons(grid_view.get(), buttonStyle, dialog_look);
    }
  }

  /* if the last page wasn't completley filled,
     still add the navigation buttons */
  if (buttons_added % page_size != 0 && HasTouchScreen())
  {
    AddNavigationButtons(grid_view.get(), buttonStyle, dialog_look);
  }

  grid_view->RefreshLayout();
  SetWindow(std::move(grid_view));
  UpdateCaption();
}

void
QuickMenu::AddNavigationButtons(GridView *grid_view,
                                const WindowStyle &buttonStyle,
                                const DialogLook &dialog_look)
{
  PixelRect button_rc;
  button_rc.left = 0;
  button_rc.top = 0;
  button_rc.right = 80;
  button_rc.bottom = 30;

  // Add "Previous Page" button
  auto &previous_button =
      buttons.emplace_back(*grid_view, button_rc, buttonStyle,
                           std::make_unique<QuickMenuButtonRenderer>(
                               dialog_look, _T("Previous Page")),
                           [this]()
                           {
                             auto &grid_view = GetWindow();
                             grid_view.ShowNextPage(GridView::Direction::LEFT);
                             SetFocus();
                             UpdateCaption();
                           });
  previous_button.SetEnabled(true);
  grid_view->AddItem(previous_button);
  // Add "Next Page" button
  auto &next_button = buttons.emplace_back(
      *grid_view, button_rc, buttonStyle,
      std::make_unique<QuickMenuButtonRenderer>(dialog_look, _T("Next Page")),
      [this]()
      {
        auto &grid_view = GetWindow();
        grid_view.ShowNextPage();
        SetFocus();
        UpdateCaption();
      });
  next_button.SetEnabled(true);
  grid_view->AddItem(next_button);

  // Add "Cancel" button
  auto &cancel_button = buttons.emplace_back(
      *grid_view, button_rc, buttonStyle,
      std::make_unique<QuickMenuButtonRenderer>(dialog_look, _T("Cancel")),
      [this]() { dialog.SetModalResult(mrCancel); });
  cancel_button.SetEnabled(true);
  grid_view->AddItem(cancel_button);
}

void
QuickMenu::UpdateCaption() noexcept
{
  auto &grid_view = GetWindow();
  StaticString<32> buffer;
  unsigned pageSize = GetWindow().GetNumColumns() * grid_view.GetNumRows();
  unsigned lastPage = buttons.size() / pageSize;
  buffer.Format(_T("Quick Menu  %d/%d"),
                grid_view.GetCurrentPage() + 1, lastPage + 1);
  dialog.SetCaption(buffer);
}

bool
QuickMenu::SetFocus() noexcept
{
  auto &grid_view = GetWindow();
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

  buttons[centerPos].SetFocus();
  grid_view.RefreshLayout();
  return true;
}

bool
QuickMenu::KeyPress(unsigned key_code) noexcept
{
  auto &grid_view = GetWindow();

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

static int
ShowQuickMenu(UI::SingleWindow &parent, const Menu &menu) noexcept
{
  const auto &dialog_look = UIGlobals::GetDialogLook();

  TWidgetDialog<QuickMenu> dialog(WidgetDialog::Full{},
                                  parent,
                                  dialog_look, nullptr);

  dialog.SetWidget(dialog, menu);
  if (dialog.ShowModal() != mrOK)
    return -1;

  return dialog.GetWidget().clicked_event;
}

void
dlgQuickMenuShowModal(UI::SingleWindow &parent) noexcept
{
  const auto *menu = InputEvents::GetMenu(_T("RemoteStick"));
  if (menu == nullptr)
    return;

  const int event = ShowQuickMenu(parent, *menu);
  if (event >= 0)
    InputEvents::ProcessEvent(event);
}
