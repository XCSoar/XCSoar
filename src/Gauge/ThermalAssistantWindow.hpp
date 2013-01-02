/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/BufferWindow.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/VarioInfo.hpp"

#include <array>

struct ThermalAssistantLook;
struct DerivedInfo;

class ThermalAssistantWindow : public BufferWindow
{
  class LiftPoints: public std::array<RasterPoint,
                                      std::tuple_size<VarioInfo::LiftDatabase>::value>
  {
  public:
    RasterPoint GetAverage() const;
  };

protected:
  const ThermalAssistantLook &look;

  RasterPoint mid;

  /**
   * The minimum distance between the window boundary and the biggest
   * circle in pixels.
   */
  unsigned padding;

  /**
   * The radius of the biggest circle in pixels.
   */
  unsigned radius;

  bool small;

  Angle direction;
  CirclingInfo circling;
  VarioInfo vario;

public:
  ThermalAssistantWindow(const ThermalAssistantLook &look,
                         unsigned _padding, bool _small = false);

public:
  bool LeftTurn() const;

  void Update(const DerivedInfo &_derived);

protected:
  /**
   * Normalize the lift to the range of 0.0 to 1.0
   * 0.0: lift = -max_lift
   * 0.5: lift = zero lift
   * 1.0: lift = max_lift
   */
  static fixed NormalizeLift(fixed lift, fixed max_lift);

  void CalculateLiftPoints(LiftPoints &lift_points, fixed max_lift) const;
  fixed CalculateMaxLift() const;
  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas, fixed max_lift) const;
  void PaintPoints(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintAdvisor(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintNotCircling(Canvas &canvas) const;

protected:
  virtual void OnResize(UPixelScalar width, UPixelScalar height);
  virtual void OnPaintBuffer(Canvas &canvas);
};

#endif
