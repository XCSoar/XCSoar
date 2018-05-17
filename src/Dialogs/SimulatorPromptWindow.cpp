/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "SimulatorPromptWindow.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Screen/Canvas.hpp"
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
  fly_bitmap.EnableInterpolation();
  fly_button.Create(*this, rc, style,
                    new BitmapButtonRenderer(fly_bitmap),
                    action_listener, FLY);

  sim_bitmap.Load(IDB_LAUNCHER2);
  sim_bitmap.EnableInterpolation();
  sim_button.Create(*this, rc, style,
                    new BitmapButtonRenderer(sim_bitmap),
                    action_listener, SIMULATOR);

  if (have_quit_button)
    quit_button.Create(*this, look.button, _("Quit"), rc, style,
                       action_listener, QUIT);
}

void
SimulatorPromptWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  const PixelRect rc = GetClientRect();

  const unsigned h_middle = new_size.cx / 2;
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
SimulatorPromptWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();
  logo_view.draw(canvas, logo_rect);

  canvas.Select(look.text_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText(label_position.x, label_position.y,
                  _("What do you want to do?"));

  ContainerWindow::OnPaint(canvas);
}

#endif /* SIMULATOR_AVAILABLE */
