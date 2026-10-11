// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SpokenAlert.hpp"
#include "Status.hpp"

/** PFLAU AlarmType of a collision alarm with an aircraft */
static constexpr uint8_t ALARM_TYPE_AIRCRAFT = 2;

/**
 * Above or below if the intruder is more than this far off the level,
 * relative to its distance (about 8.5 degrees, as in TopHat Soaring).
 */
static constexpr double VERTICAL_RATIO = 0.15;

static constexpr uint8_t
BearingToClock(int bearing) noexcept
{
  const int hour = ((bearing + 15) % 360 + 360) % 360 / 30;
  return hour == 0 ? 12 : hour;
}

static constexpr SpokenTrafficAlert::Vertical
GetVertical(const FlarmStatus &status) noexcept
{
  using Vertical = SpokenTrafficAlert::Vertical;

  if (!status.has_vertical || status.relative_distance == 0)
    return Vertical::UNKNOWN;

  const double ratio = double(status.relative_vertical) /
    status.relative_distance;
  if (ratio > VERTICAL_RATIO)
    return Vertical::ABOVE;
  if (ratio < -VERTICAL_RATIO)
    return Vertical::BELOW;
  return Vertical::LEVEL;
}

std::optional<SpokenTrafficAlert>
SpokenTrafficAlert::FromStatus(const FlarmStatus &status) noexcept
{
  if (!status.available || status.alarm_type != ALARM_TYPE_AIRCRAFT)
    return std::nullopt;

  switch (status.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    break;

  case FlarmTraffic::AlarmType::NONE:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    return std::nullopt;
  }

  return SpokenTrafficAlert{
    status.alarm_level,
    status.has_bearing ? BearingToClock(status.relative_bearing) : uint8_t(0),
    GetVertical(status),
  };
}

StaticArray<const char *, 3>
SpokenTrafficAlert::GetClips() const noexcept
{
  static constexpr const char *clock_clips[] = {
    "IDR_WAV_OCLOCK_1", "IDR_WAV_OCLOCK_2", "IDR_WAV_OCLOCK_3",
    "IDR_WAV_OCLOCK_4", "IDR_WAV_OCLOCK_5", "IDR_WAV_OCLOCK_6",
    "IDR_WAV_OCLOCK_7", "IDR_WAV_OCLOCK_8", "IDR_WAV_OCLOCK_9",
    "IDR_WAV_OCLOCK_10", "IDR_WAV_OCLOCK_11", "IDR_WAV_OCLOCK_12",
  };

  StaticArray<const char *, 3> clips;

  switch (level) {
  case FlarmTraffic::AlarmType::URGENT:
    clips.push_back("IDR_WAV_TRAFFIC_URGENT");
    break;

  case FlarmTraffic::AlarmType::IMPORTANT:
    clips.push_back("IDR_WAV_TRAFFIC_IMPORTANT");
    break;

  default:
    clips.push_back("IDR_WAV_TRAFFIC_LOW");
    break;
  }

  if (clock >= 1 && clock <= 12)
    clips.push_back(clock_clips[clock - 1]);

  switch (vertical) {
  case Vertical::UNKNOWN:
    break;

  case Vertical::LEVEL:
    clips.push_back("IDR_WAV_LEVEL");
    break;

  case Vertical::ABOVE:
    clips.push_back("IDR_WAV_ABOVE");
    break;

  case Vertical::BELOW:
    clips.push_back("IDR_WAV_BELOW");
    break;
  }

  return clips;
}
