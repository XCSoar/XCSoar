// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
