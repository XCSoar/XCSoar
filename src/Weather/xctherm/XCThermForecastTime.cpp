// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermForecastTime.hpp"
#include "XCThermAPI.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "time/BrokenDateTime.hpp"

#include <chrono>
#include <cstdlib>

namespace XCTherm {

UtcTimeParts
GetUtcTimeParts() noexcept
{
  UtcTimeParts utc;
  const auto &basic = CommonInterface::Basic();
  if (basic.date_time_utc.IsPlausible()) {
    utc.hour = basic.date_time_utc.hour;
    utc.minute = basic.date_time_utc.minute;
  }
  return utc;
}

int
PickAutoTimeIndex(const std::vector<unsigned> &cached_hours,
                  unsigned utc_h, unsigned utc_min) noexcept
{
  if (cached_hours.empty())
    return -1;

  const unsigned target = (utc_min >= 45) ? (utc_h + 1) % 24 : utc_h;

  int best_future = -1;
  unsigned best_future_dist = 25;
  int best_past = -1;
  unsigned best_past_dist = 25;

  for (size_t i = 0; i < cached_hours.size(); ++i) {
    const unsigned fwd = (cached_hours[i] + 24 - target) % 24;
    if (fwd <= 12) {
      if (fwd < best_future_dist) {
        best_future_dist = fwd;
        best_future = int(i);
      }
    } else {
      const unsigned back = 24 - fwd;
      if (back < best_past_dist) {
        best_past_dist = back;
        best_past = int(i);
      }
    }
  }

  return best_future >= 0 ? best_future : best_past;
}

unsigned
ForecastHourAt(const std::vector<unsigned> &cached_hours,
             unsigned time_index,
             const std::vector<unsigned> &available_hours,
             unsigned fallback) noexcept
{
  if (time_index < cached_hours.size())
    return cached_hours[time_index];
  if (!available_hours.empty())
    return available_hours[0];
  return fallback;
}

void
FormatTimeLabel(StaticString<64> &dest, const char *api_parameter,
                unsigned forecast_utc_hour, bool time_auto_active) noexcept
{
  int offset_min = 0;
  bool has_real_offset = false;
  const auto slice =
    XCThermAPI::Instance().GetSliceMeta(api_parameter, forecast_utc_hour);

  if (slice.has_value() && slice->run_date.size() == 8 &&
      slice->run_hour.size() == 2 &&
      BrokenDateTime::NowUTC().IsPlausible()) {
    const unsigned year =
      (unsigned)std::atoi(slice->run_date.substr(0, 4).c_str());
    const unsigned month =
      (unsigned)std::atoi(slice->run_date.substr(4, 2).c_str());
    const unsigned day =
      (unsigned)std::atoi(slice->run_date.substr(6, 2).c_str());
    const unsigned run_h = (unsigned)std::atoi(slice->run_hour.c_str());
    const BrokenDateTime run_dt(year, month, day, run_h, 0, 0);
    const BrokenDateTime forecast_dt =
      run_dt + std::chrono::hours{slice->step};
    const auto delta = forecast_dt - BrokenDateTime::NowUTC();
    offset_min = (int)std::chrono::duration_cast<std::chrono::minutes>(
      delta).count();
    has_real_offset = true;
  } else if (CommonInterface::Basic().date_time_utc.IsPlausible()) {
    const auto &basic = CommonInterface::Basic();
    const int cur_min = int(basic.date_time_utc.hour) * 60
                      + int(basic.date_time_utc.minute);
    const int fc_min = int(forecast_utc_hour) * 60;
    offset_min = fc_min - cur_min;
    if (offset_min < 0)
      offset_min += 1440;
    has_real_offset = true;
  }

  char offset_buf[16] = {};
  if (has_real_offset) {
    const int abs_min = std::abs(offset_min);
    std::snprintf(offset_buf, sizeof(offset_buf), "%s%d:%02d",
                  offset_min >= 0 ? "+" : "-", abs_min / 60, abs_min % 60);
  }

  if (time_auto_active)
    dest.Format("%s %02u:00 UTC (%s)", _("AUTO:"), forecast_utc_hour,
                offset_buf);
  else
    dest.Format("%02u:00 UTC (%s)", forecast_utc_hour, offset_buf);
}

} // namespace XCTherm
