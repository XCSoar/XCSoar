/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"

#include <atomic>
#include <cstdint>

class LXDevice: public AbstractDevice
{
  enum class Mode : uint8_t {
    UNKNOWN,
    NMEA,
    PASS_THROUGH,
    COMMAND,
  };

  Port &port;

  const unsigned bulk_baud_rate;

  std::atomic<bool> busy{false};

  /**
   * Is this ia Colibri or LX20 or a similar "old" logger?  This is
   * initialised to true if the NMEA baud rate is configured to 4800,
   * which disables the advanced LX Vario/Nano protocol, to avoid confusing
   * the Colibri.  It is cleared as soon as a "modern" LX product is
   * detected (passively).
   */
  bool is_colibri;

  /*
   * Indicates whether pass-through mode should be used
   */
  const bool use_pass_through;

  /**
   * Was a LXNAV V7 detected?
   */
  bool is_v7 = false;

  /**
   * Was a LXNAV S series vario detected?
   */
  bool is_sVario = false;

  /**
   * Was a LXNAV Nano detected?
   */
  bool is_nano;

  /**
   * Was a LXNAV Nano identified via the port setup?
   */
  const bool port_is_nano;

  /**
   * Was a LXNavigation LX1600/1606 vario detected?
   */
  bool is_lx16xx = false;

  /**
   * Was a vario with a Nano on the GPS port detected?
   */
  bool is_forwarded_nano = false;

  /**
   * Settings that were received in PLXV0 (LXNAV Vario) sentences.
   */
  DeviceSettingsMap<std::string> lxnav_vario_settings;

  /**
   * Settings that were received in PLXVC (LXNAV Nano) sentences.
   */
  DeviceSettingsMap<std::string> nano_settings;

  Mutex mutex;
  Mode mode = Mode::UNKNOWN;
  unsigned old_baud_rate = 0;

public:
  LXDevice(Port &_port, unsigned baud_rate, unsigned _bulk_baud_rate,
           bool _use_pass_through, bool _port_is_nano=false) noexcept
    :port(_port), bulk_baud_rate(_bulk_baud_rate),
     is_colibri(baud_rate == 4800), use_pass_through(_use_pass_through),
     is_nano(_port_is_nano), port_is_nano(_port_is_nano) {}

  /**
   * Was a LXNAV V7 detected?
   */
  bool IsV7() const noexcept {
    return is_v7;
  }

  /**
   * Was a LXNAV S series vario detected?
   */
  bool IsSVario() const noexcept {
    return is_sVario;
  }

  /**
   * Was a LXNAV vario device detected?
   */
  bool IsLXNAVVario() const noexcept {
    return IsV7() || IsSVario();
  }

  /**
   * Was a LXNAV Nano detected?
   */
  bool IsNano() const noexcept {
    return port_is_nano || is_nano || is_forwarded_nano;
  }

  /**
   * Was an LXNAV logger device detected?
   */
  bool IsLXNAVLogger() const noexcept {
    return IsNano() || IsSVario();
  }

  /**
   * Was a LXNavigation LX1600/1606 vario detected?
   */
  bool IsLX16xx() const noexcept {
    return is_lx16xx;
  }

  /**
   * Can this device be managed by XCSoar?
   */
  bool IsManageable() const noexcept {
    return IsV7() || IsSVario() || IsNano() || IsLX16xx();
  }

  bool UsePassThrough() const noexcept {
    return use_pass_through;
  }

  void ResetDeviceDetection() noexcept {
    is_v7 = is_sVario = is_nano = is_lx16xx = is_forwarded_nano = false;
  }

  void IdDeviceByName(NarrowString<16> productName) noexcept
  {
    is_v7 = productName.equals("V7");
    is_sVario = productName.equals("NINC") || productName.equals("S8x");
    is_nano = productName.equals("NANO") || productName.equals("NANO3") || productName.equals("NANO4");
    is_lx16xx = productName.equals("1606") || productName.equals("1600");
  }

  /**
   * Write a setting to a LXNAV Vario.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Vario has understood and processed it)
   */
  bool SendLXNAVVarioSetting(const char *name, const char *value,
                     OperationEnvironment &env);

  /**
   * Request a setting from a LXNAV Vario.  The Vario will send the value,
   * but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Vario has understood and processed it)
   */
  bool RequestLXNAVVarioSetting(const char *name, OperationEnvironment &env);

  /**
   * Wait for the specified setting to be received.  Returns the value
   * on success, or an empty string on timeout.
   */
  std::string WaitLXNAVVarioSetting(const char *name, OperationEnvironment &env,
                            unsigned timeout_ms);

  /**
   * Look up the given setting in the table of received LXNAV Vario
   * values.  If the value does not exist, an empty string is
   * returned.
   */
  gcc_pure
  std::string GetLXNAVVarioSetting(const char *name) const noexcept;

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
  std::string GetNanoSetting(const char *name) const noexcept;

protected:
  /**
   * A variant of EnableNMEA() that attempts to put the Nano into NMEA
   * mode.  If the Nano is connected through a LXNAV V7, it will
   * enable pass-through mode on the V7.
   */
  bool EnableLoggerNMEA(OperationEnvironment &env);

  bool EnableCommandMode(OperationEnvironment &env);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment &env) override;

  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;

  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutPilotEvent(OperationEnvironment &env) override;

  bool EnablePassThrough(OperationEnvironment &env) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;

  void OnSysTicker() override;

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight,
                      Path path,
                      OperationEnvironment &env) override;
};

#endif
