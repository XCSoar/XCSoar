// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StaticParser.hpp"
#include "FLARM/Error.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Details.hpp"
#include "FLARM/MessagingRecord.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "NMEA/InputLine.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "util/HexString.hpp"

#include <algorithm>
#include <fmt/format.h>

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

  if (error.severity != FlarmError::Severity::NO_ERROR) {
    const auto msg = fmt::format("{} - {}",
                                 FlarmError::ToString(error.severity),
                                 FlarmError::ToString(error.code));
    Message::AddMessage("FLARM: ", msg.c_str());
  }

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

  line.Skip(); /* Power */
  flarm.alarm_level = (FlarmTraffic::AlarmType)
    line.Read((int)FlarmTraffic::AlarmType::NONE);

  // Extended fields (6-10): present when alarm_level > 0
  int bearing;
  if (line.ReadChecked(bearing)) {
    flarm.relative_bearing = (int16_t)bearing;
    flarm.alarm_type = (uint8_t)line.Read(0);

    int vert;
    if (line.ReadChecked(vert))
      flarm.relative_vertical = vert;
    else
      flarm.relative_vertical = 0;

    int dist;
    if (line.ReadChecked(dist))
      flarm.relative_distance = (uint32_t)dist;
    else
      flarm.relative_distance = 0;

    char id_string[16];
    line.Read(id_string, 16);
    flarm.target_id = FlarmId::Parse(id_string, nullptr);

    flarm.has_extended = true;
  } else {
    flarm.has_extended = false;
    flarm.relative_bearing = 0;
    flarm.alarm_type = 0;
    flarm.relative_vertical = 0;
    flarm.relative_distance = 0;
    flarm.target_id.Clear();
  }
}

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock, RangeFilter &range) noexcept
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

  if (line.ReadChecked(value))
    // Relative East
    traffic.relative_east = value;
  else
    // No position target
    traffic.relative_east = 0;

  if (!line.ReadChecked(value))
    // Relative Altitude is required !
    return;
  traffic.relative_altitude = value;

  if (range.horizontal && range.vertical) {
    // object outside cylinder; non filtered data only !
    if ((hypot(traffic.relative_north, traffic.relative_east) > (RoughDistance)range.horizontal) ||
    (abs((int)traffic.relative_altitude) > range.vertical))
      return;
  }

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

  // PFLAA v7+ optional fields: Source, RSSI, NoTrack
  int source_val;
  if (line.ReadChecked(source_val)) {
    switch (source_val) {
    case 0: case 2: case 3: case 4: case 5:
      traffic.source = (FlarmTraffic::SourceType)source_val;
      break;
    default:
      traffic.source = FlarmTraffic::SourceType::FLARM;
      break;
    }

    int rssi_val;
    if (line.ReadChecked(rssi_val)) {
      traffic.rssi = (int8_t)rssi_val;
      traffic.rssi_available = true;
    } else {
      traffic.rssi = 0;
      traffic.rssi_available = false;
    }

    int no_track_val;
    traffic.no_track = line.ReadChecked(no_track_val) && no_track_val != 0;
  } else {
    traffic.source = FlarmTraffic::SourceType::FLARM;
    traffic.rssi = 0;
    traffic.rssi_available = false;
    traffic.no_track = false;
  }

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

void
ParsePFLAM(NMEAInputLine &line) noexcept
{
  // PFLAM,<Type>,<IDType>,<ID>,<MsgType>,<Payload>
  // Spec (FTD-109 v1.00, 2024-12): text payloads are hex-encoded, max 17 chars
  // -> 34 hex digits. 
  // VHF payload is plain ASCII, may be comma-separated; we parse only the first 
  // token and cap it at 8 chars (e.g. 123.450).
  const auto type = line.ReadView();

  if (type == "U"sv) {
    MessagingRecord record{};

    line.Skip(); /* id type */

    // 5 id, 6 digit hex
    char id_string[16];
    line.Read(id_string, 16);
    record.id = FlarmId::Parse(id_string, nullptr);

    const auto msg_type = line.ReadView();
    const auto hex_value = line.ReadView();

    constexpr std::size_t MAX_HEX_PAYLOAD = 34; // 17 chars payload encoded as hex pairs
    constexpr std::size_t MAX_VHF_LENGTH = 8;   // e.g. "123.450"

    const bool is_hex_payload = msg_type == "AREG"sv || msg_type == "PNAME"sv ||
                                msg_type == "ATYPE"sv || msg_type == "ACALL"sv;
    if (is_hex_payload && hex_value.size() > MAX_HEX_PAYLOAD)
      return;

    std::string_view first_freq;
    if (msg_type == "VHF"sv) {
      // VHF message contains comma-separated frequencies (not hex-encoded)
      // Only parse the first frequency from the comma-separated list
      const auto comma = std::find(hex_value.begin(), hex_value.end(), ',');
      first_freq = std::string_view(hex_value.data(), comma - hex_value.begin());

      if (first_freq.size() > MAX_VHF_LENGTH)
        return;
    }

    try {
      if (msg_type == "AREG"sv) {
        record.registration = ParseHexString(hex_value);
      } else if (msg_type == "PNAME"sv) {
        record.pilot = ParseHexString(hex_value);
      } else if (msg_type == "ATYPE"sv) {
        record.plane_type = ParseHexString(hex_value);
      } else if (msg_type == "ACALL"sv) {
        record.callsign = ParseHexString(hex_value);
      } else if (msg_type == "VHF"sv) {
        record.frequency = RadioFrequency::Parse(first_freq);
      }
    } catch (...) {
      // Silently ignore malformed hex data in NMEA stream
      return;
    }

    FlarmDetails::StoreMessagingRecord(record);
  }
}
