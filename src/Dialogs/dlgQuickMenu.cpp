// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include "Dialogs/Dialogs.h"
#include "Form/Button.hpp"
#include "Form/GridView.hpp"
#include "Input/InputEvents.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Math/Util.hpp"
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
#include <cstdlib>
#include <memory>

class QuickMenuButtonRenderer final : public ButtonRenderer {
  const DialogLook &look;

  TextRenderer text_renderer;

  const StaticString<64> caption;

public:
  explicit QuickMenuButtonRenderer(const DialogLook &_look,
                                   const char *_caption) noexcept
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

  unsigned row_height = 0;
  unsigned titlebar_height = 0;
  unsigned column_width = 0;

  Button *previous_button = nullptr;
  Button *next_button = nullptr;

  static constexpr unsigned MIN_COLUMNS = 3;
  static constexpr unsigned DESIRED_COLUMNS = 3;
  static const unsigned DESIRED_COLUMN_WIDTH;

  void CalculateColumnWidth(unsigned available_width) noexcept;
  PixelRect CalculateConstrainedGridRect(const PixelRect &rc) const noexcept;

public:
  unsigned clicked_event;

  QuickMenu(WndForm &_dialog, const Menu &_menu) noexcept
    :dialog(_dialog), menu(_menu) {}

  auto &GetWindow() noexcept {
    return (GridView &)WindowWidget::GetWindow();
  }

  void NavigatePage(GridView::Direction direction) noexcept;
  void UpdateCaption() noexcept;
  void SetNavigationButtons(Button *prev, Button *next) noexcept {
    previous_button = prev;
    next_button = next;
  }

  bool Focus() noexcept {
    return SetFocus();
  }

  bool IsWindowReady() const noexcept {
    return IsDefined();
  }

protected:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;
};
void
QuickMenu::Prepare(ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  WindowStyle grid_view_style;
  grid_view_style.ControlParent();
  grid_view_style.Hide();

  const auto &dialog_look = UIGlobals::GetDialogLook();

  const auto &font = *dialog_look.button.font;

  titlebar_height = dialog_look.caption.font->GetHeight();

  PixelRect client_rc = dialog.GetClientAreaWindow().GetClientRect();
  const unsigned original_height = client_rc.GetHeight();
  if (original_height > titlebar_height) {
    client_rc.bottom = client_rc.top + (original_height - titlebar_height);
  }

  const unsigned available_width = client_rc.GetWidth();
  unsigned num_cols = std::max(MIN_COLUMNS,
                                std::min(DESIRED_COLUMNS,
                                        available_width / DESIRED_COLUMN_WIDTH));
  if (num_cols == 0) num_cols = 1;
  column_width = available_width / num_cols;

  row_height =
    std::max(2 * (Layout::GetTextPadding() + font.GetHeight()),
             Layout::GetMaximumControlHeight());

  const PixelRect constrained_grid_rc = CalculateConstrainedGridRect(client_rc);
  auto grid_view = std::make_unique<GridView>();
  grid_view->Create(parent, dialog_look, constrained_grid_rc, grid_view_style,
                    column_width, row_height);

  WindowStyle buttonStyle;
  buttonStyle.TabStop();

  grid_view->RefreshLayout();

  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    if (buttons.size() >= buttons.max_size())
      continue;

    const auto &menuItem = menu[i];
    if (!menuItem.IsDefined())
      continue;

    char buffer[100];
    const auto expanded =
      ButtonLabel::Expand(menuItem.label, std::span{buffer});
    if (!expanded.visible)
      continue;

    PixelRect button_rc;
    button_rc.left = 0;
    button_rc.top = 0;
    button_rc.right = Layout::Scale(80);
    button_rc.bottom = Layout::Scale(30);

    auto &button = buttons.emplace_back(*grid_view, button_rc, buttonStyle,
                                        std::make_unique<QuickMenuButtonRenderer>(dialog_look,
                                                                                  expanded.text),
                                        [this, &menuItem](){
                                          clicked_event = menuItem.event;
                                          dialog.SetModalResult(mrOK);
                                        });
    button.SetEnabled(expanded.enabled);

    grid_view->AddItem(button);
  }

  grid_view->RefreshLayout();

  SetWindow(std::move(grid_view));
  UpdateCaption();
}

