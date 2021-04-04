/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef OBSERVATIONZONECLIENT_HPP
#define OBSERVATIONZONECLIENT_HPP

#include <memory>

class ObservationZonePoint;
class OZBoundary;
class TaskPoint;
struct GeoPoint;

/**
 * Class holding an ObzervationZonePoint, directing calls to it
 */
class ObservationZoneClient {
  std::unique_ptr<ObservationZonePoint> oz_point;

public:
  /**
   * Constructor.  Transfers ownership of the OZ to this object.
   *
   * @param _oz The OZ to store
   */
  explicit ObservationZoneClient(std::unique_ptr<ObservationZonePoint> _oz_point) noexcept;
  ~ObservationZoneClient() noexcept;

  /**
   * Accessor for OZ (for modifying parameters etc)
   *
   * @return Observation zone
   */
  ObservationZonePoint &GetObservationZone() {
    return *oz_point;
  }

  const ObservationZonePoint &GetObservationZone() const {
    return *oz_point;
  }

  bool IsInSector(const GeoPoint &location) const;

  [[gnu::pure]]
  bool CanStartThroughTop() const;

  [[gnu::pure]]
  bool TransitionConstraint(const GeoPoint &location,
                            const GeoPoint &last_location) const;

  [[gnu::pure]]
  OZBoundary GetBoundary() const;

  virtual double ScoreAdjustment() const;

  void SetLegs(const TaskPoint *previous, const TaskPoint *next);

  [[gnu::pure]]
  GeoPoint GetRandomPointInSector(double mag) const;
};


#endif
