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

#ifndef XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP

#include "Protocol.hpp"
#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "Thread/Mutex.hpp"

#include <atomic>
#include <stdint.h>

namespace LX1600 {
  enum class Setting: uint8_t;
  class SettingsMap;
}

class LXDevice: public AbstractDevice
{
  enum class Mode : uint8_t {
    UNKNOWN,
    NMEA,
    PASS_THROUGH,
    COMMAND,
  };

  Port &port;

  unsigned bulk_baud_rate;

  std::atomic<bool> busy;

  /**
   * Is this ia Colibri or LX20 or a similar "old" logger?  This is
   * initialised to true if the NMEA baud rate is configured to 4800,
   * which disables the advanced V7/Nano protocol, to avoid confusing
   * the Colibri.  It is cleared as soon as a "modern" LX product is
   * detected (passively).
   */
  bool is_colibri;

  /**
   * Was a LXNAV V7 detected?
   */
  bool is_v7;

  /**
   * Was a LXNAV Nano detected?
   */
  bool is_nano;

  /**
   * Was a LXNavigation LX1600/1606 vario detected?
   */
  bool is_lx16xx;

  /**
   * Was a V7 with a Nano on the GPS port detected?
   */
  bool is_forwarded_nano;

  /**
   * Settings that were received in PLXV0 (LXNAV V7) sentences.
   */
  DeviceSettingsMap<std::string, std::string> v7_settings;

  /**
   * Settings that were received in PLXVC (LXNAV Nano) sentences.
   */
  DeviceSettingsMap<std::string, std::string> nano_settings;

  /**
   * Settings that were received in PLXVC (LXNAV Nano) sentences.
   */
  DeviceSettingsMap<LX1600::Setting, std::string> lx16xx_settings;

  Mutex mutex;
  Mode mode;
  unsigned old_baud_rate;

public:
  LXDevice(Port &_port, unsigned baud_rate, unsigned _bulk_baud_rate,
           bool _is_nano=false)
    :port(_port), bulk_baud_rate(_bulk_baud_rate),
     busy(false),
     is_colibri(baud_rate == 4800),
     is_v7(false), is_nano(_is_nano), is_lx16xx(false),
     is_forwarded_nano(false),
     mode(Mode::UNKNOWN), old_baud_rate(0) {}

  /**
   * Was a LXNAV V7 detected?
   */
  bool IsV7() const {
    return is_v7;
  }

  /**
   * Was a LXNAV Nano detected?
   */
  bool IsNano() const {
    return is_nano || is_forwarded_nano;
  }

  /**
   * Was a LXNavigation LX1600/1606 vario detected?
   */
  bool IsLX16xx() const {
    return is_lx16xx;
  }

  void ResetDeviceDetection() {
    is_v7 = is_nano = is_lx16xx = is_forwarded_nano = false;
  }

  /**
   * Write a setting to a LXNAV V7.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the V7 has understood and processed it)
   */
  bool SendV7Setting(const char *name, const char *value,
                     OperationEnvironment &env);

  /**
   * Request a setting from a LXNAV V7.  The V7 will send the value,
   * but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the V7 has understood and processed it)
   */
  bool RequestV7Setting(const char *name, OperationEnvironment &env);

  /**
   * Wait for the specified setting to be received.  Returns the value
   * on success, or an empty string on timeout.
   */
  std::string WaitV7Setting(const char *name, OperationEnvironment &env,
                            unsigned timeout_ms);

  /**
   * Look up the given setting in the table of received LXNAV V7
   * values.  If the value does not exist, an empty string is
   * returned.
   */
  gcc_pure
  std::string GetV7Setting(const char *name) const;

  /**
   * Write a setting to a LXNAV Nano.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Nano has understood and processed it)
   */
  bool SendNanoSetting(const char *name, const char *value,
                       OperationEnvironment &env);

  /**
   * Request a setting from a LXNAV Nano.  The Nano will send the
   * value, but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Nano has understood and processed it)
   */
  bool RequestNanoSetting(const char *name, OperationEnvironment &env);

  /**
   * Wait for the specified setting to be received.  Returns the value
   * on success, or an empty string on timeout.
   */
  std::string WaitNanoSetting(const char *name, OperationEnvironment &env,
                              unsigned timeout_ms);

  /**
   * Look up the given setting in the table of received LXNAV Nano
   * values.  If the value does not exist, an empty string is
   * returned.
   */
  gcc_pure
  std::string GetNanoSetting(const char *name) const;

  /**
   * Write all settings to a LX 16xx.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the device has understood and processed it)
   */
  bool SendLX16xxSettings(const LX1600::SettingsMap &settings,
                          OperationEnvironment &env);

  /**
   * Request all settings from a LX 16xx.  The device will send the
   * values, but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the device has understood and processed it)
   */
  bool RequestLX16xxSettings(OperationEnvironment &env);

  /**
   * Wait for the specified setting to be received.  Returns the value
   * on success, or an empty string on timeout.
   */
  std::string WaitLX16xxSetting(LX1600::Setting key,
                                OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Look up the given setting in the table of received LX 16xx
   * values.  If the value does not exist, an empty string is
   * returned.
   */
  gcc_pure
  std::string GetLX16xxSetting(LX1600::Setting key) const;

protected:
  /**
   * A variant of EnableNMEA() that attempts to put the Nano into NMEA
   * mode.  If the Nano is connected through a LXNAV V7, it will
   * enable pass-through mode on the V7.
   */
  bool EnableNanoNMEA(OperationEnvironment &env);

  bool EnableCommandMode(OperationEnvironment &env);

public:
  virtual void LinkTimeout() override;
  virtual bool EnableNMEA(OperationEnvironment &env) override;

  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  virtual bool PutBallast(fixed fraction, fixed overload,
                          OperationEnvironment &env) override;
  virtual bool PutBugs(fixed bugs, OperationEnvironment &env) override;
  virtual bool PutMacCready(fixed mc, OperationEnvironment &env) override;
  virtual bool PutQNH(const AtmosphericPressure &pres,
                      OperationEnvironment &env) override;

  virtual bool PutVolume(unsigned volume, OperationEnvironment &env) override;

  virtual bool EnablePassThrough(OperationEnvironment &env) override;

  virtual bool Declare(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env) override;

  virtual void OnSysTicker() override;

  virtual bool ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env) override;
  virtual bool DownloadFlight(const RecordedFlightInfo &flight,
                              const TCHAR *path,
                              OperationEnvironment &env) override;
};

#endif
