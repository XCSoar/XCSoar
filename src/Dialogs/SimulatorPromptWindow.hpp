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

#ifndef XCSOAR_SIMULATOR_PROMPT_WINDOW_HPP
#define XCSOAR_SIMULATOR_PROMPT_WINDOW_HPP

#include "Simulator.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "Screen/ContainerWindow.hpp"
#include "Screen/Bitmap.hpp"
#include "Gauge/LogoView.hpp"
#include "Form/Button.hpp"

struct DialogLook;
class ActionListener;

class SimulatorPromptWindow final : public ContainerWindow {
  const DialogLook &look;
  ActionListener &action_listener;
  const bool have_quit_button;

  LogoView logo_view;
  PixelRect logo_rect;

  Button quit_button;

  Bitmap fly_bitmap, sim_bitmap;
  Button fly_button, sim_button;

  PixelPoint label_position;

public:
  enum Buttons {
    FLY = 1000,
    SIMULATOR,
    QUIT,
  };

  SimulatorPromptWindow(const DialogLook &_look,
                        ActionListener &_action_listener,
                        bool _quit)
    :look(_look), action_listener(_action_listener),
     have_quit_button(_quit) {}

protected:
  /* virtual methods from class Window */
  virtual void OnCreate() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnPaint(Canvas &canvas) override;
};

#endif /* SIMULATOR_AVAILABLE */

#endif
