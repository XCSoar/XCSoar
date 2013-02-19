/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_LX_LX1600_HPP
#define XCSOAR_DEVICE_DRIVER_LX_LX1600_HPP

#include "Device/Port/Port.hpp"
#include "Device/Internal.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Units/System.hpp"
#include "Util/StaticString.hpp"

#include <map>

#include <assert.h>
#include <stdint.h>

/**
 * Code specific to LX Navigation varios (e.g. LX1600).
 */
namespace LX1600 {
  enum class SCMode : uint8_t {
    EXTERNAL = 0,
    ON_CIRCLING = 1,
    AUTO_IAS = 2,
  };

  enum class SCControlMode : uint8_t {
    NORMAL = 0,
    INVERTED = 1,
    TASTER = 2,
  };

  enum class Setting : uint8_t {
    VARIO_AVG_TIME,
    VARIO_RANGE,
    VARIO_FILTER,
    TE_FILTER,
    TE_LEVEL,
    SMART_VARIO_FILTER,
    SC_MODE,
    SC_DEADBAND,
    SC_CONTROL_MODE,
    SC_THRESHOLD_SPEED,
    GLIDER_NAME,
    TIME_OFFSET,
  };

  class SettingsMap : public std::map<Setting, std::string> {};

  /**
   * Enable pass-through mode on the LX1600.  This command was provided
   * by Crtomir Rojnik (LX Navigation) in an email without further
   * explanation.  Tests have shown that this command can be sent at
   * either 4800 baud or the current vario baud rate.  Since both works
   * equally well, we don't bother to switch.
   */
  static inline bool
  ModeColibri(Port &port, OperationEnvironment &env)
  {
    return PortWriteNMEA(port, "PFLX0,COLIBRI", env);
  }

  /**
   * Cancel pass-through mode on the LX1600.  This command was provided
   * by Crtomir Rojnik (LX Navigation) in an email.  It must always be
   * sent at 4800 baud.  After this command has been sent, we switch
   * back to the "real" baud rate.
   */
  static inline bool
  ModeLX1600(Port &port, OperationEnvironment &env)
  {
    unsigned old_baud_rate = port.GetBaudrate();
    if (old_baud_rate == 4800)
      old_baud_rate = 0;
    else if (old_baud_rate != 0 && !port.SetBaudrate(4800))
      return false;

    const bool success = PortWriteNMEA(port, "PFLX0,LX1600", env);

    if (old_baud_rate != 0)
      port.SetBaudrate(old_baud_rate);

    return success;
  }

  /**
   * Store the current settings into the EEPROM of the device.
   */
  static inline bool
  SaveToEEPROM(Port &port, OperationEnvironment &env)
  {
    return PortWriteNMEA(port, "PFLX0,EEPROM", env);
  }

  /**
   * Initialize all settings to default, writes to EEPROM and resets unit.
   */
  static inline bool
  FactoryReset(Port &port, OperationEnvironment &env)
  {
    return PortWriteNMEA(port, "PFLX0,INITEEPROM", env);
  }

  static inline bool
  SetupNMEA(Port &port, OperationEnvironment &env)
  {
    /*
     * This line sets the requested NMEA sentences on the device.
     * LXWP0: every second
     * LXWP1+3+5: once every 60 seconds
     * LXWP2: once every 10 seconds
     */
    return PortWriteNMEA(port, "PFLX0,LXWP0,1,LXWP1,60,LXWP2,10,LXWP3,60,LXWP5,60", env);
  }

