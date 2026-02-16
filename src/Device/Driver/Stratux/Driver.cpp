// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Driver.hpp"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
#include "Device/Driver/FLARM/StaticParser.hpp"
#include "Device/Driver/LevilAHRS_G.hpp"
#include "FLARM/Details.hpp"
#include "util/ConvertString.hpp"

StratuxDevice::StratuxSettings settings;

void
LoadFromProfile(StratuxDevice::StratuxSettings &settings) noexcept
{
  bool ok=false;

  ok |= Profile::Get(ProfileKeys::StratuxHorizontalRange,settings.hrange);
  ok |= Profile::Get(ProfileKeys::StratuxVerticalRange,settings.vrange);

  if (!ok) {
    settings.hrange = 20000;
    settings.vrange = 2000;
  }
}

void
SaveToProfile(StratuxDevice::StratuxSettings &settings) noexcept
{
    Profile::Set(ProfileKeys::StratuxHorizontalRange, settings.hrange);
    Profile::Set(ProfileKeys::StratuxVerticalRange, settings.vrange);
}

using std::string_view_literals::operator""sv;

bool
StratuxDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  NMEAInputLine input_line(line);

  const auto type = input_line.ReadView();
  if (type == "$RPYL"sv)
    return ParseRPYL(input_line, info);
  else if (type == "$APENV1"sv)
    return ParseAPENV1(input_line, info);
  else if (type == "$PFLAA"sv) {
    RangeFilter range;
    range.horizontal = settings.hrange;
    range.vertical = settings.vrange;
    ParsePFLAA(input_line, info.flarm.traffic, info.clock, range);
    ExtractAndSetCallSign(line, info);
    return true;
  } else
    return false;
}

void StratuxDevice::ExtractAndSetCallSign(const char *line, NMEAInfo &info)
{
  NMEAInputLine input_line(line);

/* Extract traffic-id field from PFLAA sentence.
   Skip 6 fields: $PFLAA, alarm-level, relative-north, relative-east,
   relative-vertical, id-type, and read the traffic-id field. */
  input_line.Skip(6);
  char id_string[16];
  input_line.Read(id_string, 16);

/* get the actual traffic list entry with the traffic-id */
  FlarmId id = FlarmId::Parse(id_string, nullptr);
  if (!id.IsDefined())
    return;

  FlarmTraffic *flarm_slot = info.flarm.traffic.FindTraffic(id);

/* Extract callsign from traffic-id field (after '!') if present. */
  if (flarm_slot) {
    char *ptr = strchr(id_string, '!');

    if (ptr && ptr[1] != '\0' && flarm_slot->name.empty()) {
      UTF8ToWideConverter callsign(ptr + 1);
      if (callsign.IsValid())
        flarm_slot->name.append(callsign.c_str());
    }

    /* Callsign from own list always overwrites. */
    const char *cs = FlarmDetails::LookupCallsign(id);
    if (cs != nullptr && cs[0] != 0) {
      flarm_slot->name.clear();
      flarm_slot->name.append(cs);
    }
  }
}

static Device *
StratuxCreateOnPort([[maybe_unused]] const DeviceConfig &config, [[maybe_unused]] Port &com_port)
{
  LoadFromProfile(settings);

  return new StratuxDevice();
}

const struct DeviceRegister stratux_driver = {
  _T("Stratux"),
  _T("Stratux"),
  DeviceRegister::MANAGE,
  StratuxCreateOnPort,
};
