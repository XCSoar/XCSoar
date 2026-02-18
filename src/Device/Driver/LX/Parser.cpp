// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Geo/SpeedVector.hpp"
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"
#include "TransponderMode.hpp"
#include "Units/System.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"
#include "Math/Util.hpp"
#include "util/NumberParser.hpp"
#include "LogFile.hpp"

#include <optional>

using std::string_view_literals::operator""sv;

/**
 * Check whether a product name identifies an LXNAV Nano variant.
 */
[[gnu::pure]]
static bool
IsNanoProduct(const auto &product) noexcept
{
  return product.equals("NANO") ||
         product.equals("NANO3") ||
         product.equals("NANO4");
}

static bool
LXWP0(NMEAInputLine &line, NMEAInfo &info)
{
  /*
  $LXWP0,Y,222.3,1665.5,1.71,,,,,,239,174,10.1

   0 loger_stored (Y/N)
   1 IAS (kph) ----> Condor uses TAS!
   2 baroaltitude (m)
   3-8 vario (m/s) (last 6 measurements in last second)
   9 heading of plane
  10 windcourse (deg)
  11 windspeed (kph)
  */

  line.Skip();

  double airspeed;
  bool tas_available = line.ReadChecked(airspeed);
  if (tas_available && (airspeed < -50 || airspeed > 250))
    /* implausible */
    return false;
  double value;
  if (line.ReadChecked(value))
    /* a dump on a LX7007 has confirmed that the LX sends uncorrected
       altitude above 1013.25hPa here */
    info.ProvidePressureAltitude(value);

  if (tas_available)
    /*
     * Call ProvideTrueAirspeed() after ProvidePressureAltitude() to use
     * the provided altitude (if available)
     */
    info.ProvideTrueAirspeed(Units::ToSysUnit(airspeed, Unit::KILOMETER_PER_HOUR));

  if (line.ReadChecked(value))
    info.ProvideTotalEnergyVario(value);

  line.Skip(6);

  if (SpeedVector wind; line.ReadSpeedVectorKPH(wind))
    info.ProvideExternalWind(wind);

  return true;
}

void
LXDevice::LXWP1(NMEAInputLine &line, DeviceInfo &device)
{
  /*
   * $LXWP1,
   * instrument ID,
   * serial number,
   * software version,
   * hardware version,
   * license string
   */

  device.product = line.ReadView();
  device.serial = line.ReadView();
  device.software_version = line.ReadView();
  device.hardware_version = line.ReadView();
  device.license = line.ReadView();
}

static bool
LXWP2(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $LXWP2,
   * maccready value, (m/s)
   * ballast, (1.0 - 1.5)
   * bugs, (0 - 100%)
   * polar_a,
   * polar_b,
   * polar_c,
   * audio volume
   */

  double value;
  // MacCready value
  if (line.ReadChecked(value)) {
    info.settings.ProvideMacCready(value, info.clock);
  }

  // Ballast
  if (line.ReadChecked(value)) {
    info.settings.ProvideBallastOverload(value, info.clock);
  }

  // Bugs
  if (line.ReadChecked(value)) {
    double bugs;
    if (value <= 1.5 && value >= 1.0) {
      // LX160 (sw 3.04) reports bugs as 1.00, 1.05 or 1.10 (#2167)
      bugs = 2 - value;
    } else {
      // All other known LX devices report bugs as 0, 5, 10, 15, ...
      bugs = (100 - value) / 100.;
    }
    info.settings.ProvideBugs(bugs, info.clock);
  }

  line.Skip(3);

  unsigned volume;
  if (line.ReadChecked(volume))
    info.settings.ProvideVolume(volume, info.clock);

  return true;
}

static bool
LXWP3(NMEAInputLine &line, NMEAInfo &info)
{
  /*
   * $LXWP3,
   * altioffset
   * scmode
   * variofil
   * tefilter
   * televel
   * varioavg
   * variorange
   * sctab
   * sclow
   * scspeed
   * SmartDiff
   * glider name
   * time offset
   */

  double value;

  // Altitude offset -> QNH
  if (line.ReadChecked(value)) {
    value = Units::ToSysUnit(-value, Unit::FEET);
    auto qnh = AtmosphericPressure::PressureAltitudeToStaticPressure(value);
    info.settings.ProvideQNH(qnh, info.clock);
  }

  return true;
}

