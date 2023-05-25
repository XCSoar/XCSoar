// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SimulatorPromptWindow.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Gauge/LogoView.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/BitmapButtonRenderer.hpp"
#include "Simulator.hpp"
#include "Resources.hpp"

void
SimulatorPromptWindow::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();

  WindowStyle style;
  style.TabStop();

  fly_bitmap.Load(IDB_LAUNCHER1);
  fly_button.Create(*this, rc, style,
                    std::make_unique<BitmapButtonRenderer>(fly_bitmap),
                    [this](){ callback(Result::FLY); });

  sim_bitmap.Load(IDB_LAUNCHER2);
  sim_button.Create(*this, rc, style,
                    std::make_unique<BitmapButtonRenderer>(sim_bitmap),
                    [this](){ callback(Result::SIMULATOR); });

  if (have_quit_button)
    quit_button.Create(*this, look.button, _("Quit"), rc, style,
                       [this](){ callback(Result::QUIT); });
}

void
SimulatorPromptWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);

  const PixelRect rc = GetClientRect();

  const unsigned h_middle = new_size.width / 2;
  const unsigned bottom_padding = Layout::Scale(15);
  const unsigned button_width = Layout::Scale(112);
  const unsigned button_height = Layout::Scale(30);
  const unsigned label_height =
    look.text_font.GetHeight() + Layout::GetTextPadding();

  PixelRect button_rc;
  button_rc.left = h_middle - button_width;
  button_rc.right = h_middle;
  button_rc.bottom = rc.bottom - bottom_padding;
  button_rc.top = button_rc.bottom - button_height;
  fly_button.Move(button_rc);

  label_position.x = button_rc.left;
  label_position.y = button_rc.top - label_height;

  button_rc.left = button_rc.right;
  button_rc.right = h_middle + button_width;
  sim_button.Move(button_rc);

  logo_rect = rc;
  logo_rect.bottom = button_rc.top - label_height - Layout::Scale(5);

  if (have_quit_button) {
    button_rc = rc;
    button_rc.left = button_rc.right - Layout::Scale(75);
    button_rc.bottom = button_rc.top + Layout::GetMaximumControlHeight();
    quit_button.Move(button_rc);
  }
}

void
SimulatorPromptWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.ClearWhite();
  logo_view.draw(canvas, logo_rect);

  canvas.Select(look.text_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(label_position, _("What do you want to do?"));

  ContainerWindow::OnPaint(canvas);
}

#endif /* SIMULATOR_AVAILABLE */
