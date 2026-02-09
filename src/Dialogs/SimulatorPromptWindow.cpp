// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SimulatorPromptWindow.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "Look/DialogLook.hpp"
#include "Look/Colors.hpp"
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

#ifndef USE_WIN32_RESOURCES
  fly_bitmap.Load(IDB_LAUNCHER1_RGBA);
#else
  fly_bitmap.Load(IDB_LAUNCHER1);
#endif
  fly_button.Create(*this, rc, style,
                    std::make_unique<BitmapButtonRenderer>(fly_bitmap, true),
                    [this](){ callback(Result::FLY); });

#ifndef USE_WIN32_RESOURCES
  sim_bitmap.Load(IDB_LAUNCHER2_RGBA);
#else
  sim_bitmap.Load(IDB_LAUNCHER2);
#endif
  sim_button.Create(*this, rc, style,
                    std::make_unique<BitmapButtonRenderer>(sim_bitmap, true),
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
#ifndef NDEBUG
  /* Reserve extra space for debug warning banner */
  const int banner_extra_space = Layout::Scale(30);
  logo_rect.bottom = button_rc.top - label_height - Layout::Scale(5) -
    banner_extra_space;
#else
  logo_rect.bottom = button_rc.top - label_height - Layout::Scale(5);
#endif

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
  {
    const PixelRect rc = GetClientRect();
    const int height = rc.GetHeight();
    for (int y = rc.top; y < rc.bottom; y++) {
      const unsigned alpha = height > 0
        ? (unsigned)(y - rc.top) * 256 / height
        : 0;
      const Color c = Color(COLOR_XCSOAR.Red()
                              + (int(COLOR_XCSOAR_DARK.Red()) - int(COLOR_XCSOAR.Red()))
                                * (int)alpha / 256,
                            COLOR_XCSOAR.Green()
                              + (int(COLOR_XCSOAR_DARK.Green()) - int(COLOR_XCSOAR.Green()))
                                * (int)alpha / 256,
                            COLOR_XCSOAR.Blue()
                              + (int(COLOR_XCSOAR_DARK.Blue()) - int(COLOR_XCSOAR.Blue()))
                                * (int)alpha / 256);
      canvas.DrawFilledRectangle(PixelRect{PixelPoint{rc.left, y},
                                          PixelPoint{rc.right, y + 1}}, c);
    }
  }
  logo_view.draw(canvas, logo_rect, true);

  canvas.Select(look.text_font);
  canvas.SetTextColor(COLOR_WHITE);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(label_position, _("What do you want to do?"));

  ContainerWindow::OnPaint(canvas);
}

#endif /* SIMULATOR_AVAILABLE */