/**
 * Parse double from a string_view value field.
 * Returns the parsed value, or std::nullopt on failure.
 */
static std::optional<double>
ParseDoubleValue(std::string_view sv) noexcept
{
  const std::string s{sv};
  char *endptr;
  double d = ParseDouble(s.c_str(), &endptr);
  if (endptr > s.c_str())
    return d;
  return std::nullopt;
}

/**
 * Parse the POLAR value from a PLXV0 sentence.
 *
 * Format: <a>,<b>,<c>,<polar_load>,<polar_weight>,
 *         <max_weight>,<empty_weight>,<pilot_weight>,<name>,<stall>
 *
 * Coefficients arrive in LX units (v=1 corresponds to 100 km/h)
 * and are converted to SI (m/s) before storing.
 */
static void
ParsePLXV0Polar(std::string_view value, NMEAInfo &info) noexcept
{
  const std::string value_str{value};
  NMEAInputLine polar_line{value_str.c_str()};
  double a_lx, b_lx, c, polar_load, polar_weight,
    max_weight, empty_weight, pilot_weight;
  if (polar_line.ReadChecked(a_lx) &&
      polar_line.ReadChecked(b_lx) &&
      polar_line.ReadChecked(c) &&
      polar_line.ReadChecked(polar_load) &&
      polar_line.ReadChecked(polar_weight) &&
      polar_line.ReadChecked(max_weight) &&
      polar_line.ReadChecked(empty_weight) &&
      polar_line.ReadChecked(pilot_weight)) {

    const double a = a_lx / (LX_POLAR_V * LX_POLAR_V);
    const double b = b_lx / LX_POLAR_V;

    info.settings.ProvidePolarCoefficients(a, b, c, info.clock);
    info.settings.ProvidePolarLoad(polar_load, info.clock);
    info.settings.ProvidePolarReferenceMass(polar_weight, info.clock);
    info.settings.ProvidePolarMaximumMass(max_weight, info.clock);
    info.settings.ProvidePolarPilotWeight(pilot_weight, info.clock);
    info.settings.ProvidePolarEmptyWeight(empty_weight, info.clock);
  } else {
    LogFmt("LXNAV: Failed to parse POLAR values from: {}", value);
  }
}

/**
 * Parse the $PLXV0 sentence (LXNAV sVarios (including V7)).
 */
static bool
PLXV0(NMEAInputLine &line, DeviceSettingsMap<std::string> &settings,
      NMEAInfo &info)
{
  const auto name = line.ReadView();
  if (name.empty())
    return true;

  const auto type = line.ReadView();

  const auto value = line.Rest();

  if (!type.starts_with('W'))
    return true;

  const std::lock_guard<Mutex> lock{settings};
  settings.Set(std::string{name}, value);

  if (name == "ELEVATION"sv) {
    if (auto d = ParseDoubleValue(value))
      info.settings.ProvideElevation(iround(*d), info.clock);
  } else if (name == "MC"sv) {
    if (auto d = ParseDoubleValue(value))
      info.settings.ProvideMacCready(*d, info.clock);
  } else if (name == "POLAR"sv || name == "POL"sv) {
    ParsePLXV0Polar(value, info);
  }

  return true;
}

static void
ParseNanoVarioInfo(NMEAInputLine &line, DeviceInfo &device)
{
  device.product = line.ReadView();
  device.software_version = line.ReadView();
  line.Skip(); /* ver.date, e.g. "May 12 2012 21:38:28" */
  device.hardware_version = line.ReadView();
}

/**
 * Parse a PLXVC,RADIO,A response.
 *
 * $PLXVC,RADIO,A,<command>,<value>[,<name>]
 *
 * command: COMM (active), SBY (standby), VOL, SQUELCH, VOX
 * value: frequency in kHz (e.g. 128800) or volume percentage
 */
static void
ParseRadio(NMEAInputLine &line, NMEAInfo &info)
{
  const auto command = line.ReadView();
  unsigned value;
  if (!line.ReadChecked(value))
    return;

  if (command == "COMM"sv || command == "SBY"sv) {
    auto freq = RadioFrequency::FromKiloHertz(value);
    if (!freq.IsDefined())
      return;

    if (command == "COMM"sv) {
      info.settings.has_active_frequency.Update(info.clock);
      info.settings.active_frequency = freq;

      /* Optional station name */
      const auto name = line.ReadView();
      if (!name.empty())
        info.settings.active_freq_name.SetASCII(name);
    } else {
      info.settings.has_standby_frequency.Update(info.clock);
      info.settings.standby_frequency = freq;

      const auto name = line.ReadView();
      if (!name.empty())
        info.settings.standby_freq_name.SetASCII(name);
    }
  }
}

