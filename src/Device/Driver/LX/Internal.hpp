// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "Device/Config.hpp"
#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "Geo/GeoPoint.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Engine/Task/TaskType.hpp"
#include "time/PeriodClock.hpp"

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

struct MoreData;
struct DerivedInfo;

/**
 * LXNAV polar coefficients use a normalised speed where
 * v==1 corresponds to 100 km/h.  This is the conversion factor
 * from that unit to m/s.
 */
static constexpr double LX_POLAR_V = 100.0 / 3.6;

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

  const DeviceConfig::PolarSync polar_sync;

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
    std::string name;
    double stall = 0;
    bool valid = false;
  } device_polar;

public:
  /**
   * Declaration H-records read from the device via PLXVC,DECL,R.
   * Used to match or create a plane profile by registration.
   */
  struct DeviceDeclaration {
    std::string pilot_name;
    std::string glider_type;
    std::string registration;
    std::string competition_id;
    unsigned lines_received = 0;
    unsigned total_lines = 0;
    bool complete = false;
    bool processed = false;
  };

private:
  DeviceDeclaration device_declaration;

  bool declaration_requested = false;

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

  /**
   * Has the polar sync notification been shown this session?
   */
  bool polar_sync_notified = false;

  /**
   * Name of the last navigation target sent to the device via PLXVTARG.
   * Used to avoid resending unchanged targets.
   */
  std::string last_sent_target_name;

  /**
   * Location of the last navigation target sent to the device.
   */
  GeoPoint last_sent_target_location = GeoPoint::Invalid();

  /**
   * Previous task type, used to detect GoTo→Ordered transitions
   * so we can send one re-sync target to the vario.
   */
  TaskType last_task_type = TaskType::NONE;

  Mutex mutex;
  Mode mode = Mode::UNKNOWN;
  unsigned old_baud_rate = 0;

public:
  LXDevice(Port &_port, unsigned baud_rate, unsigned _bulk_baud_rate,
           bool _use_pass_through,
           DeviceConfig::PolarSync _polar_sync = DeviceConfig::PolarSync::OFF,
           bool _port_is_nano=false) noexcept
    :port(_port), bulk_baud_rate(_bulk_baud_rate),
     polar_sync(_polar_sync),
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
    polar_sync_notified = false;
    device_polar.valid = false;
    last_sent_target_name.clear();
    last_sent_target_location = GeoPoint::Invalid();
    last_task_type = TaskType::NONE;
  }

  /**
   * Identify a detected device by its product name string and update
   * internal device-type flags.  Logs newly detected vario types.
   */
  void IdDeviceByName(const StaticString<16> &product_name,
                      const DeviceInfo &device_info) noexcept;

  /**
   * Update device-type flags from a DeviceInfo product field.
   * In pass-through mode flags are only set, never cleared.
   * Sets #vario_just_detected when a vario is first seen.
   */
  void UpdateDeviceFlags(const DeviceInfo &device_info,
                         bool pass_through) noexcept;

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
   * Prepare the port for communicating with the LXNAV logger.
   *
   * For S-series varios (built-in logger): always stays in NMEA
   * mode so PLXVC commands reach the vario's own logger.  If a
   * FLARM is behind the vario, its flight downloads are handled
   * by the FLARM driver on a separate device slot.
   *
   * For V7 with a Nano behind it: enables pass-through mode so
   * PLXVC commands reach the Nano logger.
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
  bool PutActiveFrequency(RadioFrequency frequency,
                          const char *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const char *name,
                           OperationEnvironment &env) override;
  bool ExchangeRadioFrequencies(OperationEnvironment &env,
                                NMEAInfo &info) override;
  bool PutTransponderCode(TransponderCode code,
                          OperationEnvironment &env) override;
  bool PutPolar(const GlidePolar &polar,
                OperationEnvironment &env) override;
  bool PutTarget(const GeoPoint &location, const char *name,
                 std::optional<double> elevation,
                 OperationEnvironment &env) override;

  bool EnablePassThrough(OperationEnvironment &env) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;

  void OnSysTicker() override;

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

private:
  /**
   * Like IdDeviceByName(), but assumes #mutex is already held.
   */
  void IdDeviceByNameLocked(const StaticString<16> &product_name,
                            const DeviceInfo &device_info) noexcept;

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
   * Track polar changes and reset sent values when the polar
   * changes (e.g. due to plane profile switch).
   */
  void TrackPolarChanges(const DerivedInfo &calculated) noexcept;

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight,
                      Path path,
                      OperationEnvironment &env) override;
};
