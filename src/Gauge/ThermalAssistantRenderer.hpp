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

#ifndef THERMAL_ASSISTENT_RENDERER_HPP
#define THERMAL_ASSISTENT_RENDERER_HPP

#include "NMEA/CirclingInfo.hpp"
#include "NMEA/VarioInfo.hpp"
#include "Screen/Point.hpp"
#include "Screen/BulkPoint.hpp"

#include <array>

struct ThermalAssistantLook;
struct AttitudeState;
struct DerivedInfo;
class Canvas;

class ThermalAssistantRenderer
{
  class LiftPoints: public std::array<BulkPixelPoint,
                                      std::tuple_size<LiftDatabase>::value>
  {
  public:
    PixelPoint GetAverage() const;
  };

protected:
  const ThermalAssistantLook &look;

  PixelPoint mid;

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
  ThermalAssistantRenderer(const ThermalAssistantLook &look,
                           unsigned _padding, bool _small = false);

public:
  const PixelPoint &GetMiddle() const {
    return mid;
  }

  unsigned GetRadius() const{
    return radius;
  }

  void Update(const AttitudeState &attitude, const DerivedInfo &_derived);

  void UpdateLayout(const PixelRect &rc);
  void Paint(Canvas &canvas);

  const ThermalAssistantLook &GetLook() {
    return look;
  }

protected:
  /**
   * Normalize the lift to the range of 0.0 to 1.0
   * 0.0: lift = -max_lift
   * 0.5: lift = zero lift
   * 1.0: lift = max_lift
   */
  static double NormalizeLift(double lift, double max_lift);

  void CalculateLiftPoints(LiftPoints &lift_points, double max_lift) const;
  double CalculateMaxLift() const;
  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas, double max_lift) const;
  void PaintPoints(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintAdvisor(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintNotCircling(Canvas &canvas) const;
};

#endif
