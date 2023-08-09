// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PolarInfo;

namespace PolarGlue {

PolarInfo
GetDefault() noexcept;

bool
LoadFromProfile(PolarInfo &polar) noexcept;

PolarInfo
LoadFromProfile() noexcept;

} // namespace PolarGlue
