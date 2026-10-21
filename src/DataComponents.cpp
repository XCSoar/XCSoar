// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataComponents.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Observation/Database.hpp"
#include "Topography/TopographyStore.hpp"
#include "Terrain/RasterTerrain.hpp"

DataComponents::DataComponents() noexcept
  :airspaces(new Airspaces()),
   waypoints(new Waypoints()),
   observations(new ObservationDatabase())
{
}

DataComponents::~DataComponents() noexcept = default;
