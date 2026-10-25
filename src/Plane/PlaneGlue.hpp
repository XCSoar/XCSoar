// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct Plane;
struct ComputerSettings;
class GlidePolar;
class ProfileMap;

namespace PlaneGlue {

void
FromProfile(Plane &plane, const ProfileMap &profile) noexcept;

/**
 * Apply the plane's polar, masses and speeds to the glide polar.
 *
 * @return false if the plane has a polar but it is invalid (e.g. a
 * sink rate with the wrong sign); the glide polar is then invalid too,
 * rather than still the previous plane's
 */
bool
Synchronize(const Plane &plane, ComputerSettings &settings,
            GlidePolar &gp) noexcept;

} // namespace PlaneGlue
