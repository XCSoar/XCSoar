// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class AirspaceActivity {
  struct Days
  {
    bool sunday:1;
    bool monday:1;
    bool tuesday:1;
    bool wednesday:1;
    bool thursday:1;
    bool friday:1;
    bool saturday:1;
  };

  union
  {
    Days days;
    unsigned char value;
  } mask;

public:
  constexpr AirspaceActivity() noexcept {
    SetAll();
  };

  constexpr bool equals(const AirspaceActivity _mask) const noexcept {
    return mask.value == _mask.mask.value;
  }

  constexpr AirspaceActivity(int8_t day_of_week) noexcept {
    // setter from BrokenDate format day
    mask.value = 0;
    switch(day_of_week) {
    case 0:
      mask.days.sunday = true;
      break;
    case 1:
      mask.days.monday = true;
      break;
    case 2:
      mask.days.tuesday = true;
      break;
    case 3:
      mask.days.wednesday = true;
      break;
    case 4:
      mask.days.thursday = true;
      break;
    case 5:
      mask.days.friday = true;
      break;
    case 6:
      mask.days.saturday = true;
      break;
    default:
      SetAll();
      break;
    }
  }

  constexpr void SetAll() noexcept {
    mask.value = 0xFF;
  }

  constexpr void SetWeekdays() noexcept {
    mask.value = 0;
    mask.days.monday = true;
    mask.days.tuesday = true;
    mask.days.wednesday = true;
    mask.days.thursday = true;
    mask.days.friday = true;
  }

  constexpr void SetWeekend() noexcept {
    mask.value = 0;
    mask.days.saturday = true;
    mask.days.sunday = true;
  }

  constexpr bool Matches(AirspaceActivity _mask) const noexcept {
    return mask.value & _mask.mask.value;
  }
};

static_assert(sizeof(AirspaceActivity) == 1, "Wrong size");
