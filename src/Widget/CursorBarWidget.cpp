// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CursorBarWidget.hpp"

#include "Asset.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Form/Button.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "ui/window/PaintWindow.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

#include <algorithm>
#include <functional>
#include <memory>

static constexpr int SEPARATOR_H = 1;

class CursorBarLabel final : public PaintWindow {
  const DialogLook &look;
  StaticString<64> text{""};
  bool is_available = true;
  std::function<void()> click_handler;

public:
  explicit CursorBarLabel(const DialogLook &_look) noexcept
    :look(_look) {}

  void SetText(const char *_text) noexcept {
    text = _text;
    if (IsDefined())
      Invalidate();
  }

  void SetAvailable(bool avail) noexcept {
    is_available = avail;
    if (IsDefined())
      Invalidate();
  }

  void SetClickHandler(std::function<void()> handler) noexcept {
    click_handler = std::move(handler);
  }

protected:
  bool OnMouseDown(PixelPoint p) noexcept override {
    (void)p;
    if (click_handler) {
      click_handler();
      return true;
    }

    return PaintWindow::OnMouseDown(p);
  }

  void OnPaint(Canvas &canvas) noexcept override {
    const auto rc = GetClientRect();
    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(is_available
                        ? look.text_color
                        : look.button.disabled.color);
    canvas.Select(look.text_font);

    const auto text_size = canvas.CalcTextSize(text);
    const int avail_w = int(rc.GetWidth());
    const int avail_h = int(rc.GetHeight());
    const int y = std::max(0, (avail_h - int(text_size.height)) / 2);

    if (int(text_size.width) <= avail_w) {
      const int x = (avail_w - int(text_size.width)) / 2;
      canvas.DrawText({x, y}, text);
    } else {
      /* Keep long labels visible: DrawText with a negative/huge x from
         unsigned underflow is clipped away entirely on OpenGL. */
      canvas.DrawClippedText({0, y}, unsigned(avail_w), text);
    }
  }
};

class CursorBarWidget::BarWindow final : public SolidContainerWindow {
  CursorBarWidget &owner;
  const DialogLook &look;
  const unsigned row_count;

  StaticArray<Button, MAX_ROWS> prev_buttons;
  StaticArray<Button, MAX_ROWS> next_buttons;
  StaticArray<std::unique_ptr<CursorBarLabel>, MAX_ROWS> labels;

  /**
   * Prev/next width is based on the compact control height, not the
   * tall touch row height, so phone bars keep enough room for labels.
   */
  static int CalcButtonWidth() noexcept {
    return int(Layout::GetMinimumControlHeight()) * 2;
  }

  static void
  ComputeLayout(const PixelRect &rc, unsigned rows,
                int &total_h, int &row_h, int &btn_w, int &w) noexcept
  {
    w = std::max(1, (int)rc.GetWidth());
    const unsigned separators = rows > 1 ? rows - 1 : 0;
    const unsigned preferred_h = CursorBarWidget::DefaultHeight(rows);
    total_h = std::max(int(SEPARATOR_H) + 2,
                       std::min((int)rc.GetHeight(), (int)preferred_h));
    row_h = std::max(1, (total_h - int(separators) * SEPARATOR_H)
                     / int(rows));

    /* Cap at 1/5 of the bar so both buttons leave most of the width
       for the centre label on narrow phones. */
    const int max_btn_w = std::max(1, w / 5);
    btn_w = std::max(1, std::min(CalcButtonWidth(), max_btn_w));
  }

  [[gnu::pure]]
  static int RowTop(unsigned row, int row_h) noexcept {
    return int(row) * (row_h + SEPARATOR_H);
  }

public:
  BarWindow(CursorBarWidget &_owner, const DialogLook &_look,
            unsigned _row_count) noexcept
    :owner(_owner), look(_look), row_count(_row_count)
  {
    prev_buttons.resize(row_count);
    next_buttons.resize(row_count);
    labels.resize(row_count);
  }

  void CreateBar(ContainerWindow &parent, const PixelRect &rc) noexcept {
    WindowStyle style;
    style.Hide();
    style.ControlParent();
    SolidContainerWindow::Create(parent, rc, look.background_color, style);
    SetGradientTopColor(look.background_gradient_top_color);

    int total_h, row_h, btn_w, w;
    ComputeLayout(rc, row_count, total_h, row_h, btn_w, w);
    (void)total_h;

    WindowStyle child_style;

    for (unsigned row = 0; row < row_count; ++row) {
      const int top = RowTop(row, row_h);
      const int bottom = top + row_h;

      prev_buttons[row].Create(*this, {0, top, btn_w, bottom}, child_style,
                               std::make_unique<SymbolButtonRenderer>(
                                 look.button, "<"),
                               [this, row]{
                                 owner.InvokeStep(row, -1);
                               });
      next_buttons[row].Create(*this, {w - btn_w, top, w, bottom},
                               child_style,
                               std::make_unique<SymbolButtonRenderer>(
                                 look.button, ">"),
                               [this, row]{
                                 owner.InvokeStep(row, +1);
                               });

      WindowStyle label_style;
      labels[row] = std::make_unique<CursorBarLabel>(look);
      labels[row]->Create(*this, {btn_w, top, w - btn_w, bottom},
                          label_style);
      labels[row]->SetClickHandler([this, row]{
        owner.InvokeLabelClick(row);
      });
    }
  }

