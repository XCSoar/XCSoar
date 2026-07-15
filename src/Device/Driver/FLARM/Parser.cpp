// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "FLARM/Id.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

#include <cstring>
#include <string>

using std::string_view_literals::operator""sv;

bool
FlarmDevice::ParsePFLAC(NMEAInputLine &line, NMEAInfo &info)
{
  [[maybe_unused]] const auto responsetype = line.ReadView();

  const auto name = line.ReadView();

  if (name == "ERROR"sv)
    // ignore error responses...
    return true;

  if (name == "RADIOID"sv) {
    /* Format: <IDType>,<ID> */
    const auto id_type = line.ReadView();
    const auto id_hex = line.ReadView();
    {
      std::string value;
      value.reserve(id_type.size() + 1 + id_hex.size());
      value.append(id_type);
      value.push_back(',');
      value.append(id_hex);

      const std::lock_guard<Mutex> lock(settings);
      settings.Set(std::string{name}, std::move(value));
    }

    FlarmId id = FlarmId::Undefined();
    if (!id_hex.empty() && id_hex.size() < 16) {
      char hex[16];
      memcpy(hex, id_hex.data(), id_hex.size());
      hex[id_hex.size()] = '\0';
      id = FlarmId::Parse(hex, nullptr);
    }

    /* Always update radio_id so a missing/invalid RADIOID does not
       leave a stale self-id in the blackboard. */
    info.flarm.hardware.radio_id = id;
    if (id.IsDefined())
      info.flarm.hardware.available.Update(info.clock);

    return true;
  }

  const auto value = line.Rest();

  const std::lock_guard<Mutex> lock(settings);
  settings.Set(std::string{name}, std::string{value});

  return true;
}

bool
FlarmDevice::ParseNMEA(const char *_line, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(_line))
    return false;

  NMEAInputLine line(_line);

  const auto type = line.ReadView();
  if (type == "$PFLAC"sv)
    return ParsePFLAC(line, info);
  else
    return false;
}
