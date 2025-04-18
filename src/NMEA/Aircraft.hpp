// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct MoreData;
struct DerivedInfo;
struct AircraftState;

[[gnu::pure]]
const AircraftState
ToAircraftState(const MoreData &info, const DerivedInfo &calculated);