/**
 * Map an LXNAV transponder mode string to a TransponderMode.
 */
[[gnu::pure]]
static TransponderMode
ParseTransponderMode(std::string_view mode_str) noexcept
{
  static constexpr struct {
    std::string_view name;
    TransponderMode::Mode value;
  } table[] = {
    {"OFF"sv,   TransponderMode::OFF},
    {"SBY"sv,   TransponderMode::SBY},
    {"GND"sv,   TransponderMode::GND},
    {"ON"sv,    TransponderMode::ON},
    {"ALT"sv,   TransponderMode::ALT},
    {"IDENT"sv, TransponderMode::IDENT},
  };

  for (const auto &[name, value] : table)
    if (mode_str == name)
      return TransponderMode{value};

  return TransponderMode::Null();
}

/**
 * Parse a PLXVC,XPDR,A response.
 *
 * $PLXVC,XPDR,A,<command>,<value>
 *
 * command: SQUAWK, MODE, ALT, STATUS
 */
static void
ParseTransponder(NMEAInputLine &line, NMEAInfo &info)
{
  const auto command = line.ReadView();

  if (command == "SQUAWK"sv) {
    /* The protocol sends squawk as a display string (e.g. "7700").
       TransponderCode stores values in octal, so parse base 8. */
    char buf[8];
    line.Read(buf, sizeof(buf));
    auto code = TransponderCode::Parse(buf);
    if (code.IsDefined()) {
      info.settings.has_transponder_code.Update(info.clock);
      info.settings.transponder_code = code;
    }
  } else if (command == "MODE"sv) {
    const auto mode_str = line.ReadView();
    auto mode = ParseTransponderMode(mode_str);
    if (mode.IsDefined()) {
      info.settings.has_transponder_mode.Update(info.clock);
      info.settings.transponder_mode = mode;
    }
  }
}

/**
 * Parse H-record content from a declaration line.
 * Matches HFPLTPILOT:, HFGTYGLIDERTYPE:, HFGIDGLIDERID:,
 * HFCIDCOMPETITIONID: prefixes.
 */
static void
ParseDeclHRecord(const std::string_view content,
                 LXDevice::DeviceDeclaration &decl) noexcept
{
  using namespace std::string_view_literals;
  if (content.starts_with("HFPLTPILOT:"sv))
    decl.pilot_name = content.substr(11);
  else if (content.starts_with("HFGTYGLIDERTYPE:"sv))
    decl.glider_type = content.substr(16);
  else if (content.starts_with("HFGIDGLIDERID:"sv))
    decl.registration = content.substr(14);
  else if (content.starts_with("HFCIDCOMPETITIONID:"sv))
    decl.competition_id = content.substr(19);
}

/**
 * Parse the $PLXVC sentence (LXNAV Nano and sVarios).
 *
 * $PLXVC,<key>,<type>,<values>*<checksum><cr><lf>
 */
