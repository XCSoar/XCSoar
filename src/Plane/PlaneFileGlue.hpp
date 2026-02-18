// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
class AllocatedPath;
struct Plane;
class GlidePolar;
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

/**
 * Search .xcp files in the data directory for a plane matching
 * the given registration.
 *
 * @return path to the matching .xcp file, or nullptr if not found
 */
AllocatedPath
FindByRegistration(const char *registration);

/**
 * Create a new .xcp plane profile from the given polar and identity.
 *
 * @return path of the created file
 * Throws on I/O error.
 */
AllocatedPath
CreateFromPolar(const char *registration,
                const char *competition_id,
                const char *glider_type,
                const GlidePolar &polar);

} // namespace PlaneGlue
