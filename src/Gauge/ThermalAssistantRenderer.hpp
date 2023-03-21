// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/CirclingInfo.hpp"
#include "NMEA/VarioInfo.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/BulkPoint.hpp"

#include <array>

struct ThermalAssistantLook;
struct AttitudeState;
struct DerivedInfo;
struct PixelRect;
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
