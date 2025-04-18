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
  const std::unique_ptr<ObservationZonePoint> oz_point;

public:
  /**
   * Constructor.  Transfers ownership of the OZ to this object.
   *
   * @param _oz The OZ to store
   */
  template<typename T>
  explicit ObservationZoneClient(T &&_oz_point) noexcept
    :oz_point(std::forward<T>(_oz_point)) {}

  ~ObservationZoneClient() noexcept;

  /**
   * Accessor for OZ (for modifying parameters etc)
   *
   * @return Observation zone
   */
  ObservationZonePoint &GetObservationZone() noexcept {
    return *oz_point;
  }

  const ObservationZonePoint &GetObservationZone() const noexcept {
    return *oz_point;
  }

  bool IsInSector(const GeoPoint &location) const noexcept;

  [[gnu::pure]]
  bool CanStartThroughTop() const noexcept;

  [[gnu::pure]]
  bool TransitionConstraint(const GeoPoint &location,
                            const GeoPoint &last_location) const noexcept;

  [[gnu::pure]]
  OZBoundary GetBoundary() const noexcept;

  [[gnu::pure]]
  virtual double ScoreAdjustment() const noexcept;

  void SetLegs(const TaskPoint *previous, const TaskPoint *next) noexcept;

  [[gnu::pure]]
  GeoPoint GetRandomPointInSector(double mag) const noexcept;
};
