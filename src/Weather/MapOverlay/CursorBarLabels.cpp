// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CursorBarLabels.hpp"

#include "Language/Language.hpp"

#include <cstdio>

namespace WeatherMapOverlay {

const char *
NoDataTag() noexcept
{
  return _("[no data]");
}

const char *
NoDataStepHint() noexcept
{
  return _("No data – use Info → Weather");
}

const char *
NoForecastHint() noexcept
{
  return _("No forecast – download first");
}

void
FormatSignedMinuteOffset(char *dest, size_t size, int offset_min) noexcept
{
  const int abs_min = std::abs(offset_min);
  std::snprintf(dest, size, "%s%d:%02d",
                offset_min >= 0 ? "+" : "-", abs_min / 60, abs_min % 60);
}

int
NormaliseMinuteOffsetAroundNow(int offset_min) noexcept
{
  if (offset_min < -720)
    offset_min += 1440;
  else if (offset_min > 720)
    offset_min -= 1440;

  return offset_min;
}

void
FormatAutoUtcHourLabel(StaticString<64> &dest, bool auto_advance,
                       unsigned hour_utc, const char *offset_buf) noexcept
{
  if (auto_advance)
    dest.Format("%s %02u:00 UTC (%s)", _("AUTO:"), hour_utc, offset_buf);
  else
    dest.Format("%02u:00 UTC (%s)", hour_utc, offset_buf);
}

void
FormatAutoLocalTimeLabel(StaticString<64> &dest, bool auto_advance,
                         bool has_time, unsigned hour, unsigned minute,
                         const char *offset_buf) noexcept
{
  if (auto_advance) {
    if (has_time)
      dest.Format("%s %02u:%02u (%s)", _("AUTO:"), hour, minute, offset_buf);
    else
      dest = _("AUTO:");
  } else if (has_time) {
    dest.Format("%02u:%02u (%s)", hour, minute, offset_buf);
  } else {
    dest = _("AUTO:");
  }
}

} // namespace WeatherMapOverlay
