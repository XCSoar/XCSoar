// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmProgressOverlay.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/ProgressBarRenderer.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/PaintWindow.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/StaticString.hxx"

class FlarmProgressBar final : public PaintWindow {
  unsigned progress = 0;
  StaticString<128> text;

public:
  void Update(const char *_text, unsigned _progress) noexcept {
    text = _text;
    progress = _progress;
    Invalidate();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();

    DrawSimpleProgressBar(canvas, canvas.GetRect(),
                          progress, 0, 100,
                          look.dark_mode
                          ? &look.background_color : nullptr);

    canvas.Select(look.text_font);
    canvas.SetTextColor(look.text_color);
    canvas.SetBackgroundTransparent();

    const int padding = Layout::GetTextPadding();
    const int text_height = look.text_font.GetHeight();
    const int y = ((int)canvas.GetHeight() - text_height) / 2;
    canvas.DrawText({padding, y}, text.c_str());
  }
};

static FlarmProgressBar *overlay;

void
FlarmProgressOverlay::Show(const char *text, unsigned progress) noexcept
{
  auto &main_window = UIGlobals::GetMainWindow();
  const auto rc = main_window.GetClientRect();
  const unsigned height = Layout::GetMinimumControlHeight();

  const PixelRect bar_rect{
    rc.left,
    rc.bottom - (int)height,
    rc.right,
    rc.bottom,
  };

  if (overlay == nullptr) {
    overlay = new FlarmProgressBar();
    WindowStyle style;
    overlay->Create(main_window, bar_rect, style);
  } else {
    overlay->Move(bar_rect);
  }

  overlay->Update(text, progress);
  overlay->ShowOnTop();
}

void
FlarmProgressOverlay::Close() noexcept
{
  delete overlay;
  overlay = nullptr;
}
