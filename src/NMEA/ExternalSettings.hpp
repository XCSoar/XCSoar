/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_EXTERNAL_SETTINGS_HPP
#define XCSOAR_EXTERNAL_SETTINGS_HPP

#include "NMEA/Validity.hpp"
#include "Math/fixed.hpp"
#include "Engine/Atmosphere/Pressure.hpp"

/**
 * Settings received from an external device.
 */
struct ExternalSettings {
  Validity mac_cready_available;

  /** MacCready value (m/s) of external device (if available) */
  fixed mac_cready;

  Validity ballast_fraction_available;

  /** Ballast information (1: full, 0: empty) of external device (if available) */
  fixed ballast_fraction;

  Validity ballast_overload_available;

  /**
   * Ballast information (1: no ballast, 1.5: ballast = 0.5 times dry mass)
   * of external device (if available)
   */
  fixed ballast_overload;

  Validity wing_loading_available;

  /** Wing loading information (kg/m^2) of external device (if available) */
  fixed wing_loading;

  Validity bugs_available;

  /** Bugs information (1: clean, 0: dirty) of external device (if available) */
  fixed bugs;

  Validity qnh_available;

  /** the QNH setting [hPa] */
  AtmosphericPressure qnh;

  void Clear();
  void Expire(fixed time);
  void Complement(const ExternalSettings &add);

  /**
   * Clear attributes that have the same values like those in the
   * "other" object, but only if that is different from the values in
   * "last".
   *
   * This is used by the device code to break the feedback loop: when
   * a setting is sent to the device, and the device reports back the
   * new setting, it is ignored, by calling this method.
   */
  void EliminateRedundant(const ExternalSettings &other,
                          const ExternalSettings &last);

  /**
   * Compare the MacCready setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareMacCready(fixed value) const {
    return mac_cready_available && fabs(mac_cready - value) <= fixed(0.05);
  }

  /**
   * Compare the ballast fraction setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBallastFraction(fixed value) const {
    return ballast_fraction_available &&
      fabs(ballast_fraction - value) <= fixed(0.01);
  }

  /**
   * Compare the ballast overload setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBallastOverload(fixed value) const {
    return ballast_overload_available &&
      fabs(ballast_overload - value) <= fixed(0.01);
  }

  /**
   * Compare the wing loading setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareWingLoading(fixed value) const {
    return wing_loading_available &&
      fabs(wing_loading - value) <= fixed_half;
  }

  /**
   * Compare the bugs setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareBugs(fixed value) const {
    return bugs_available && fabs(bugs - value) <= fixed(0.01);
  }

  /**
   * Compare the QNH setting with the specified value.
   *
   * @return true if the current setting is the same, false if the
   * value is different or if there is no value
   */
  bool CompareQNH(fixed value) const {
    return qnh_available && fabs(qnh.GetQNH() - value) <= fixed_half;
  }

  /**
   * Sets a new MacCready value, but updates the time stamp only if
   * the value has changed.
   *
   * @return true if the value and the time stamp have been updated
   */
  bool ProvideMacCready(fixed value, fixed time);
  bool ProvideBallastFraction(fixed value, fixed time);
  bool ProvideBallastOverload(fixed value, fixed time);
  bool ProvideWingLoading(fixed value, fixed time);
  bool ProvideBugs(fixed value, fixed time);
  bool ProvideQNH(fixed value, fixed time);
};

#endif