const unsigned QuickMenu::DESIRED_COLUMN_WIDTH = Layout::PtScale(160);

void
QuickMenu::CalculateColumnWidth(unsigned available_width) noexcept
{
  unsigned num_cols = std::max(MIN_COLUMNS,
                               std::min(DESIRED_COLUMNS,
                                       available_width / DESIRED_COLUMN_WIDTH));
  if (num_cols == 0) num_cols = 1;
  column_width = available_width / num_cols;
}

void
QuickMenu::NavigatePage(GridView::Direction direction) noexcept
{
  if (!IsWindowReady())
    return;
  auto &grid_view = GetWindow();
  grid_view.RefreshLayout();
  grid_view.ShowNextPage(direction);
  Focus();
  UpdateCaption();
}

PixelRect
QuickMenu::CalculateConstrainedGridRect(const PixelRect &rc) const noexcept
{
  if (row_height == 0)
    return rc;

  const unsigned available_height = rc.GetHeight();
  const unsigned constrained_height = available_height > row_height / 2
    ? available_height - row_height / 2
    : available_height;
  return PixelRect(0, 0, rc.GetWidth(), constrained_height);
}

void
QuickMenu::Show(const PixelRect &rc) noexcept
{
  CalculateColumnWidth(rc.GetWidth());

  const PixelRect constrained_rc = CalculateConstrainedGridRect(rc);
  WindowWidget::Show(constrained_rc);

  auto &grid_view = GetWindow();
  grid_view.SetColumnWidth(column_width);
  grid_view.RefreshLayout();

  UpdateCaption();
}

void
QuickMenu::Move(const PixelRect &rc) noexcept
{
  CalculateColumnWidth(rc.GetWidth());

  const PixelRect constrained_rc = CalculateConstrainedGridRect(rc);
  WindowWidget::Move(constrained_rc);

  auto &grid_view = GetWindow();
  grid_view.SetColumnWidth(column_width);
  grid_view.RefreshLayout();

  UpdateCaption();
}



void
QuickMenu::UpdateCaption() noexcept
{
  auto &grid_view = GetWindow();
  StaticString<32> buffer;
  unsigned pageSize = grid_view.GetNumColumns() * grid_view.GetNumRows();
  unsigned lastPage = std::max<unsigned>(1, DivideRoundUp(buttons.size(), pageSize));
  unsigned currentPage = std::min(grid_view.GetCurrentPage(), lastPage - 1u);

  if (lastPage > 1) {
    buffer.Format(_T("Quick Menu  %d/%d"),
                  currentPage + 1, lastPage);
  } else {
    buffer = _T("Quick Menu");
  }
  dialog.SetCaption(buffer);

  if (previous_button != nullptr) {
    previous_button->SetEnabled(lastPage > 1);
  }
  if (next_button != nullptr) {
    next_button->SetEnabled(lastPage > 1);
  }
}

