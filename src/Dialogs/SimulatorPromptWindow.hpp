/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Simulator.hpp"

#ifdef SIMULATOR_AVAILABLE

#include "ui/window/ContainerWindow.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Gauge/LogoView.hpp"
#include "Form/Button.hpp"

#include <functional>

struct DialogLook;

class SimulatorPromptWindow final : public ContainerWindow {
public:
  enum class Result {
    FLY = 1000,
    SIMULATOR,
    QUIT,
  };

private:
  const DialogLook &look;
  const std::function<void(Result)> callback;
  const bool have_quit_button;

  LogoView logo_view;
  PixelRect logo_rect;

  Button quit_button;

  Bitmap fly_bitmap, sim_bitmap;
  Button fly_button, sim_button;

  PixelPoint label_position;

public:
  SimulatorPromptWindow(const DialogLook &_look,
                        std::function<void(Result)> _callback,
                        bool _quit)
    :look(_look), callback(std::move(_callback)),
     have_quit_button(_quit) {}

protected:
  /* virtual methods from class Window */
  void OnCreate() override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};

#endif /* SIMULATOR_AVAILABLE */
