// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class RasterTerrain;
class AtmosphericPressure;
class Airspaces;
class OperationEnvironment;

/**
 * Reads the airspace files into the memory
 */
void
ReadAirspace(Airspaces &airspaces,
             AtmosphericPressure press,
             OperationEnvironment &operation);

void
SetAirspaceGroundLevels(Airspaces &airspaces,
                        const RasterTerrain &terrain) noexcept;
