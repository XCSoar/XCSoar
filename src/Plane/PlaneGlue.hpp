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

void
Synchronize(const Plane &plane, ComputerSettings &settings,
            GlidePolar &gp) noexcept;

} // namespace PlaneGlue
