// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StaticParser.hpp"
#include "FLARM/Error.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/Version.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "NMEA/InputLine.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"

using std::string_view_literals::operator""sv;

void
ParsePFLAE(NMEAInputLine &line, FlarmError &error, TimeStamp clock) noexcept
{
  const auto type = line.ReadView();
  if (type != "A"sv)
    return;

  error.severity = (FlarmError::Severity)
    line.Read((int)FlarmError::Severity::NO_ERROR);
  error.code = (FlarmError::Code)line.ReadHex(0);
  TCHAR buffer[100];
  StringFormatUnsafe(buffer, _T("%s - %s"),
                     FlarmError::ToString(error.severity),
                     FlarmError::ToString(error.code));
  if (error.severity != FlarmError::Severity::NO_ERROR)
    Message::AddMessage(_T("FLARM: "), buffer);

  error.available.Update(clock);
}

void
ParsePFLAV(NMEAInputLine &line, FlarmVersion &version,
           TimeStamp clock) noexcept
{
  const auto type = line.ReadView();
  if (type != "A"sv)
    return;

  version.hardware_version = line.ReadView();
  version.hardware_version.CleanASCII();

  version.software_version = line.ReadView();
  version.software_version.CleanASCII();

  version.obstacle_version = line.ReadView();
  version.obstacle_version.CleanASCII();

  version.available.Update(clock);
}

void
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, TimeStamp clock) noexcept
{
  flarm.available.Update(clock);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.Read(0);
  flarm.tx = line.Read(false);
  flarm.gps = (FlarmStatus::GPSStatus)
    line.Read((int)FlarmStatus::GPSStatus::NONE);

  line.Skip();
  flarm.alarm_level = (FlarmTraffic::AlarmType)
    line.Read((int)FlarmTraffic::AlarmType::NONE);
}

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock) noexcept
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
