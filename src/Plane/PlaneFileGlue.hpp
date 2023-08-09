// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
struct Plane;
class KeyValueFileReader;
class KeyValueFileWriter;

namespace PlaneGlue {

/**
 * Throws on I/O error.
 */
bool
Read(Plane &plane, KeyValueFileReader &reader);

bool
ReadFile(Plane &plane, Path path) noexcept;

/**
 * Throws on I/O error.
 */
void
Write(const Plane &plane, KeyValueFileWriter &writer);

/**
 * Throws on I/O error.
 */
void
WriteFile(const Plane &plane, Path path);

} // namespace PlaneGlue
