// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SimulatorPromptWindow.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "Look/DialogLook.hpp"
#include "Look/Colors.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Gauge/LogoView.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/BitmapButtonRenderer.hpp"
#include "Renderer/GradientRenderer.hpp"
#include "Simulator.hpp"
#include "Resources.hpp"

/**
 * This window's position in the main window coordinate system.
 */
[[gnu::pure]]
static PixelRect
GetWindowRectInRoot(const Window &window) noexcept
{
  PixelRect rc = window.GetPosition();
  for (const Window *p = window.GetParent();
       p != nullptr && p->GetParent() != nullptr;
       p = p->GetParent())
    rc.Offset(p->GetTopLeft().x, p->GetTopLeft().y);
  return rc;
}

/**
 * The main window's safe area, in this window's client coordinates.
 * The gradient may fill the whole window; Quit, Fly, Simulator and
 * the version string stay in this rectangle.
 */
[[gnu::pure]]
static PixelRect
GetLocalSafeRect(const Window &window) noexcept
{
  const PixelRect local = window.GetClientRect();
  const PixelRect safe = UIGlobals::GetMainWindow().GetSafeAreaRect();
  const PixelRect in_root = GetWindowRectInRoot(window);
  PixelRect local_safe{
    safe.left - in_root.left,
    safe.top - in_root.top,
    safe.right - in_root.left,
    safe.bottom - in_root.top,
  };
  local_safe = local_safe.Intersection(local);
  if (local_safe.IsEmpty())
    return local;
  return local_safe;
}

void
SimulatorPromptWindow::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();

  WindowStyle style;
  style.TabStop();

  fly_bitmap.Load(IDB_LAUNCHER1_RGBA);
  fly_button.Create(*this, rc, style,
                    std::make_unique<BitmapButtonRenderer>(fly_bitmap, true),
                    [this](){ callback(Result::FLY); });

  sim_bitmap.Load(IDB_LAUNCHER2_RGBA);
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

  /* the window size may stay the same when only the insets change */
  layout_rc = {};
  LayoutControls();
}

void
SimulatorPromptWindow::LayoutControls() noexcept
{
  const PixelRect rc = GetLocalSafeRect(*this);
  if (rc.left == layout_rc.left && rc.top == layout_rc.top &&
      rc.right == layout_rc.right && rc.bottom == layout_rc.bottom)
    return;
  layout_rc = rc;

  const unsigned h_middle = unsigned(rc.left + rc.GetWidth() / 2);
  const unsigned bottom_padding = Layout::Scale(15);
  const unsigned button_width = Layout::Scale(112);
  const unsigned button_height = Layout::Scale(30);
  const unsigned label_height =
    look.text_font.GetHeight() + Layout::GetTextPadding();

  PixelRect button_rc;
  button_rc.left = int(h_middle) - int(button_width);
  button_rc.right = int(h_middle);
  button_rc.bottom = rc.bottom - int(bottom_padding);
  button_rc.top = button_rc.bottom - int(button_height);
  fly_button.Move(button_rc);

  label_position.x = button_rc.left;
  label_position.y = button_rc.top - int(label_height);

  button_rc.left = button_rc.right;
  button_rc.right = int(h_middle) + int(button_width);
  sim_button.Move(button_rc);

  logo_rect = rc;
#ifndef NDEBUG
  /* Reserve extra space for debug warning banner */
  const int banner_extra_space = Layout::Scale(30);
  logo_rect.bottom = button_rc.top - int(label_height) - Layout::Scale(5) -
    banner_extra_space;
#else
  logo_rect.bottom = button_rc.top - int(label_height) - Layout::Scale(5);
#endif

  if (have_quit_button) {
    button_rc = rc;
    button_rc.left = button_rc.right - Layout::Scale(75);
    button_rc.bottom = button_rc.top + int(Layout::GetMaximumControlHeight());
    quit_button.Move(button_rc);
  }
}

void
SimulatorPromptWindow::OnPaint(Canvas &canvas) noexcept
{
  /* insets can arrive after the first OnResize */
  LayoutControls();

#ifdef ENABLE_OPENGL
  DrawVerticalGradient(canvas, GetClientRect(),
                       COLOR_XCSOAR, COLOR_XCSOAR_DARK,
                       COLOR_XCSOAR_DARK);
  logo_view.draw(canvas, logo_rect, true);

  canvas.Select(look.text_font);
  canvas.SetTextColor(COLOR_WHITE);
#else
  /* Without OpenGL there is no alpha blending.  The software
     renderer uses pre-composited PNGs with opaque white
     backgrounds.  A dark/gradient background would show visible
     white rectangles around every bitmap.  Use a plain white
     background instead. */
  canvas.ClearWhite();
  logo_view.draw(canvas, logo_rect);

  canvas.Select(look.text_font);
  canvas.SetTextColor(COLOR_BLACK);
#endif
  canvas.SetBackgroundTransparent();
  canvas.DrawText(label_position, _("What do you want to do?"));

  ContainerWindow::OnPaint(canvas);
}

#endif /* SIMULATOR_AVAILABLE */