  void LayoutBar(const PixelRect &rc) noexcept {
    int total_h, row_h, btn_w, w;
    ComputeLayout(rc, row_count, total_h, row_h, btn_w, w);
    (void)total_h;

    for (unsigned row = 0; row < row_count; ++row) {
      const int top = RowTop(row, row_h);
      const int bottom = top + row_h;

      if (prev_buttons[row].IsDefined())
        prev_buttons[row].Move({0, top, btn_w, bottom});
      if (next_buttons[row].IsDefined())
        next_buttons[row].Move({w - btn_w, top, w, bottom});
      if (labels[row] != nullptr && labels[row]->IsDefined())
        labels[row]->Move({btn_w, top, w - btn_w, bottom});
    }
  }

  void SetRowText(unsigned row, const char *text, bool available) noexcept {
    if (row >= row_count || labels[row] == nullptr)
      return;

    labels[row]->SetText(text);
    labels[row]->SetAvailable(available);
  }

protected:
  void OnResize(PixelSize new_size) noexcept override {
    SolidContainerWindow::OnResize(new_size);

    if (prev_buttons[0].IsDefined())
      LayoutBar(GetClientRect());
  }

  void OnPaint(Canvas &canvas) noexcept override {
    SolidContainerWindow::OnPaint(canvas);

    if (row_count < 2)
      return;

    int total_h, row_h, btn_w, w;
    ComputeLayout(GetClientRect(), row_count, total_h, row_h, btn_w, w);
    (void)total_h;
    (void)btn_w;

    const Pen separator_pen(1, look.ReadOnlyValueBorderColor());
    canvas.Select(separator_pen);

    for (unsigned row = 1; row < row_count; ++row) {
      const int y = RowTop(row, row_h);
      canvas.DrawLine({0, y}, {w, y});
    }
  }
};

CursorBarWidget::CursorBarWidget(unsigned _row_count) noexcept
  :row_count(std::clamp(_row_count, 1U, MAX_ROWS)) {}

unsigned
CursorBarWidget::DefaultHeight(unsigned rows) noexcept
{
  rows = std::clamp(rows, 1U, MAX_ROWS);
  const unsigned row_h = HasTouchScreen()
    ? Layout::GetMaximumControlHeight()
    : Layout::GetMinimumControlHeight();
  const unsigned separators = rows > 1 ? rows - 1 : 0;
  return row_h * rows + separators * SEPARATOR_H;
}

PixelSize
CursorBarWidget::GetMinimumSize() const noexcept
{
  return {100U, DefaultHeight(row_count)};
}

PixelSize
CursorBarWidget::GetMaximumSize() const noexcept
{
  return {4096U, DefaultHeight(row_count)};
}

void
CursorBarWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto w = std::make_unique<BarWindow>(*this, look, row_count);
  w->CreateBar(parent, rc);
  SetWindow(std::move(w));
}

void
CursorBarWidget::Unprepare() noexcept
{
  DeleteWindow();
}

void
CursorBarWidget::RelayoutBar() noexcept
{
  if (!IsDefined())
    return;

  static_cast<BarWindow &>(GetWindow()).LayoutBar(GetWindow().GetClientRect());
}

void
CursorBarWidget::Show(const PixelRect &rc) noexcept
{
  assert(IsDefined());

  PixelRect safe_rc = rc;
  if (safe_rc.GetHeight() == 0)
    safe_rc.bottom = safe_rc.top + 1;

  Window &window = GetWindow();
  window.Move(safe_rc);
  RelayoutBar();
  window.Show();
}

void
CursorBarWidget::Move(const PixelRect &rc) noexcept
{
  if (!IsDefined())
    return;

  WindowWidget::Move(rc);

  if (GetWindow().IsVisible())
    RelayoutBar();
}

void
CursorBarWidget::SetRowText(unsigned row, const char *text,
                            bool available) noexcept
{
  if (!IsDefined())
    return;

  static_cast<BarWindow &>(GetWindow()).SetRowText(row, text, available);
}

void
CursorBarWidget::InvokeStep(unsigned row, int delta) const noexcept
{
  if (step_callback)
    step_callback(row, delta);
}

void
CursorBarWidget::InvokeLabelClick(unsigned row) const noexcept
{
  if (label_click_callback)
    label_click_callback(row);
}
