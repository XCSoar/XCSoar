// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Airspaces;
class BufferedReader;

/**
 * Throws on error.
 */
void
ParseAirspaceFile(Airspaces &airspaces,
                  BufferedReader &reader);
