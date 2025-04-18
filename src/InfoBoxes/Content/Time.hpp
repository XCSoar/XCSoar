// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct InfoBoxData;

void
UpdateInfoBoxTimeLocal(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTimeUTC(InfoBoxData &data) noexcept;

void
UpdateInfoBoxTimeFlight(InfoBoxData &data) noexcept;
