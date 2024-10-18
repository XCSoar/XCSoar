// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct InfoBoxData;

void
UpdateInfoBoxHomeDistance(InfoBoxData &data) noexcept;

void
UpdateInfoBoxHomeAltitudeDiff(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTakeoffDistance(InfoBoxData &data) noexcept;

extern const struct InfoBoxPanel atc_infobox_panels[];

void
UpdateInfoBoxATCRadial(InfoBoxData &data) noexcept;