static void
PLXVC(NMEAInputLine &line, NMEAInfo &info,
      DeviceSettingsMap<std::string> &settings,
      LXDevice::DeviceDeclaration &device_declaration,
      Mutex &decl_mutex)
{
  const auto key = line.ReadView();
  const auto type = line.ReadView();

  if (key == "SET"sv && type.starts_with('A')) {
    const auto name = line.ReadView();
    const auto value = line.Rest();
    if (!name.empty()) {
      const std::lock_guard<Mutex> lock{settings};
      settings.Set(std::string{name}, value);
    }
  } else if (key == "DECL"sv && type.starts_with('A')) {
    /* PLXVC,DECL,A,<current_line>,<total_lines>,<content> */
    unsigned current_line = 0, total_lines = 0;
    if (!line.ReadChecked(current_line) ||
        !line.ReadChecked(total_lines))
      return;
    const auto content = line.Rest();

    const std::lock_guard lock{decl_mutex};
    if (device_declaration.complete)
      return;
    device_declaration.total_lines = total_lines;
    device_declaration.lines_received++;
    ParseDeclHRecord(content, device_declaration);
    if (current_line >= 6) {
      device_declaration.complete = true;

      /* Populate ExternalSettings with glider identity */
      if (!device_declaration.registration.empty())
        info.settings.ProvideGliderRegistration(
          device_declaration.registration.c_str(), info.clock);
      if (!device_declaration.competition_id.empty())
        info.settings.ProvideGliderCompetitionId(
          device_declaration.competition_id.c_str(), info.clock);
      if (!device_declaration.glider_type.empty())
        info.settings.ProvideGliderType(
          device_declaration.glider_type.c_str(), info.clock);
    }
  } else if (key == "INFO"sv && type.starts_with('A')) {
    ParseNanoVarioInfo(line, info.device);
  } else if (key == "GPSINFO"sv && type.starts_with('A')) {
    /* the LXNAV V7 (firmware >= 2.01) forwards the Nano's INFO
       sentence with the "GPS" prefix */

    const auto name = line.ReadView();
    if (name == "LXWP1"sv) {
      LXDevice::LXWP1(line, info.secondary_device);
    } else if (name == "INFO"sv) {
      const auto type2 = line.ReadView();
      if (type2.starts_with('A'))
        ParseNanoVarioInfo(line, info.secondary_device);
    }
  } else if (key == "RADIO"sv && type.starts_with('A')) {
    ParseRadio(line, info);
  } else if (key == "XPDR"sv && type.starts_with('A')) {
    ParseTransponder(line, info);
  }
}

/**
 * Parse the $PLXVF sentence (LXNAV sVarios (including V7)).
 *
 * $PLXVF,time ,AccX,AccY,AccZ,Vario,IAS,PressAlt*CS<CR><LF>
 *
 * Example: $PLXVF,,1.00,0.87,-0.12,-0.25,90.2,244.3,*CS<CR><LF>
 *
 * @see http://www.xcsoar.org/trac/raw-attachment/ticket/1666/V7%20dataport%20specification%201.97.pdf
 */
static bool
PLXVF(NMEAInputLine &line, NMEAInfo &info)
{
  line.Skip();

  double a[3];
  bool a_available = line.ReadChecked(a[0]);
  if (!line.ReadChecked(a[1]))
    a_available = false;
  if (!line.ReadChecked(a[2]))
    a_available = false;

  if (a_available)
    info.acceleration.ProvideGLoad(SpaceDiagonal(a[0], a[1], a[2]));

  double vario;
  if (line.ReadChecked(vario))
    info.ProvideNettoVario(vario);

  double ias;
  bool have_ias = line.ReadChecked(ias);

  double altitude;
  if (line.ReadChecked(altitude)) {
    info.ProvidePressureAltitude(altitude);

    if (have_ias)
      info.ProvideIndicatedAirspeedWithAltitude(ias, altitude);
  }

  return true;
}

/**
 * Parse the $PLXVS sentence (LXNAV sVarios (including V7)).
 *
 * $PLXVS,OAT,mode,voltage *CS<CR><LF>
 *
 * Example: $PLXVS,23.1,0,12.3,*CS<CR><LF>
 *
 * @see http://www.xcsoar.org/trac/raw-attachment/ticket/1666/V7%20dataport%20specification%201.97.pdf
 */
static bool
PLXVS(NMEAInputLine &line, NMEAInfo &info)
{
  double temperature;
  if (line.ReadChecked(temperature)) {
    info.temperature = Temperature::FromCelsius(temperature);
    info.temperature_available.Update(info.clock);
  }

  int mode;
  info.switch_state.flight_mode = SwitchState::FlightMode::UNKNOWN;
  if (line.ReadChecked(mode)) {
    if (mode == 0)
      info.switch_state.flight_mode = SwitchState::FlightMode::CIRCLING;
    else if (mode == 1)
      info.switch_state.flight_mode = SwitchState::FlightMode::CRUISE;
  }

  double voltage;
  if (line.ReadChecked(voltage)) {
    info.voltage = voltage;
    info.voltage_available.Update(info.clock);
  }

  /* IGC press altitude field - pressure altitude used by device for IGC recording */
  double igc_press_altitude;
  if (line.ReadChecked(igc_press_altitude)) {
    info.igc_pressure_altitude = igc_press_altitude;
    info.igc_pressure_altitude_available.Update(info.clock);
  }

  /* Parse FlapPosition field (added in protocol v1.03) */
  const auto flap_str = line.ReadView();
  if (!flap_str.empty()) {
    if (flap_str == "L"sv)
      info.switch_state.flap_position = SwitchState::FlapPosition::LANDING;
    else
      /* Other flap position values not yet defined in protocol documentation */
      info.switch_state.flap_position = SwitchState::FlapPosition::UNKNOWN;
  }

  /* VP (Vario Priority) flag is available but not currently used */
  line.Skip();

  return true;
}

