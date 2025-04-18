// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
