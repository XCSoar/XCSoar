// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Origin.hpp"

class Airspaces;
class BufferedReader;

/**
 * Throws on error.
 *
 * @param origin Where the airspaces in this file were loaded from
 */
void
ParseAirspaceFile(Airspaces &airspaces,
                  BufferedReader &reader,
                  AirspaceOrigin origin = AirspaceOrigin::FILE);
