// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PolarInfo;
class Path;
class NLineReader;
class BufferedOutputStream;

namespace PolarGlue {

/**
 * Throws on I/O error.
 */
void
LoadFromFile(PolarInfo &polar, Path path);

/**
 * Throws on I/O error.
 */
void
SaveToFile(const PolarInfo &polar, Path path);

/**
 * Throws on I/O error.
 */
bool
LoadFromFile(PolarInfo &polar, NLineReader &reader);

/**
 * Throws on I/O error.
 */
void
SaveToFile(const PolarInfo &polar, BufferedOutputStream &writer);

} // namespace PolarGlue
