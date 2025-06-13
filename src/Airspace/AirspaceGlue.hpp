// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class RasterTerrain;
class AtmosphericPressure;
class Airspaces;
class OperationEnvironment;
class Path;

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

/**
 * Reads the airspace files from path.
 */
bool ParseAirspaceFile(Airspaces &airspaces, Path path,
                       OperationEnvironment &operation) noexcept;
