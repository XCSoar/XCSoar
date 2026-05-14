// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Color.hpp"
#include "FLARM/Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Engine/Airspace/Ptr.hpp"

#include <variant>

class OrderedTask;

/** Live FLARM / traffic row focus — projection tracks #CommonInterface updates. */
struct MapPreviewFocusTraffic {
  FlarmId id;
  FlarmColor color;
};

/** Active-task observation zone row — index resolved via #ProtectedTaskManager at draw time. */
struct MapPreviewFocusTaskOZ {
  unsigned index;
};

/**
 * Map tap distance/direction or arrival-altitude row: fit aircraft and target,
 * draw straight leg (#TaskLook::bearing_pen) like the map goto helper line.
 */
struct MapPreviewFocusLocationLeg {
  GeoPoint target;
};

/** Draft task turnpoint row in #TaskEditPanel — #OrderedTask pointer stays valid for dialog lifetime. */
struct MapPreviewFocusTaskTurnpoint {
  OrderedTask *task;
  unsigned index;
};

/**
 * Fit entire task geometry — draft task in #TaskEditPanel / Manage actions,
 * or read-only file task while browsing #TaskListPanel.
 */
struct MapPreviewFocusTaskWhole {
  const OrderedTask *task;
};

/**
 * Remote-controlled centre / overlay policy for embedded map previews (UI thread).
 */
using MapPreviewFocus = std::variant<
  std::monostate,
  ConstAirspacePtr,
  WaypointPtr,
  MapPreviewFocusLocationLeg,
  GeoPoint,
  MapPreviewFocusTraffic,
  MapPreviewFocusTaskOZ,
  MapPreviewFocusTaskTurnpoint,
  MapPreviewFocusTaskWhole>;
