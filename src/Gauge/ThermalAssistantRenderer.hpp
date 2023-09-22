// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/RadarRenderer.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/VarioInfo.hpp"
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
    [[gnu::pure]]
    PixelPoint GetAverage() const noexcept;
  };

protected:
  const ThermalAssistantLook &look;

  RadarRenderer radar_renderer;

  bool small;

  Angle direction;
  CirclingInfo circling;
  VarioInfo vario;

public:
  ThermalAssistantRenderer(const ThermalAssistantLook &look,
                           unsigned _padding, bool _small = false);

public:
  const PixelPoint &GetMiddle() const {
    return radar_renderer.GetCenter();
  }

  unsigned GetRadius() const{
    return radar_renderer.GetRadius();
  }

  void Update(const AttitudeState &attitude, const DerivedInfo &_derived);

  void UpdateLayout(const PixelRect &rc) noexcept {
    radar_renderer.UpdateLayout(rc);
  }

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
  static constexpr double NormalizeLift(double lift, double max_lift) noexcept;

  void CalculateLiftPoints(LiftPoints &lift_points, double max_lift) const;
  double CalculateMaxLift() const;
  void PaintRadarPlane(Canvas &canvas, double max_lift) const;
  void PaintRadarBackground(Canvas &canvas, double max_lift) const;
  void PaintPoints(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintAdvisor(Canvas &canvas, const LiftPoints &lift_points) const;
  void PaintNotCircling(Canvas &canvas) const;
};