bool
QuickMenu::SetFocus() noexcept
{
  auto &grid_view = GetWindow();

  grid_view.RefreshLayout();

  unsigned numColumns = grid_view.GetNumColumns();
  unsigned numRows = grid_view.GetNumRows();
  unsigned pageSize = numColumns * numRows;
  unsigned lastPage = buttons.size() > 0
    ? DivideRoundUp(buttons.size(), pageSize) - 1
    : 0;
  unsigned currentPage = std::min(grid_view.GetCurrentPage(), lastPage);
  unsigned currentPageSize = currentPage == lastPage
    ? buttons.size() % pageSize
    : pageSize;
  if (currentPageSize == 0 && buttons.size() > 0)
    currentPageSize = pageSize;
  
  unsigned maxRowsOnPage = DivideRoundUp(currentPageSize, numColumns);
  unsigned centerCol = numColumns / 2;
  unsigned centerRow = maxRowsOnPage / 2;

  const unsigned pageStart = currentPage * pageSize;
  const unsigned pageEnd = std::min(pageStart + currentPageSize, (unsigned)buttons.size());
  unsigned focusIndex = buttons.size(); // not found

  auto is_focusable = [](const Button &b) noexcept {
    return b.IsVisible() && b.IsEnabled() && b.IsTabStop();
  };

  auto manhattan = [numColumns, pageSize](unsigned idx, unsigned cCol, unsigned cRow) noexcept {
    unsigned pagePos = idx % pageSize;
    unsigned col = pagePos % numColumns;
    unsigned row = pagePos / numColumns;
    return std::abs((int)col - (int)cCol) + std::abs((int)row - (int)cRow);
  };

  int bestDist = INT_MAX;
  for (unsigned i = pageStart; i < pageEnd; ++i) {
    if (!is_focusable(buttons[i]))
      continue;
    int dist = manhattan(i, centerCol, centerRow);
    if (dist < bestDist) {
      bestDist = dist;
      focusIndex = i;
      if (dist == 0) break; // perfect center, stop searching
    }
  }

  if (focusIndex >= buttons.size()) {
    for (unsigned page = 0; page <= lastPage; ++page) {
      unsigned pStart = page * pageSize;
      unsigned pSize = page == lastPage ? buttons.size() % pageSize : pageSize;
      if (pSize == 0 && buttons.size() > 0)
        pSize = pageSize;
      unsigned pEnd = std::min(pStart + pSize, (unsigned)buttons.size());
      for (unsigned i = pStart; i < pEnd; ++i) {
        if (is_focusable(buttons[i])) {
          while (grid_view.GetCurrentPage() != page) {
            if (grid_view.GetCurrentPage() < page)
              grid_view.ShowNextPage(GridView::Direction::RIGHT);
            else
              grid_view.ShowNextPage(GridView::Direction::LEFT);
          }
          buttons[i].SetFocus();
          return true;
        }
      }
    }
    return false;
  }

  buttons[focusIndex].SetFocus();
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

class QuickMenuDialog final : public WidgetDialog {
  QuickMenu *quick_menu_widget = nullptr;

public:
  QuickMenuDialog(Full, UI::SingleWindow &parent, const DialogLook &look,
                  const char *caption) noexcept
    :WidgetDialog(Full{}, parent, look, caption) {}

  template<typename... Args>
  void SetWidget(Args&&... args) {
    auto widget = std::make_unique<QuickMenu>(std::forward<Args>(args)...);
    quick_menu_widget = widget.get();
    FinishPreliminary(std::move(widget));
  }

  // Intentionally hides WidgetDialog::GetWidget() to return QuickMenu&
  // instead of Widget& for type-specific behavior
  QuickMenu &GetWidget() noexcept {
    return *quick_menu_widget;
  }

  // Intentionally hides WndForm::ShowModal() to provide custom modal
  // behavior with auto-sizing and button layout
  int ShowModal() {
    if (IsAutoSize())
      AutoSize();
    else
      widget.Move(buttons.BottomLayout());

    widget.Show();
    int result = WndForm::ShowModal();
    widget.Hide();
    return result;
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    WndForm::OnResize(new_size);

    if (IsAutoSize())
      return;

    widget.Move(buttons.BottomLayout());
  }
};

static int
ShowQuickMenu(UI::SingleWindow &parent, const Menu &menu) noexcept
{
  const auto &dialog_look = UIGlobals::GetDialogLook();

  QuickMenuDialog dialog(WidgetDialog::Full{},
                         parent,
                         dialog_look, nullptr);

  dialog.SetWidget(dialog, menu);

  dialog.PrepareWidget();

  auto &quick_menu = dialog.GetWidget();
  Button *prev_button = dialog.AddSymbolButton(_T("<"), [&quick_menu]() {
    quick_menu.NavigatePage(GridView::Direction::LEFT);
  });

  Button *next_button = dialog.AddSymbolButton(_T(">"), [&quick_menu]() {
    quick_menu.NavigatePage(GridView::Direction::RIGHT);
  });

  dialog.AddButton(_("Cancel"), mrCancel);

  quick_menu.SetNavigationButtons(prev_button, next_button);

  quick_menu.UpdateCaption();
  
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
