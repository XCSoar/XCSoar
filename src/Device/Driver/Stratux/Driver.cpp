// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Driver.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Interface.hpp"
#include "Profile/Profile.hpp"

#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"

StratuxDevice::StratuxSettings settings;

using std::string_view_literals::operator""sv;

void
StratuxDevice::ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock) noexcept
{
  flarm.modified.Update(clock);

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FlarmTraffic traffic;
  traffic.alarm_level = (FlarmTraffic::AlarmType)
    line.Read((int)FlarmTraffic::AlarmType::NONE);

  double value;
  bool stealth = false;

  if (!line.ReadChecked(value))
    // Relative North is required !
    return;
  traffic.relative_north = value;

  if (!line.ReadChecked(value))
    // Relative East is required !
    return;
  traffic.relative_east = value;

  if (!line.ReadChecked(value))
    // Relative Altitude is required !
    return;
  traffic.relative_altitude = value;

  if ((hypot(traffic.relative_north, traffic.relative_east) > (RoughDistance)settings.hrange) ||
    (abs((int)traffic.relative_altitude) > settings.vrange))
    // object outside cylinder (stratux devive settings)
    return;

  line.Skip(); /* id type */

  // 5 id, 6 digit hex
  char id_string[16];
  line.Read(id_string, 16);
  traffic.id = FlarmId::Parse(id_string, nullptr);

  Angle track;
  traffic.track_received = line.ReadBearing(track);
  if (!traffic.track_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.track = Angle::Zero();
  } else
    traffic.track = track;

  traffic.turn_rate_received = line.ReadChecked(value);
  if (!traffic.turn_rate_received) {
    // Field is empty in stealth mode
    traffic.turn_rate = 0;
  } else
    traffic.turn_rate = value;

  traffic.speed_received = line.ReadChecked(value);
  if (!traffic.speed_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.speed = 0;
  } else
    traffic.speed = value;

  traffic.climb_rate_received = line.ReadChecked(value);
  if (!traffic.climb_rate_received) {
    // Field is empty in stealth mode
    stealth = true;
    traffic.climb_rate = 0;
  } else
    traffic.climb_rate = value;

  traffic.stealth = stealth;

  unsigned type = line.ReadHex(0);
  if (type > 15 || type == 14)
    traffic.type = FlarmTraffic::AircraftType::UNKNOWN;
  else
    traffic.type = (FlarmTraffic::AircraftType)type;

  FlarmTraffic *flarm_slot = flarm.FindTraffic(traffic.id);
  if (flarm_slot == nullptr) {
    flarm_slot = flarm.AllocateTraffic();
    if (flarm_slot == nullptr)
      // no more slots available
      return;

    flarm_slot->Clear();
    flarm_slot->id = traffic.id;

    flarm.new_traffic.Update(clock);
  }

  // set time of fix to current time
  flarm_slot->valid.Update(clock);

  flarm_slot->Update(traffic);
}

bool
StratuxDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  NMEAInputLine input_line(line);

  const auto type = input_line.ReadView();
  if (type == "$PFLAA"sv) {
    ParsePFLAA(input_line, info.flarm.traffic, info.clock);
    return true;
  } else
    return false;
}

static Device *
StratuxCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  settings.hrange = std::stoi(Profile::Get(ProfileKeys::StratuxHorizontalRange));
  settings.vrange = std::stoi(Profile::Get(ProfileKeys::StratuxVerticalRange));

  return new StratuxDevice();
}

const struct DeviceRegister stratux_driver = {
  _T("Stratux"),
  _T("Stratux"),
  DeviceRegister::MANAGE,
  StratuxCreateOnPort,
};
