// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "LogFile.hpp"
#include "time/PeriodClock.hpp"

#include <atomic>
#include <cstdint>
#include <optional>

struct MoreData;
struct DerivedInfo;

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
   * Has the firmware version been logged?
   */
  bool firmware_version_logged = false;

  /**
   * Settings that were received in PLXV0 (LXNAV Vario) sentences.
   */
  DeviceSettingsMap<std::string> lxnav_vario_settings;

  /**
   * Settings that were received in PLXVC (LXNAV Nano) sentences.
   */
  DeviceSettingsMap<std::string> nano_settings;

  /**
   * Has MC been requested from the device?
   */
  bool mc_requested = false;

  /**
   * Last MC value sent to the device. Used to avoid re-sending unchanged values
   * and to detect echoes from the device.
   */
  std::optional<double> last_sent_mc;

  /**
   * Has ballast been requested from the device?
   */
  bool ballast_requested = false;

  /**
   * Last ballast overload value sent to the device. Used to detect feedback loops.
   */
  std::optional<double> last_sent_ballast_overload;

  /**
   * Last crew mass value when ballast was sent. Used to detect crew weight changes.
   */
  std::optional<double> last_sent_crew_mass;

  /**
   * Last empty mass value sent to the device. Used to detect feedback loops.
   */
  std::optional<double> last_sent_empty_mass;

  /**
   * Tracked polar values to detect when plane profile changes.
   */
  struct TrackedPolar {
    double a = 0, b = 0, c = 0;
    double reference_mass = 0;
    double empty_mass = 0;
    double crew_mass = 0;
    bool valid = false;
  } tracked_polar;


  /**
   * Full polar data received from device via POLAR command.
   * Used when no plane profile is active.
   */
  struct DevicePolar {
    double a = 0, b = 0, c = 0;
    double polar_load = 0;
    double polar_weight = 0;
    double max_weight = 0;
    double empty_weight = 0;
    double pilot_weight = 0;
    bool valid = false;
  } device_polar;

  /**
   * Has POLAR been requested from the device?
   */
  bool polar_requested = false;

  /**
   * Clock to track when POLAR was last requested for periodic polling.
   */
  PeriodClock last_polar_request;

  /**
   * Has bugs been requested from the device?
   */
  bool bugs_requested = false;

  /**
   * Last bugs value sent to the device. Used to detect feedback loops.
   */
  std::optional<double> last_sent_bugs;

  /**
   * Was a vario just detected? Used to trigger settings request on detection.
   */
  bool vario_just_detected = false;

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

  void IdDeviceByName(StaticString<16> productName, const DeviceInfo &device_info) noexcept
  {
    const bool new_v7 = productName.equals("V7");
    const bool new_sVario = productName.equals("NINC") || productName.equals("S8x");
    const bool new_nano = productName.equals("NANO") || productName.equals("NANO3") || productName.equals("NANO4");
    const bool new_lx16xx = productName.equals("1606") || productName.equals("1600");

    {
      const std::lock_guard lock{mutex};
      if ((new_v7 && !is_v7) || (new_sVario && !is_sVario)) {
        const char *device_type = new_v7 ? "V7" : "S series vario";
        LogFmt("LXNAV: {} detected via PLXVC (product: {}, firmware: {})",
               device_type,
               productName.c_str(),
               device_info.software_version.empty() ? "unknown" : device_info.software_version.c_str());
        if (!device_info.software_version.empty())
          firmware_version_logged = true;
      }

      is_v7 = new_v7;
      is_sVario = new_sVario;
      is_nano = new_nano;
      is_lx16xx = new_lx16xx;
    }
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
  [[gnu::pure]]
  std::string GetLXNAVVarioSetting(const char *name) const noexcept;

  /**
   * Write a setting to a LXNAV Nano.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the Nano has understood and processed it)
   */
  bool SendNanoSetting(const char *name, const char *value,
                       OperationEnvironment &env);

  bool SendNanoSetting(const char *name, unsigned value,
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
  [[gnu::pure]]
  std::string GetNanoSetting(const char *name) const noexcept;

  [[gnu::pure]]
  unsigned GetNanoSettingInteger(const char *name) const noexcept;

protected:
  /**
   * A variant of EnableNMEA() that attempts to put the Nano into NMEA
   * mode.  If the Nano is connected through a LXNAV V7, it will
   * enable pass-through mode on the V7.
   */
  bool EnableLoggerNMEA(OperationEnvironment &env);

  bool EnableCommandMode(OperationEnvironment &env);

public:
  // These methods are reused by the LX Eos driver
  static void LXWP1(NMEAInputLine &line, DeviceInfo &device);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment &env) override;

  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool PutBallast(double fraction, double overload,
                  OperationEnvironment &env) override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutMacCready(double mc, OperationEnvironment &env) override;
  bool PutCrewMass(double crew_mass, OperationEnvironment &env) override;
  bool PutEmptyMass(double empty_mass, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pres,
              OperationEnvironment &env) override;

  bool PutElevation(int elevation, OperationEnvironment &env) override;
  bool RequestElevation(OperationEnvironment &env) override;

  bool PutVolume(unsigned volume, OperationEnvironment &env) override;
  bool PutPilotEvent(OperationEnvironment &env) override;

  bool EnablePassThrough(OperationEnvironment &env) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;

  void OnSysTicker() override;

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

private:
  /**
   * Check if MC value from device is an echo of what we sent.
   */
  [[gnu::pure]]
  bool IsMCEcho(const ExternalSettings &settings) const noexcept {
    if (!last_sent_mc.has_value())
      return false;
    return settings.CompareMacCready(*last_sent_mc);
  }

  /**
   * Check if ballast value from device is an echo of what we sent.
   */
  [[gnu::pure]]
  bool IsBallastEcho(const ExternalSettings &settings) const noexcept {
    if (!last_sent_ballast_overload.has_value())
      return false;
    return settings.CompareBallastOverload(*last_sent_ballast_overload);
  }

  /**
   * Check if bugs value from device is an echo of what we sent.
   */
  [[gnu::pure]]
  bool IsBugsEcho(const ExternalSettings &settings) const noexcept {
    if (!last_sent_bugs.has_value())
      return false;
    return settings.CompareBugs(*last_sent_bugs);
  }

  /**
   * Check if pilot weight from device is an echo of what we sent.
   */
  [[gnu::pure]]
  bool IsCrewWeightEcho(const ExternalSettings &settings) const noexcept {
    if (!last_sent_crew_mass.has_value())
      return false;
    return settings.ComparePolarPilotWeight(*last_sent_crew_mass);
  }

  /**
   * Check if empty weight from device is an echo of what we sent.
   */
  [[gnu::pure]]
  bool IsEmptyWeightEcho(const ExternalSettings &settings) const noexcept {
    if (!last_sent_empty_mass.has_value())
      return false;
    return settings.ComparePolarEmptyWeight(*last_sent_empty_mass);
  }

  /**
   * Handle MC synchronization with the device.
   */
  void SyncMacCready(const MoreData &basic,
                     const DerivedInfo &calculated,
                     OperationEnvironment &env,
                     bool plane_profile_active) noexcept;

  /**
   * Handle Ballast synchronization with the device.
   */
  void SyncBallast(const MoreData &basic,
                   const DerivedInfo &calculated,
                   OperationEnvironment &env,
                   bool plane_profile_active) noexcept;

  /**
   * Handle Bugs synchronization with the device.
   */
  void SyncBugs(const MoreData &basic,
                const DerivedInfo &calculated,
                OperationEnvironment &env,
                bool plane_profile_active) noexcept;
  void SyncCrewWeight(const MoreData &basic,
                     const DerivedInfo &calculated,
                     OperationEnvironment &env,
                     bool plane_profile_active);
  void SyncEmptyWeight(const MoreData &basic,
                       const DerivedInfo &calculated,
                       OperationEnvironment &env,
                       bool plane_profile_active);

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight,
                      Path path,
                      OperationEnvironment &env) override;
};
