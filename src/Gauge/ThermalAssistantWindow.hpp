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

#ifndef THERMAL_ASSISTENT_WINDOW_HPP
#define THERMAL_ASSISTENT_WINDOW_HPP

#include "Screen/AntiFlickerWindow.hpp"
#include "ThermalAssistantRenderer.hpp"

struct ThermalAssistantLook;

class ThermalAssistantWindow : public AntiFlickerWindow
{
  ThermalAssistantRenderer renderer;

#ifdef ENABLE_OPENGL
  const bool transparent;
#endif

public:
  /**
   * @param transparent draw in a circular area only, the rest of the
   * window is transparent (OpenGL only)
   */
  ThermalAssistantWindow(const ThermalAssistantLook &look,
                         unsigned _padding, bool _small = false,
                         bool transparent=false);

  void Update(const AttitudeState &attitude, const DerivedInfo &_derived);

protected:
  void DrawCircle(Canvas &canvas);

  virtual void OnResize(PixelSize new_size) override;
  virtual void OnPaintBuffer(Canvas &canvas) override;
};

#endif
