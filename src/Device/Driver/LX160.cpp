// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/LX160.hpp"
#include "Device/Driver.hpp"
#include "Device/Config.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Driver/LX/Parsers.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Geo/GeoPoint.hpp"
#include "Formatter/NMEAFormatter.hpp"
#include "Operation/Operation.hpp"

#include <string_view>

using std::string_view_literals::operator""sv;

/*
 * Driver for the LX Navigation LX160 vario / final-glide computer.
 *
 * The LX160 has a single bidirectional NMEA port:
 *
 *   IN : $GPGGA + $GPRMC (+ optional $GPRMB)
 *   OUT: $LXWP0/$LXWP1/$LXWP2/$LXWP3
 *
 * Read side reuses the LXWPx parsers from src/Device/Driver/LX/Parser.cpp
 * (exposed via Parsers.hpp).
 *
 * Write side mirrors `cai_lnav`: $GPGGA + $GPRMC + $GPRMB are emitted on
 * each calculation tick (~1 Hz) using the shared NMEAFormatter helpers.
 *
 * The LX160 has no NMEA writeback for MC / ballast / bugs / polar (those
 * are physical panel switches), so this driver does not advertise
 * SEND_SETTINGS or DECLARE.
 */

class LX160Device final : public AbstractDevice {
  Port &port;
  /* Sampled from DeviceConfig at open time; reconnect required to change. */
  const bool send_position;

public:
  LX160Device(Port &_port, bool _send_position) noexcept
    : port(_port), send_position(_send_position) {}

  bool ParseNMEA(const char *line, NMEAInfo &info) override;

  /* No PutMacCready / PutBallast / PutBugs override: the LX160 has been
     bench-verified to silently ignore both PFLX2 (LX1600 7-field form
     and LX MiniMap 6-field form).  Ballast and bugs are physical front-
     panel switches, and the spring-loaded MC knob means MC sync is
     RECEIVE-only on this hardware. */

  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;
};

bool
LX160Device::ParseNMEA(const char *string, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(string))
    return false;

  NMEAInputLine line(string);
  const auto type = line.ReadView();

  if (type == "$LXWP0"sv)
    return LX::LXWP0(line, info);

  if (type == "$LXWP1"sv) {
    LX::LXWP1(line, info.device);
    return true;
  }

  if (type == "$LXWP2"sv)
    return LX::LXWP2(line, info);

  if (type == "$LXWP3"sv)
    return LX::LXWP3(line, info);

  /* The LX160 echoes any $GPGGA/$GPRMC it receives back onto its NMEA
     OUT line.  When XCSoar is the GPS source (send_position == true)
     those echoes are our own data bouncing back -- swallow them so
     they don't show up in the merge thread as a second GPS source.
     When send_position == false the user has wired an external GPS
     to the LX160 and the echoed sentences are the only path to GPS
     in XCSoar -- let the generic parser ingest them. */
  if (send_position &&
      (type == "$GPGGA"sv || type == "$GPRMC"sv))
    return true;

  return false;
}

void
LX160Device::OnCalculatedUpdate(const MoreData &basic,
                                const DerivedInfo &calculated)
{
  NullOperationEnvironment env;
  char buffer[100];

  if (send_position) {
    FormatGPGGA(buffer, sizeof(buffer), basic);
    PortWriteNMEA(port, buffer, env);

    FormatGPRMC(buffer, sizeof(buffer), basic);
    PortWriteNMEA(port, buffer, env);
  }

  const GeoPoint here = basic.location_available
    ? basic.location
    : GeoPoint::Invalid();

  const ElementStat &current_leg = calculated.task_stats.current_leg;
  const AGeoPoint destination(current_leg.location_remaining,
                              current_leg.solution_planned.min_arrival_altitude);

  if (FormatGPRMB(buffer, sizeof(buffer), here, destination))
    PortWriteNMEA(port, buffer, env);
}

static Device *
LX160CreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new LX160Device(com_port, config.send_position);
}

const struct DeviceRegister lx160_driver = {
  "LX160",
  "LX160",
  DeviceRegister::RECEIVE_SETTINGS | DeviceRegister::SEND_POSITION,
  LX160CreateOnPort,
};
