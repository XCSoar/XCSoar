// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Airspaces;
class TLineReader;
class ProgressListener;

/**
 * Throws on error.
 */
void
ParseAirspaceFile(Airspaces &airspaces,
                  TLineReader &reader,
                  ProgressListener &progress);
