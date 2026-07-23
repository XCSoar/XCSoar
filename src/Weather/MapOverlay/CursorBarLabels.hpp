// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <cstddef>

namespace WeatherMapOverlay {

/**
 * Translatable suffix shown on cursor-bar labels when cached data is
 * missing, e.g. @c "14:00 UTC  [no data]".
 */
[[gnu::pure]]
const char *
NoDataTag() noexcept;

/**
 * Full cursor-bar label when stepping is impossible because nothing is
 * cached yet.
 */
[[gnu::pure]]
const char *
NoDataStepHint() noexcept;

/**
 * Full cursor-bar label when no forecast times exist for the selection.
 */
[[gnu::pure]]
const char *
NoForecastHint() noexcept;

/**
 * Append #NoDataTag() to @p label, storing the result in @p dest.
 */
template<size_t N>
void
AppendNoDataTag(StaticString<N> &dest, const char *label) noexcept
{
  dest.Format("%s  %s", label, NoDataTag());
}

/**
 * Copy @p base into @p dest, or append #NoDataTag() when @p has_data is false.
 */
template<size_t N>
void
AssignLabeledData(StaticString<N> &dest, const char *base,
                  bool has_data) noexcept
{
  if (base == nullptr || *base == '\0') {
    dest.clear();
    return;
  }

  if (has_data)
    dest = base;
  else
    AppendNoDataTag(dest, base);
}

template<size_t N>
void
AssignLabeledData(StaticString<N> &dest, const StaticString<N> &base,
                  bool has_data) noexcept
{
  AssignLabeledData(dest, base.c_str(), has_data);
}

/**
 * Format @p offset_min as @c "+H:MM" or @c "-H:MM" into @p dest.
 */
void
FormatSignedMinuteOffset(char *dest, size_t size, int offset_min) noexcept;

/**
 * Wrap @p offset_min to @c (-720, 720] for same-day display.
 */
[[gnu::const]]
int
NormaliseMinuteOffsetAroundNow(int offset_min) noexcept;

/**
 * Cursor-bar UTC hour label with optional AUTO prefix.
 */
void
FormatAutoUtcHourLabel(StaticString<64> &dest, bool auto_advance,
                       unsigned hour_utc,
                       const char *offset_buf) noexcept;

/**
 * Named secondary-axis label with optional AUTO prefix, e.g.
 * @c "AUTO: 2000m" or @c "2000m".
 */
template<size_t N>
void
FormatAutoNamedLabel(StaticString<N> &dest, bool auto_advance,
                     const char *name) noexcept
{
  if (name == nullptr || *name == '\0') {
    dest.clear();
    return;
  }

  if (auto_advance)
    dest.Format("%s %s", _("AUTO:"), name);
  else
    dest = name;
}

/**
 * Cursor-bar local @c HH:MM label with optional AUTO prefix.
 *
 * When @p auto_advance is true and @p has_time is false, shows @c "AUTO:"
 * only.
 */
void
FormatAutoLocalTimeLabel(StaticString<64> &dest, bool auto_advance,
                         bool has_time, unsigned hour, unsigned minute,
                         const char *offset_buf) noexcept;

void
ShowAutoTimeStatusMessage(bool enabled) noexcept;

void
ShowAutoAltitudeStatusMessage(bool enabled) noexcept;

} // namespace WeatherMapOverlay
