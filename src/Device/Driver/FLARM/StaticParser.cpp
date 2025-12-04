// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StaticParser.hpp"
#include "FLARM/Error.hpp"
#include "FLARM/FlightState.hpp"
#include "FLARM/List.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/AlertZoneList.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "LogFile.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "NMEA/InputLine.hpp"
#include "util/Macros.hpp"
#include "util/NumberParser.hxx"
#include "util/StringAPI.hxx"

using std::string_view_literals::operator""sv;

void
ParsePFLAE(NMEAInputLine &line, FlarmError &error, FlarmVersion &version,
           TimeStamp clock) noexcept
{
  const auto type = line.ReadView();
  if (type != "A"sv)
    return;

  error.severity = (FlarmError::Severity)
    line.Read((int)FlarmError::Severity::NO_ERROR);
  error.code = (FlarmError::Code)line.ReadHex(0);

  if (!line.IsEmpty()) {
    error.message = line.ReadView();
    error.message.CleanASCII();
    if (!error.message.empty() && version.protocol_version < 7)
        version.protocol_version = 7;
    } else {
      error.message.clear();
  }

  TCHAR buffer[100];
  if (!error.message.empty()) {
    StringFormatUnsafe(buffer, _T("%s - %s: %s"),
                       FlarmError::ToString(error.severity),
                       FlarmError::ToString(error.code),
                       error.message.c_str());
  } else {
    StringFormatUnsafe(buffer, _T("%s - %s"),
                       FlarmError::ToString(error.severity),
                       FlarmError::ToString(error.code));
  }

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
ParsePFLAU(NMEAInputLine &line, FlarmStatus &flarm, FlarmVersion &version,
           TimeStamp clock) noexcept
{
  flarm.available.Update(clock);

  // PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,
  //   <RelativeVertical>,<RelativeDistance>(,<ID>)
  flarm.rx = line.Read(0);
  flarm.tx = line.Read(false);
  flarm.gps = (FlarmStatus::GPSStatus)
    line.Read((int)FlarmStatus::GPSStatus::NONE);

  line.Skip(); // Power
  const int alarm_level_raw = line.Read(0);
  line.Skip(); // RelativeBearing
  const uint8_t alarm_type = line.ReadHex(0);

  // Interpret AlarmLevel based on AlarmType
  // AlarmLevel=1 can mean: normal alarm (15-20s), Alert Zone, or traffic advisory (AlarmType=4)
  if (alarm_level_raw == 1 && alarm_type == 4) {
    // Traffic advisory: use INFO_ALERT level
    flarm.alarm_level = FlarmTraffic::AlarmType::INFO_ALERT;
  } else {
    // Map AlarmLevel 0-3 to enum (0=NONE, 1=LOW, 2=IMPORTANT, 3=URGENT)
    flarm.alarm_level = (FlarmTraffic::AlarmType)
      (alarm_level_raw <= 3 ? alarm_level_raw : 0);
  }

  line.Skip(); // RelativeVertical
  line.Skip(); // RelativeDistance

  if (!line.IsEmpty()) {
    line.Skip(); // ID
    if (version.protocol_version < 4)
      version.protocol_version = 4;
  }
}

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, FlarmVersion &version,
           TimeStamp clock, RangeFilter &range) noexcept
{
  flarm.modified.Update(clock);

  // PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,
  //   <IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
  FlarmTraffic traffic;
  // Source will be determined from actual received data, default to FLARM
  traffic.source = FlarmTraffic::Source::FLARM;
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

  line.Skip(); // IDType

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

  if (!line.IsEmpty()) {
    // NoTrack field: skip this target if NoTrack=1 (privacy setting)
    const unsigned notrack = line.Read(0);
    if (notrack == 1)
      return;
    if (version.protocol_version < 8)
      version.protocol_version = 8;
  }

  if (!line.IsEmpty()) {
    // Source field: 0=FLARM, 1=ADS-B, 3=ADS-R, 4=TIS-B, 6=Mode-S
    const unsigned source_raw = line.Read(255);
    switch (source_raw) {
    case 0:
      traffic.source = FlarmTraffic::Source::FLARM;
      break;
    case 1:
      traffic.source = FlarmTraffic::Source::ADS_B;
      break;
    case 3:
      traffic.source = FlarmTraffic::Source::ADS_R;
      break;
    case 4:
      traffic.source = FlarmTraffic::Source::TIS_B;
      break;
    case 6:
      traffic.source = FlarmTraffic::Source::MODE_S;
      break;
    default:
      traffic.source = FlarmTraffic::Source::UNKNOWN;
      break;
    }
    if (version.protocol_version < 9)
      version.protocol_version = 9;

    // RSSI field - not currently stored in FlarmTraffic
    if (!line.IsEmpty())
    line.Skip();
  } else {
    traffic.source = FlarmTraffic::Source::FLARM;
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
ParsePFLAO(NMEAInputLine &line, AlertZoneList &zones, FlarmVersion &version,
           TimeStamp clock) noexcept
{
  if (version.protocol_version < 7)
    version.protocol_version = 7;

  zones.modified.Update(clock);

  // PFLAO,<AlarmLevel>,<Inside>,<Latitude>,<Longitude>,<Radius>,
  //   <Bottom>,<Top>,<ActivityLimit>,<ID>,<ID-Type>,<ZoneType>

  FlarmAlertZone zone;
  zone.Clear();

  const int alarm_level_raw = line.Read(0);
  zone.alarm_level = (FlarmAlertZone::AlarmType)
    (alarm_level_raw <= 3 ? alarm_level_raw : 0);

  zone.inside = line.Read(0) != 0;

  // Latitude: degrees * 10^7
  int32_t lat_raw = line.Read(0);
  // Longitude: degrees * 10^7
  int32_t lon_raw = line.Read(0);

  // Convert to GeoPoint and validate using Check()
  zone.center = GeoPoint(
    Angle::Degrees(lon_raw / 10000000.0),
    Angle::Degrees(lat_raw / 10000000.0)
  );

  if (!zone.center.Check())
    return;

  // Radius: 0-2000 meters
  unsigned radius = line.Read(0);
  if (radius > 2000)
    return;
  zone.radius = radius;

  // Bottom: -1000 to 6000 meters above WGS84 ellipsoid
  int bottom = line.Read(0);
  if (bottom < -1000 || bottom > 6000)
    return;
  zone.bottom = bottom;

  // Top: 0 to 6000 meters above WGS84 ellipsoid
  int top = line.Read(0);
  if (top < 0 || top > 6000)
    return;
  zone.top = top;

  // ActivityLimit: seconds since epoch (0 = no end time)
  zone.activity_limit = line.Read(0);

  // ID: 6-digit hex
  char id_string[16];
  line.Read(id_string, 16);
  zone.id = FlarmId::Parse(id_string, nullptr);

  line.Skip(); // ID-Type

  // ZoneType: hex 10-FF
  unsigned zone_type_raw = line.ReadHex(0);
  if (zone_type_raw < 0x10 || zone_type_raw > 0xFF)
    return;

  // Map known zone types
  switch (zone_type_raw) {
  case 0x41:
    zone.zone_type = FlarmAlertZone::ZoneType::SKYDIVER_DROP_ZONE;
    break;
  case 0x42:
    zone.zone_type = FlarmAlertZone::ZoneType::AERODROME_TRAFFIC_ZONE;
    break;
  case 0x43:
    zone.zone_type = FlarmAlertZone::ZoneType::MILITARY_FIRING_AREA;
    break;
  case 0x44:
    zone.zone_type = FlarmAlertZone::ZoneType::KITE_FLYING_ZONE;
    break;
  case 0x45:
    zone.zone_type = FlarmAlertZone::ZoneType::WINCH_LAUNCHING_AREA;
    break;
  case 0x46:
    zone.zone_type = FlarmAlertZone::ZoneType::RC_FLYING_AREA;
    break;
  case 0x47:
    zone.zone_type = FlarmAlertZone::ZoneType::UAS_FLYING_AREA;
    break;
  case 0x48:
    zone.zone_type = FlarmAlertZone::ZoneType::AEROBATIC_BOX;
    break;
  case 0x7E:
    zone.zone_type = FlarmAlertZone::ZoneType::GENERIC_DANGER_AREA;
    break;
  case 0x7F:
    zone.zone_type = FlarmAlertZone::ZoneType::GENERIC_PROHIBITED_AREA;
    break;
  default:
    zone.zone_type = FlarmAlertZone::ZoneType::OTHER;
    break;
  }

  FlarmAlertZone *zone_slot = zones.FindZone(zone.id);

  if (zone_slot == nullptr) {
    zone_slot = zones.AllocateZone();
    if (zone_slot == nullptr)
      return;

    zone_slot->Clear();
    zone_slot->id = zone.id;
  }

  *zone_slot = zone;

  // set time of fix to current time (after updating zone data)
  zone_slot->valid.Update(clock);
}

void
ParsePFLAF(NMEAInputLine &line) noexcept
{
  /* PFLAF,<QueryType>[,<ScenarioNumber>|ERROR,<ErrorType>] */
  const auto query_type = line.ReadView();

  if (query_type == "A"sv) {
    /* Answer from FLARM */
    if (!line.IsEmpty()) {
      const auto next = line.ReadView();
      if (next == "ERROR"sv) {
        /* Error response: PFLAF,A,ERROR,<ErrorType> */
        const auto error_type = line.ReadView();
        if (error_type == "COMMAND"sv) {
          LogFormat("FLARM: Demo scenario command rejected - invalid command");
        } else if (error_type == "UNKNOWNSCENARIO"sv) {
          LogFormat("FLARM: Demo scenario command rejected - unknown scenario number");
        } else if (error_type == "INPROGRESS"sv) {
          LogFormat("FLARM: Demo scenario command rejected - scenario already running");
        } else if (error_type == "INFLIGHT"sv) {
          LogFormat("FLARM: Demo scenario command rejected - device is in flight");
        } else {
          LogFormat("FLARM: Demo scenario command rejected - error: %.*s",
                    (int)error_type.size(), error_type.data());
        }
      } else {
        /* Success response: PFLAF,A,<ScenarioNumber> */
        (void)ParseInteger<unsigned>(next);
      }
    }
  }
}

void
ParsePFLAJ(NMEAInputLine &line, FlarmFlightState &flight_state,
           FlarmVersion &version, TimeStamp clock) noexcept
{
  if (version.protocol_version < 8)
    version.protocol_version = 8;

  /* PFLAJ,<QueryType>,<FlightState>,<FlightRecorderState>,[<TisbAdsrClientStatus>] */
  const auto query_type = line.ReadView();

  if (query_type != "A"sv)
    return; /* Only process answer messages */

  /* FlightState: 0 = On ground, 1 = In flight */
  const unsigned flight_state_raw = line.Read(255);
  if (flight_state_raw <= 1) {
    flight_state.flight_state = (FlarmFlightState::FlightState)flight_state_raw;
  }

  /* FlightRecorderState: 0 = OFF, 1 = Recording, 2 = Barometric recording only */
  const unsigned recorder_state_raw = line.Read(255);
  if (recorder_state_raw <= 2) {
    flight_state.recorder_state = (FlarmFlightState::FlightRecorderState)recorder_state_raw;
  }

  /* TisbAdsrClientStatus (optional) - skip if present */
  line.Skip();

  flight_state.available.Update(clock);
}
