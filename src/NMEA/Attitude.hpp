// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "time/Validity.hpp"

#include <cstdint>

/**
 * A container holding the aircraft current attitude state.
 */
struct AttitudeState
{
  /** Estimated bank angle */
  Angle bank_angle;

  /** Estimated pitch angle */
  Angle pitch_angle;

  /** Estimated heading */
  Angle heading;

  Validity bank_angle_available;
  Validity pitch_angle_available;
  Validity heading_available;

  /**
   * Where #heading comes from.  A device only ever reports a compass
   * heading; BasicComputer fills in the other two when no device has
   * one, and the display marks them, because they are not where the
   * nose points but an estimate of it.
   */
  enum class HeadingSource : uint8_t {
    /** the compass of a connected device */
    COMPASS,

    /** the GPS track corrected for the wind */
    WIND,

    /** the GPS track alone, while the wind is not known */
    TRACK,
  } heading_source;

  /**
   * The short title of #heading for a display: "HDG" from a compass,
   * "HDG*" for the track corrected for the wind and "TRK*" for the
   * track alone, so that the pilot sees how far he can trust it.  The
   * ICAO abbreviations are not translated.
   */
  constexpr const char *GetHeadingTitle() const noexcept {
    switch (heading_source) {
    case HeadingSource::COMPASS:
      break;

    case HeadingSource::WIND:
      return "HDG*";

    case HeadingSource::TRACK:
      return "TRK*";
    }

    return "HDG";
  }

  /**
   * Invalidate all data held in this object.
   */
  constexpr void Reset() noexcept {
    bank_angle_available.Clear();
    pitch_angle_available.Clear();
    heading_available.Clear();
    heading_source = HeadingSource::COMPASS;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void Complement(const AttitudeState &add) noexcept;

  void Expire(TimeStamp now) noexcept;
};
