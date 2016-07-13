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

#ifndef FLARM_TRAFFIC_WINDOW_H
#define FLARM_TRAFFIC_WINDOW_H

#include "Screen/PaintWindow.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Color.hpp"
#include "TeamCode/Settings.hpp"
#include "Math/FastRotation.hpp"

class Color;
class Brush;
struct FlarmTrafficLook;

/**
 * A Window which renders FLARM traffic.
 */
class FlarmTrafficWindow : public PaintWindow {
protected:
  const FlarmTrafficLook &look;

  /**
   * The distance of the biggest circle in meters.
   */
  double distance;

  int selection;
  int warning;
  PixelPoint radar_mid;

  /**
   * The minimum distance between the window boundary and the biggest
   * circle in pixels.
   */
  const unsigned h_padding, v_padding;

  /**
   * The radius of the biggest circle in pixels.
   */
  unsigned radius;

  bool small;

  PixelPoint sc[TrafficList::MAX_COUNT];

  bool enable_north_up;
  Angle heading;
  FastRotation fr;
  FastIntegerRotation fir;
  TrafficList data;
  Validity data_modified;
  TeamCodeSettings settings;

public:
  enum SideInfoType {
    SIDE_INFO_RELATIVE_ALTITUDE,
    SIDE_INFO_VARIO,
  } side_display_type;

public:
  FlarmTrafficWindow(const FlarmTrafficLook &_look,
                     unsigned _h_padding, unsigned _v_padding,
                     bool _small = false);

public:
  bool WarningMode() const;

  const FlarmTraffic *GetTarget() const {
    return selection >= 0
      ? &data.list[selection]
      : NULL;
  }

  void SetTarget(int i);

  void SetTarget(const FlarmTraffic *traffic) {
    SetTarget(traffic != NULL ? (int)data.TrafficIndex(traffic) : -1);
  }

  void SetTarget(const FlarmId &id) {
    SetTarget(data.FindTraffic(id));
  }

  void NextTarget();
  void PrevTarget();
  bool SelectNearTarget(PixelPoint p, int max_distance);

  void SetDistance(double _distance) {
    distance = _distance;
    Invalidate();
  }

  void Paint(Canvas &canvas);

protected:
  double RangeScale(double d) const;

  void UpdateSelector(FlarmId id, PixelPoint pt);
  void UpdateWarnings();
  void Update(Angle new_direction, const TrafficList &new_data,
              const TeamCodeSettings &new_settings);
  void PaintRadarNoTraffic(Canvas &canvas) const;
  void PaintRadarTarget(Canvas &canvas, const FlarmTraffic &traffic,
                        unsigned i);
  void PaintRadarTraffic(Canvas &canvas);

  void PaintTargetInfoSmall(
      Canvas &canvas, const FlarmTraffic &traffic, unsigned i,
      const Color &text_color, const Brush &arrow_brush);

  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas) const;
  void PaintNorth(Canvas &canvas) const;

protected:
  /* virtual methods from class Window */
  virtual void OnResize(PixelSize new_size) override;

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
