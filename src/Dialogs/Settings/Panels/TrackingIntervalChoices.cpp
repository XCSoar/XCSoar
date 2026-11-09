// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingIntervalChoices.hpp"
#include "Dialogs/GroupedListPicker.hpp"
#include "Language/Language.hpp"

#include <array>
#include <cstdlib>

void
FormatTrackingInterval(StaticString<16> &buffer,
                       unsigned seconds) noexcept
{
  if (seconds < 60)
    buffer.Format(_("%u sec"), seconds);
  else
    buffer.Format(_("%u min"), seconds / 60);
}

unsigned
ClosestTrackingInterval(unsigned value) noexcept
{
  unsigned closest_value = TRACKING_INTERVAL_SECONDS[0];
  int closest_diff = -1;

  for (unsigned seconds : TRACKING_INTERVAL_SECONDS) {
    const int diff = std::abs(int(value) - int(seconds));
    if (closest_diff < 0 || diff < closest_diff) {
      closest_diff = diff;
      closest_value = seconds;
    }
  }

  return closest_value;
}

bool
PickTrackingInterval(unsigned &interval) noexcept
{
  std::array<StaticString<16>, TRACKING_INTERVAL_COUNT> captions;
  std::array<PickerChoice, TRACKING_INTERVAL_COUNT> choices;

  int current = 0;
  for (unsigned i = 0; i < TRACKING_INTERVAL_COUNT; ++i) {
    const unsigned seconds = TRACKING_INTERVAL_SECONDS[i];
    FormatTrackingInterval(captions[i], seconds);
    choices[i] = {captions[i].c_str()};
    if (seconds == interval)
      current = (int)i;
  }

  const int picked = PickChoice(_("Tracking Interval"), nullptr,
                                choices, current);
  if (picked < 0 ||
      TRACKING_INTERVAL_SECONDS[picked] == interval)
    return false;

  interval = TRACKING_INTERVAL_SECONDS[picked];
  return true;
}
