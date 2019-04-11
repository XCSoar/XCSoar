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

#define ENABLE_MAIN_WINDOW
#define ENABLE_CLOSE_BUTTON

#include "Main.hpp"
#include "Event/LambdaTimer.hpp"
#include "Screen/Canvas.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Look/TaskLook.hpp"
#include "Look/FinalGlideBarLook.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "NMEA/Derived.hpp"
#include "Geo/SpeedVector.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"

#ifdef USE_GDI
#include "ResourceLoader.hpp"
#endif

class FinalGlideBarWindow : public PaintWindow
{
  FinalGlideBarRenderer renderer;
  GlideState state;
  GlidePolar glide_polar;
  DerivedInfo calculated;

  GlideSettings glide_settings;

public:
  FinalGlideBarWindow(const FinalGlideBarLook &look, const TaskLook &task_look)
    :renderer(look, task_look),
     state(GeoVector(100, Angle::Zero()),
           1000, 1000,
           SpeedVector(Angle::Zero(), 0))
  {
    glide_polar = GlidePolar(0);

    calculated.task_stats.total.solution_remaining =
      MacCready::Solve(glide_settings, glide_polar, state);

    calculated.task_stats.total.solution_mc0 =
      MacCready::Solve(glide_settings, glide_polar, state);

    calculated.task_stats.task_valid = true;
  }

  double GetAltitudeDifference() {
    return calculated.task_stats.total.solution_remaining.altitude_difference;
  }

  double GetAltitudeDifference0() {
    return calculated.task_stats.total.solution_mc0.altitude_difference;
  }

  void SetAltitudeDifference(double altitude_difference) {
    state.altitude_difference = altitude_difference;

    calculated.task_stats.total.solution_remaining =
      MacCready::Solve(glide_settings, glide_polar, state);
  }

  void SetAltitudeDifference0(double altitude_difference0) {
    state.altitude_difference = altitude_difference0;

    calculated.task_stats.total.solution_mc0 =
      MacCready::Solve(glide_settings, glide_polar, state);
  }

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();
    renderer.Draw(canvas, canvas.GetRect(), calculated, glide_settings, true);
  }
};

static void
Main()
{
  FinalGlideBarLook final_glide_look;
  final_glide_look.Initialise(normal_font);

  TaskLook task_look;
  task_look.Initialise();

  FinalGlideBarWindow final_glide(final_glide_look, task_look);
  WindowStyle with_border;
  with_border.Border();
  final_glide.Create(main_window, main_window.GetClientRect(), with_border);
  main_window.SetFullWindow(final_glide);

  double step(10);
  double mc_mc0_step(100);

  auto timer = MakeLambdaTimer([&](){
      double altitude_difference = final_glide.GetAltitudeDifference();
      double altitude_difference0 = final_glide.GetAltitudeDifference0();

      if (altitude_difference >= 600 ) {
        step = -10;
      } else if (altitude_difference <= -600) {
        step = 10;

        if (altitude_difference0 > 600) {
          mc_mc0_step = -100;
        } else if (altitude_difference0 <= altitude_difference) {
          mc_mc0_step = 100;
        }

        altitude_difference0 += mc_mc0_step;
      }

      altitude_difference += step;
      altitude_difference0 += step;

      final_glide.SetAltitudeDifference(altitude_difference);
      final_glide.SetAltitudeDifference0(altitude_difference0);

      final_glide.Invalidate();
    });
  timer.Schedule(std::chrono::milliseconds(100));

  main_window.RunEventLoop();

  timer.Cancel();
}
