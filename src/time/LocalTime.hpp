// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TimeStamp;
class RoughTimeDelta;

/**
 * Returns the local time express as seconds from midnight
 * @param d UTC time in seconds
 * @param utc_offset Offset in second
 */
[[gnu::pure]]
TimeStamp
TimeLocal(TimeStamp time, RoughTimeDelta utc_offset) noexcept;
