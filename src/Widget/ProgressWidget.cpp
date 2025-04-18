// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressWidget.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/ProgressBarRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "util/tstring_view.hxx"
#include "UIGlobals.hpp"

class ProgressWidget::ProgressBar final : public PaintWindow {
  unsigned range = 0, position = 0;

  tstring text;

public:
  explicit ProgressBar(tstring &&_text) noexcept
    :text(std::move(_text)) {}

  void SetRange(unsigned _range) noexcept {
    if (_range == range) return;
    range = _range;
    Invalidate();
  }

  void SetPosition(unsigned _position) noexcept {
    if (_position == position) return;
    position = _position;
    Invalidate();
  }

  void SetText(const TCHAR *_text) noexcept {
    text = _text;
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    DrawSimpleProgressBar(canvas, canvas.GetRect(), position, 0, range);

    if (!text.empty()) {
      auto &look = UIGlobals::GetDialogLook();
      auto &font = look.text_font;
      canvas.Select(font);

      const int text_height = font.GetHeight();
      const int padding = ((int)canvas.GetHeight() - text_height) / 2;

      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundTransparent();

      const tstring_view _text{text};
      canvas.DrawText({padding, padding}, _text);
    }
  }
};

PixelSize
ProgressWidget::GetMinimumSize() const noexcept
{
  return {
    Layout::GetMaximumControlHeight(),
    Layout::GetMinimumControlHeight(),
  };
}

PixelSize
ProgressWidget::GetMaximumSize() const noexcept
{
  return {
    4096U,
    Layout::GetMaximumControlHeight(),
  };
}

void
ProgressWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle hidden;
  hidden.Hide();

  auto w = std::make_unique<ProgressBar>(std::move(text));
  w->Create(parent, rc, hidden);
  SetWindow(std::move(w));

  env.SetOperationEnvironment(*this);
}

void
ProgressWidget::Unprepare() noexcept
{
  env.SetOperationEnvironment(nullptr);
  DeleteWindow();
}

void
ProgressWidget::SetText(const TCHAR *_text) noexcept
{
  if (IsDefined()) {
    auto &pb = (ProgressBar &)GetWindow();
    pb.SetText(_text);
  } else
    text = _text;
}

void
ProgressWidget::SetProgressRange(unsigned range) noexcept
{
  auto &pb = (ProgressBar &)GetWindow();
  pb.SetRange(range);
}

void
ProgressWidget::SetProgressPosition(unsigned position) noexcept
{
  auto &pb = (ProgressBar &)GetWindow();
  pb.SetPosition(position);
}