void
LXDevice::IdDeviceByNameLocked(const StaticString<16> &product_name,
                               const DeviceInfo &device_info) noexcept
{
  const bool new_v7 = product_name.equals("V7");
  const bool new_sVario = product_name.equals("NINC") ||
                           product_name.equals("S8x");
  const bool new_nano = IsNanoProduct(product_name);
  const bool new_lx16xx = product_name.equals("1606") ||
                           product_name.equals("1600");

  if ((new_v7 && !is_v7) || (new_sVario && !is_sVario)) {
    const char *device_type = new_v7 ? "V7" : "S series vario";
    LogFmt("LXNAV: {} detected via PLXVC (product: {}, firmware: {})",
           device_type, product_name.c_str(),
           device_info.software_version.empty()
             ? "unknown"
             : device_info.software_version.c_str());
    if (!device_info.software_version.empty())
      firmware_version_logged = true;
  }

  is_v7 = new_v7;
  is_sVario = new_sVario;
  is_nano = new_nano;
  is_lx16xx = new_lx16xx;
}

void
LXDevice::IdDeviceByName(const StaticString<16> &product_name,
                         const DeviceInfo &device_info) noexcept
{
  const std::lock_guard lock{mutex};
  IdDeviceByNameLocked(product_name, device_info);
}

void
LXDevice::UpdateDeviceFlags(const DeviceInfo &device_info,
                            bool pass_through) noexcept
{
  const bool saw_v7 = device_info.product.equals("V7");
  const bool saw_sVario = device_info.product.equals("NINC") ||
                           device_info.product.equals("S8x");
  const bool saw_nano = IsNanoProduct(device_info.product);
  const bool saw_lx16xx = device_info.product.equals("1606") ||
                           device_info.product.equals("1600");

  const bool was_vario = IsLXNAVVario();

  {
    const std::lock_guard lock{mutex};
    if (pass_through) {
      /* in pass-through mode, never clear flags — the primary
         device is still there even though it's "hidden" */
      is_v7 |= saw_v7;
      is_sVario |= saw_sVario;
      is_nano |= saw_nano;
      is_lx16xx |= saw_lx16xx;
      is_forwarded_nano = saw_nano;
    } else {
      is_v7 = saw_v7;
      is_sVario = saw_sVario;
      is_nano = saw_nano;
      is_lx16xx = saw_lx16xx;
    }

    if (saw_v7 || saw_sVario || saw_nano || saw_lx16xx)
      is_colibri = false;

    if (!was_vario && (saw_v7 || saw_sVario))
      vario_just_detected = true;
  }
}

bool
LXDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$LXWP0"sv)
    return LXWP0(line, info);

  if (type == "$LXWP1"sv) {
    DeviceInfo &device_info = mode == Mode::PASS_THROUGH
      ? info.secondary_device
      : info.device;
    LXWP1(line, device_info);
    UpdateDeviceFlags(device_info, mode == Mode::PASS_THROUGH);
    return true;
  }

  if (type == "$LXWP2"sv)
    return LXWP2(line, info);

  if (type == "$LXWP3"sv)
    return LXWP3(line, info);

  if (type == "$PLXV0"sv) {
    is_colibri = false;
    return PLXV0(line, lxnav_vario_settings, info);
  }

  if (type == "$PLXVC"sv) {
    is_colibri = false;
    PLXVC(line, info, nano_settings, device_declaration, mutex);

    {
      const std::lock_guard lock{mutex};
      is_forwarded_nano =
        IsNanoProduct(info.secondary_device.product);
      const bool was_vario = IsLXNAVVario();
      IdDeviceByNameLocked(info.device.product, info.device);
      if (!was_vario && IsLXNAVVario())
        vario_just_detected = true;
    }
    return true;
  }

  if (type == "$PLXVF"sv) {
    is_colibri = false;
    return PLXVF(line, info);
  }

  if (type == "$PLXVS"sv) {
    is_colibri = false;
    return PLXVS(line, info);
  }

  return false;
}
