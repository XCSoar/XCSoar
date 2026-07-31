// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/Frame.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"

#include <algorithm>

WndFrame::WndFrame(const DialogLook &_look) noexcept
  :look(_look),
   text_color(look.text_color)
{
}

WndFrame::WndFrame(ContainerWindow &parent, const DialogLook &_look,
                   PixelRect rc,
                   const WindowStyle style) noexcept
  :look(_look),
   text_color(look.text_color)
{
  Create(parent, rc, style);
}

void
WndFrame::SetAlignCenter() noexcept
{
  text_renderer.SetCenter();
  Invalidate();
}

void
WndFrame::SetVAlignCenter() noexcept
{
  text_renderer.SetVCenter();
  Invalidate();
}

void
WndFrame::SetText(const char *_text) noexcept
{
  text = _text;
  text_renderer.InvalidateLayout();
  Invalidate();
}

unsigned
WndFrame::GetTextHeight() const noexcept
{
  PixelRect rc = GetClientRect();
  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  AnyCanvas canvas;
  canvas.Select(look.text_font);

  return text_renderer.GetHeight(canvas, rc, text.c_str());
}

void
WndFrame::OnPaint(Canvas &canvas) noexcept
{
  if (HaveClipping())
    canvas.Clear(look.background_brush);

  if (top_separator) {
    /* Filled strip instead of DrawLine at y=0: OpenGL often clips a
       1px stroke on the window edge. */
    const int thickness = (int)std::max(1u, Layout::ScaleFinePenWidth(1));
    canvas.DrawFilledRectangle({0, 0, (int)canvas.GetWidth(), thickness},
                               look.dark_mode ? COLOR_GRAY : COLOR_BLACK);
  }

  canvas.SetTextColor(text_color);
  canvas.SetBackgroundTransparent();

  canvas.Select(look.text_font);

  PixelRect rc = GetClientRect();
  const int padding = Layout::GetTextPadding();
  rc.Grow(-padding);

  text_renderer.Draw(canvas, rc, text.c_str());
}
