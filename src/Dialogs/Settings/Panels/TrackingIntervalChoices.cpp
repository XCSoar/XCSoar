// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrackingIntervalChoices.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"

#include <cstdlib>

static constexpr unsigned tracking_interval_seconds[] = {
  1, 2, 3, 5, 10, 15, 20, 30, 45,
  60, 120, 180, 300, 600, 900, 1200, 1800, 2400, 3000, 3600,
};

[[gnu::pure]]
static unsigned
FindClosestTrackingInterval(unsigned value) noexcept
{
  unsigned closest_value = tracking_interval_seconds[0];
  int closest_diff = -1;

  for (unsigned seconds : tracking_interval_seconds) {
    const int diff = std::abs(int(value) - int(seconds));
    if (closest_diff < 0 || diff < closest_diff) {
      closest_diff = diff;
      closest_value = seconds;
    }
  }

  return closest_value;
}

static void
FillTrackingIntervalChoices(DataFieldEnum &df, unsigned value) noexcept
{
  StaticString<16> buffer;
  for (unsigned seconds : tracking_interval_seconds) {
    if (seconds < 60)
      buffer.Format(_("%u sec"), seconds);
    else
      buffer.Format(_("%u min"), seconds / 60);
    df.AddChoice(seconds, buffer);
  }
  df.SetValue(FindClosestTrackingInterval(value));
}

void
AddTrackingIntervalRow(RowFormWidget &form, unsigned value) noexcept
{
  WndProperty *interval = form.AddEnum(_("Tracking Interval"), nullptr);
  FillTrackingIntervalChoices(*(DataFieldEnum *)interval->GetDataField(),
                              value);
  interval->RefreshDisplay();
}
