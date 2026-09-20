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
 * UDP.ini is the analog panel / Simkits stream.  Nav (GPS, baro, TAS,
 * heading, wind, TE climb) comes from Condor NMEA (`$GPRMC`/`$GPGGA` +
 * `$LXWP0`).  Cockpit keys that duplicate or contradict that stream are
 * ignored: airspeed, altitude (altimeter QNH/QFE), evario (SC/Vario
 * needle), compass, yaw, vx, vy, and optional latitude/longitude.
 *
 * Mapped parameters: vario, nettovario, pitch, bank, rollrate, pitchrate,
 * yawrate, gforce, radiofrequency (MHz → active radio), MC, water (**).
 *
 * Do not set gps.simulator: that flag is the built-in Simulator overlay.
 * Condor NMEA already clears gps.real via parser.SetReal(), which is the
 * SkyLines / Cloud / LiveTrack24 gate.
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
  double roll_rate = 0, pitch_rate = 0, yaw_rate = 0;

  void
  PushGyro(NMEAInfo &info) noexcept {
    info.gyroscope.ProvideAngularRates(
      Angle::Radians(roll_rate), Angle::Radians(pitch_rate),
      Angle::Radians(yaw_rate), true, true);
  }

  bool
  ApplyKey(std::string_view key, double value, NMEAInfo &info) noexcept {
    if (StringIsEqualIgnoreCase(key, "vario"sv)) {
      info.ProvideNoncompVario(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "nettovario"sv)) {
      info.ProvideNettoVario(value);
      return true;
    }

    if (StringIsEqualIgnoreCase(key, "bank"sv)) {
      /* Condor bank sign is opposite XCSoar (positive = right wing down). */
      info.attitude.bank_angle = Angle::Radians(-value);
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

    return false;
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
