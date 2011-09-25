/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "FLARM/State.hpp"
#include "SettingsComputer.hpp"
#include "Math/FastRotation.hpp"
#include "FlarmTrafficLook.hpp"

/**
 * A Window which renders FLARM traffic.
 */
class FlarmTrafficWindow : public PaintWindow {
protected:
  const FlarmTrafficLook &look;

  /**
   * The distance of the biggest circle in meters.
   */
  fixed distance;

  int selection;
  int warning;
  RasterPoint radar_mid;

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

  RasterPoint sc[FlarmState::FLARM_MAX_TRAFFIC];

  bool enable_north_up;
  Angle heading;
  FastRotation fr;
  FastIntegerRotation fir;
  FlarmState data;
  SETTINGS_TEAMCODE settings;

public:
  int side_display_type;

public:
  FlarmTrafficWindow(const FlarmTrafficLook &_look,
                     unsigned _padding, bool _small = false);

public:
  bool WarningMode() const;

  const FlarmTraffic *GetTarget() const {
    return selection >= 0
      ? &data.traffic[selection]
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
  bool SelectNearTarget(int x, int y, int max_distance);

  void SetDistance(fixed _distance) {
    distance = _distance;
    invalidate();
  }

  void Paint(Canvas &canvas);

protected:
  fixed RangeScale(fixed d) const;

  void UpdateSelector(const FlarmId id, const RasterPoint pt);
  void UpdateWarnings();
  void Update(Angle new_direction, const FlarmState &new_data,
              const SETTINGS_TEAMCODE &new_settings);
  void PaintRadarNoTraffic(Canvas &canvas) const;
  void PaintRadarTarget(Canvas &canvas, const FlarmTraffic &traffic,
                        unsigned i);
  void PaintRadarTraffic(Canvas &canvas);
  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas) const;
  void PaintNorth(Canvas &canvas) const;

protected:
  virtual bool on_resize(UPixelScalar width, UPixelScalar height);
  virtual void on_paint(Canvas &canvas);
};

#endif
