// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SpokenVerticalSpeed.hpp"
#include "Units/Unit.hpp"

#include <algorithm>
#include <cmath>

static constexpr const char *number_clips[] = {
  "IDR_WAV_NUMBER_0", "IDR_WAV_NUMBER_1", "IDR_WAV_NUMBER_2",
  "IDR_WAV_NUMBER_3", "IDR_WAV_NUMBER_4", "IDR_WAV_NUMBER_5",
  "IDR_WAV_NUMBER_6", "IDR_WAV_NUMBER_7", "IDR_WAV_NUMBER_8",
  "IDR_WAV_NUMBER_9",
};

/**
 * The digits as the last word of a number.  They fall like the end of
 * a sentence; said on their own, "zero point six" sounds like a
 * question.
 */
static constexpr const char *last_number_clips[] = {
  "IDR_WAV_NUMBER_0_END", "IDR_WAV_NUMBER_1_END", "IDR_WAV_NUMBER_2_END",
  "IDR_WAV_NUMBER_3_END", "IDR_WAV_NUMBER_4_END", "IDR_WAV_NUMBER_5_END",
  "IDR_WAV_NUMBER_6_END", "IDR_WAV_NUMBER_7_END", "IDR_WAV_NUMBER_8_END",
  "IDR_WAV_NUMBER_9_END",
};

/**
 * Round to a multiple of #step and clamp to #limit steps, so the
 * number never needs more words than there are clips.
 */
static long
RoundToSteps(double value, double step, long limit) noexcept
{
  return std::clamp(std::lround(value / step), -limit, limit);
}

StaticArray<const char *, 5>
GetSpokenVerticalSpeedClips(double value, Unit unit) noexcept
{
  StaticArray<const char *, 5> clips;

  if (unit == Unit::FEET_PER_MINUTE) {
    const long hundreds = RoundToSteps(value, 100, 99);
    if (hundreds == 0) {
      clips.push_back(last_number_clips[0]);
      return clips;
    }

    if (hundreds < 0)
      clips.push_back("IDR_WAV_MINUS");

    const long magnitude = std::labs(hundreds);
    if (magnitude >= 10) {
      clips.push_back(number_clips[magnitude / 10]);
      clips.push_back(magnitude % 10 == 0
                      ? "IDR_WAV_THOUSAND_END"
                      : "IDR_WAV_THOUSAND");
    }

    /* "hundred" is always the last word, and its clip falls */
    if (magnitude % 10 != 0) {
      clips.push_back(number_clips[magnitude % 10]);
      clips.push_back("IDR_WAV_HUNDRED");
    }

    return clips;
  }

  const long tenths = RoundToSteps(value, 0.1, 999);
  if (tenths < 0)
    clips.push_back("IDR_WAV_MINUS");

  const long magnitude = std::labs(tenths);
  if (magnitude >= 100)
    clips.push_back(number_clips[magnitude / 100]);

  clips.push_back(number_clips[magnitude / 10 % 10]);
  clips.push_back("IDR_WAV_POINT");
  clips.push_back(last_number_clips[magnitude % 10]);
  return clips;
}
