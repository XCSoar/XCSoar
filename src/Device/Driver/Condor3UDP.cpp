// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Condor3UDP.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "Math/Angle.hpp"
#include "RadioFrequency.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"

#include <cmath>
#include <cstring>
#include <string_view>

using std::string_view_literals::operator""sv;

/**
 * Condor 3 generic UDP telemetry (Condor 3 manual, section 14 «Simkits and UDP
 * outputs», §14.1 «Generic UDP output», subsection «UDP Packet data», PDF
 * pages 72–73).
 *
 * The manual specifies: each UDP datagram is an ASCII stream of
 * `parameter=value` pairs (one pair per line in practice); all values are
 * floats with `.` as the decimal separator; SI units.  Parameters marked *
 * require `ExtendedData=1` in `UDP.ini`; ** require `ExtendedData1=1`.
 *
 * XCSoar receives this on a UDP listener port (`UDP.ini` default port is
 * often 55278).  Input is line-split like a serial stream, then each line is
 * parsed here (not NMEA `$…*CS` — the method name is the generic device hook).
 *
 * Implemented parameters from the manual table: airspeed, altitude, vario,
 * evario, nettovario, compass, yaw, pitch, bank, vx, vy, rollrate,
 * pitchrate, yawrate, gforce, radiofrequency (MHz → active radio), MC,
 * water (**).  Latitude/longitude are not in
 * the published UDP table; optional keys are accepted if Condor adds them.
 *
 * Polar sync (device dialog) is not offered for this driver; Condor UDP does
 * not carry full polar coefficients, and XCSoar does not send polars to Condor.
 *
 * Not mapped (no suitable `NMEAInfo` field or non-float): time, integrator,
 * slipball, turnrate, yawstringangle, quaternion*, ax/ay/az, height*,
 * wheelheight*, turbulencestrength*, surfaceroughness*, hudmessages*, flaps**
 * (integer).
 */
class Condor3UDPDevice final : public AbstractDevice {
  double vx = 0, vy = 0;
  bool have_vx = false, have_vy = false;

  double roll_rate = 0, pitch_rate = 0, yaw_rate = 0;

  bool have_latitude = false, have_longitude = false;

  void
  MarkSimulator(NMEAInfo &info) noexcept {
    info.gps.simulator = true;
    info.gps.real = false;
  }

  void
  UpdateGroundVelocity(NMEAInfo &info) noexcept {
    if (!have_vx || !have_vy)
      return;

    const double h = std::hypot(vx, vy);
    info.ground_speed = h;
    info.ground_speed_available.Update(info.clock);
    if (h > 0.2) {
      /* Assume velocity components in east/north axes (m/s). */
      info.track = Angle::Radians(std::atan2(vx, vy));
      info.track_available.Update(info.clock);
    }
  }

  bool
  ApplyKey(std::string_view key, double value, NMEAInfo &info) noexcept {
    MarkSimulator(info);

    if (StringIsEqualIgnoreCase(key, "airspeed"sv)) {
      /* Documented as true airspeed (m/s). */
      info.ProvideTrueAirspeed(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "altitude"sv)) {
      /* Altimeter / pressure altitude indication (m, SI in Condor 3). */
      info.ProvideBaroAltitudeTrue(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "vario"sv)) {
      info.ProvideNoncompVario(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "evario"sv)) {
      info.ProvideTotalEnergyVario(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "nettovario"sv)) {
      info.ProvideNettoVario(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "compass"sv)) {
      info.attitude.heading = Angle::Degrees(value);
      info.attitude.heading_available.Update(info.clock);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "bank"sv)) {
      info.attitude.bank_angle = Angle::Radians(value);
      info.attitude.bank_angle_available.Update(info.clock);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "pitch"sv)) {
      info.attitude.pitch_angle = Angle::Radians(value);
      info.attitude.pitch_angle_available.Update(info.clock);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "rollrate"sv)) {
      roll_rate = value;
      PushGyro(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "pitchrate"sv)) {
      pitch_rate = value;
      PushGyro(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "yawrate"sv)) {
      yaw_rate = value;
      PushGyro(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "vx"sv)) {
      vx = value;
      have_vx = true;
      UpdateGroundVelocity(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "vy"sv)) {
      vy = value;
      have_vy = true;
      UpdateGroundVelocity(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "gforce"sv)) {
      info.acceleration.ProvideGLoad(value, true);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "MC"sv)) {
      info.settings.ProvideMacCready(value, info.clock);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "radiofrequency"sv)) {
      /* Manual: MHz.  XCSoar uses integer kHz internally. */
      const unsigned khz = (unsigned)std::lround(value * 1000.0);
      const RadioFrequency freq = RadioFrequency::FromKiloHertz(khz);
      if (freq.IsDefined()) {
        info.settings.active_frequency = freq;
        info.settings.has_active_frequency.Update(info.clock);
      }
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "water"sv)) {
      /* ExtendedData1: water ballast [kg] per manual. */
      info.settings.ProvideBallastLitres(value, info.clock);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "yaw"sv)) {
      /* Heading from body yaw [rad]; compass [deg] preferred when both sent. */
      if (!info.attitude.heading_available) {
        info.attitude.heading = Angle::Radians(value);
        info.attitude.heading_available.Update(info.clock);
      }
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "latitude"sv)) {
      info.location.latitude = Angle::Degrees(value);
      have_latitude = true;
      FinishLocationIfComplete(info);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "longitude"sv)) {
      info.location.longitude = Angle::Degrees(value);
      have_longitude = true;
      FinishLocationIfComplete(info);
      return true;
    }

    return false;
  }

  void
  PushGyro(NMEAInfo &info) noexcept {
    info.gyroscope.ProvideAngularRates(
      Angle::Radians(roll_rate), Angle::Radians(pitch_rate),
      Angle::Radians(yaw_rate), true, true);
  }

  void
  FinishLocationIfComplete(NMEAInfo &info) noexcept {
    if (!have_latitude || !have_longitude)
      return;

    if (!info.location.Check())
      return;

    info.location.Normalize();
    info.location_available.Update(info.clock);
    info.gps.fix_quality = FixQuality::SIMULATION;
    info.gps.fix_quality_available.Update(info.clock);
    MarkSimulator(info);
  }

public:
  bool
  ParseNMEA(const char *line, NMEAInfo &info) override {
    if (line == nullptr || *line == '\0')
      return false;

    const char *eq = std::strchr(line, '=');
    if (eq == nullptr || eq == line)
      return false;

    std::string_view key = StripLeft(std::string_view(line, eq - line));
    key = StripRight(key);
    if (key.empty())
      return false;

    const char *vbegin = StripLeft(eq + 1);
    char *endptr = nullptr;
    const double value = ParseDouble(vbegin, &endptr);
    if (endptr == vbegin)
      return false;

    if (*StripLeft(endptr) != '\0')
      return false;

    return ApplyKey(key, value, info);
  }
};

static Device *
Condor3UDPCreateOnPort([[maybe_unused]] const DeviceConfig &config,
                       [[maybe_unused]] Port &com_port) {
  return new Condor3UDPDevice();
}

const struct DeviceRegister condor3_udp_driver = {
  "Condor3UDP",
  "Condor Soaring Simulator 3 (UDP telemetry)",
  DeviceRegister::RECEIVE_SETTINGS,
  Condor3UDPCreateOnPort,
};
