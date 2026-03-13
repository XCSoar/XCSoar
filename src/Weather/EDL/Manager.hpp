// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <chrono>

namespace EDL {

void
ResetForDedicatedPage() noexcept;

void
EnsureInitialised() noexcept;

void
StepForecast(std::chrono::hours delta) noexcept;

void
SelectIsobar(unsigned isobar) noexcept;

bool
ShouldShowOnMainMap() noexcept;

void
SetShowOnMainMap(bool enabled) noexcept;

void
ClearOverlay() noexcept;

bool
OverlayEnabled() noexcept;

bool
OverlayVisible() noexcept;

void
SetLoadingStatus() noexcept;

void
SetIdleStatus() noexcept;

void
SetErrorStatus() noexcept;

void
ApplyOverlay(Path path) noexcept;

void
RefreshOverlayVisibility() noexcept;

const char *
GetStatusLabel() noexcept;

BrokenDateTime
GetForecastTime() noexcept;

BrokenDateTime
GetForecastTimeLocal() noexcept;

BrokenDateTime
ToLocalForecastTime(BrokenDateTime forecast) noexcept;

StaticString<64>
GetOverlayLabel() noexcept;

int
GetAltitude() noexcept;

unsigned
GetIsobar() noexcept;

int
GetAltitudeForIsobar(unsigned isobar) noexcept;

} // namespace EDL