  /**
   * Set the MC setting of the LX16xx vario
   * @param mc in m/s
   */
  static inline bool
  SetMacCready(Port &port, OperationEnvironment &env, fixed mc)
  {
    assert(mc >= fixed(0.0) && mc <= fixed(5.0));

    char buffer[32];
    sprintf(buffer, "PFLX2,%1.1f,,,,,,", (double)mc);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the ballast setting of the LX16xx vario
   * @param overload 1.0 - 1.5 (100 - 140%)
   */
  static inline bool
  SetBallast(Port &port, OperationEnvironment &env, fixed overload)
  {
    assert(overload >= fixed(1.0) && overload <= fixed(1.5));

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,%.2f,,,,", (double)overload);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the bugs setting of the LX16xx vario
   * @param bugs 0 - 30 %
   */
  static inline bool
  SetBugs(Port &port, OperationEnvironment &env, unsigned bugs)
  {
    assert(bugs <= 30);

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,,%u,,,", bugs);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the altitude offset of the LX16xx vario
   * @param altitude_offset offset necessary to set QNE in ft (default=0)
   */
  static inline bool
  SetAltitudeOffset(Port &port, OperationEnvironment &env, fixed altitude_offset)
  {
    char buffer[100];
    sprintf(buffer, "PFLX3,%.2f,,,,,,,,,,,,", (double)altitude_offset);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the QNH setting of the LX16xx vario
   */
  static inline bool
  SetQNH(Port &port, OperationEnvironment &env, const AtmosphericPressure &qnh)
  {
    assert(qnh.IsPlausible());

    fixed altitude_offset = Units::ToUserUnit(
        -AtmosphericPressure::StaticPressureToPressureAltitude(qnh),
        Unit::FEET);

    return SetAltitudeOffset(port, env, altitude_offset);
  }

  /**
   * Set the polar coefficients of the LX16xx vario
   *
   * These are the polar coefficients in LX format
   * (i.e. for v=(km/h*100) and w=(m/s))
   */
  static inline bool
  SetPolar(Port &port, OperationEnvironment &env, fixed a, fixed b, fixed c)
  {
    char buffer[100];
    sprintf(buffer, "PFLX2,,,,%.2f,%.2f,%.2f,", (double)a, (double)b, (double)c);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the polar coefficients of the LX16xx vario
   * @param polar Polar coefficients in XCSoar format (SI, m/s)
   */
  static inline bool
  SetPolar(Port &port, OperationEnvironment &env, const PolarCoefficients &polar)
  {
    // Convert from m/s to (km/h)/100
    fixed polar_a = polar.a * 10000 / sqr(fixed(3.6));
    fixed polar_b = polar.b * 100 / fixed(3.6);
    fixed polar_c = polar.c;

    return SetPolar(port, env, polar_a, polar_b, polar_c);
  }

  /**
   * Set the audio volume setting of the LX16xx vario
   * @param volume 0 - 100 %
   */
  static inline bool
  SetVolume(Port &port, OperationEnvironment &env, unsigned volume)
  {
    assert(volume <= 100);

    if (volume > 99)
      volume = 99;

    char buffer[100];
    sprintf(buffer, "PFLX2,,,,,,,%u", volume);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the device settings to the values specified in the #settings map.
   *
   * This function is not performing any checks on the #settings map. The
   * caller is responsible for providing correct values in string form.
   */
  static inline bool
  SetSettings(Port &port, OperationEnvironment &env,
              const SettingsMap &settings)
  {
    Setting pflx3_settings[] = {
      Setting::SC_MODE,
      Setting::VARIO_FILTER,
      Setting::TE_FILTER,
      Setting::TE_LEVEL,
      Setting::VARIO_AVG_TIME,
      Setting::VARIO_RANGE,
      Setting::SC_DEADBAND,
      Setting::SC_CONTROL_MODE,
      Setting::SC_THRESHOLD_SPEED,
      Setting::SMART_VARIO_FILTER,
      Setting::GLIDER_NAME,
      Setting::TIME_OFFSET,
    };

    NarrowString<256> buffer("PFLX3,");
    for (auto setting : pflx3_settings) {
      buffer += ',';

      auto it = settings.find(setting);
      if (it != settings.end())
        buffer += it->second.c_str();
    }

    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the filter settings of the LX16xx vario
   *
   * @param vario_filter filtering of vario in seconds (float) default=1
   * @param te_filter filtering of TE compensation in seconds (float)
   * (from 0.1 to 2.0 default=1.5)
   * @param te_level level of TE compensation in %
   * (from 50 to 150 default=0) 0 -> TECOMP = OFF
   */
  static inline bool
  SetFilters(Port &port, OperationEnvironment &env,
             fixed vario_filter, fixed te_filter, unsigned te_level)
  {
    assert(te_filter >= fixed(0.1) && te_filter <= fixed(2.0));
    assert((te_level >= 50 && te_level <= 150) || te_level == 0);

    char buffer[100];
    sprintf(buffer, "PFLX3,,,%.1f,%.1f,%u",
            (double)vario_filter, (double)te_filter, te_level);
    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the speed command settings of the LX16xx vario
   *
   * @param mode methods for automatic SC switch index (default=ON_CIRCLING)
   * @param deadband area of silence in SC mode (float)
   * (from 0 to 10.0 m/s, 1.0 = silence between +1m/s and -1m/s, default=1)
   * @param control_mode external switch/taster function (default=INVERTED)
   * @param threshold_speed speed of automatic switch from vario to sc mode
   * (if SCMODE == 2) (from 50 to 150 km/h, default=110)
   */
  static inline bool
  SetSCSettings(Port &port, OperationEnvironment &env,
                SCMode mode, fixed deadband, SCControlMode control_mode,
                fixed threshold_speed = fixed(0))
  {
    assert((unsigned)mode <= (unsigned)SCMode::AUTO_IAS);
    assert(deadband >= fixed(0) && deadband <= fixed(10));
    assert((unsigned)control_mode <= (unsigned)SCControlMode::TASTER);
    assert(mode != SCMode::AUTO_IAS ||
           (threshold_speed >= fixed(50) && threshold_speed <= fixed(150)));

    char buffer[100];
    if (mode == SCMode::AUTO_IAS)
      sprintf(buffer, "PFLX3,,%u,,,,,,%.1f,%u,%.0f",
              (unsigned)mode, (double)deadband, (unsigned)control_mode,
              (double)threshold_speed);
    else
      sprintf(buffer, "PFLX3,,%u,,,,,,%.1f,%u",
              (unsigned)mode, (double)deadband, (unsigned)control_mode);

    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the vario settings of the LX16xx vario
   *
   * @param avg_time averaging time in seconds for integrator
   * (between 5s and 30s, default=25)
   * @param range range of the vario display (2.5, 5.0 or 10.0, default=5.0)
   */
  static inline bool
  SetVarioSettings(Port &port, OperationEnvironment &env,
                   unsigned avg_time, fixed range)
  {
    assert(avg_time >= 5 && avg_time <= 30);
    assert(range >= fixed(2.5) && range <= fixed(10));

    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,%u,%.1f", avg_time, (double)range);

    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the Smart VARIO filtering
   * @param filter filter setting in m/s^2
   */
  static inline bool
  SetSmartDiffFilter(Port &port, OperationEnvironment &env, fixed filter)
  {
    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,,,,,,%.1f", (double)filter);

    return PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the time offset of the LX16xx vario
   * @param offset time offset in hours
   */
  static inline bool
  SetTimeOffset(Port &port, OperationEnvironment &env, int offset)
  {
    assert(offset >= -14 && offset <= 14);

    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,,,,,,,,%d", offset);

    return PortWriteNMEA(port, buffer, env);
  }
}

#endif
