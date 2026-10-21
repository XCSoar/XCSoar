// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Observation/Store.hpp"

struct DerivedInfo;
struct SpeedVector;
class ObservationDatabase;

/**
 * Replace own-ship rows from the calculated blackboard. An own-ship
 * thermal that is still near its previous ground point keeps its
 * received time.
 */
void
PublishOwnObservations(ObservationDatabase &db,
                       const DerivedInfo &calculated) noexcept;

/**
 * Rows the map may draw. Own-ship locations are shifted by the wind.
 * Cloud and ThermalInfoMap are omitted when their settings are off.
 */
void
CollectVisibleThermals(std::span<const ObservedThermal> in,
                       ThermalRows &out,
                       double aircraft_altitude,
                       const SpeedVector &wind,
                       bool show_cloud, bool show_tim) noexcept;
