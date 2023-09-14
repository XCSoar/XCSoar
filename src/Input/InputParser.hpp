// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct InputConfig;
class TLineReader;

/**
 * Parses the contents of a *.xci file into the #InputConfig object.
 */
void
ParseInputFile(InputConfig &config, TLineReader &reader);
