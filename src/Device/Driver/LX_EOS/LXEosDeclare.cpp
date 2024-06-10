// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Error.hpp"
#include "LXEosDevice.hpp"
#include "util/CRC8.hpp"

bool
LXEosDevice::Declare(const Declaration &declaration,
                     [[maybe_unused]] const Waypoint *home,
                     OperationEnvironment &env)
{
  // Declaration can have maximum 12 items, including takeoff and landing
  if (declaration.Size() < 2 || declaration.Size() > 10) return false;

  port.StopRxThread();

  env.SetProgressRange(declaration.Size() + 2);

  try {
    SendDeclaration(declaration, home, env);

    env.SetProgressPosition(1);

    for (unsigned int tp = 1; tp <= declaration.Size(); tp++) {
      SendObsZone(tp, declaration, env);
      env.SetProgressPosition(tp + 1);
    }

    SendCompetitionClass(declaration, env);

    env.SetProgressPosition(declaration.Size() + 2);
    port.StartRxThread();
    return true;
  } catch (...) {
    return false;
  }
}

void
LXEosDevice::SendDeclaration(const Declaration &declaration,
                             [[maybe_unused]] const Waypoint *home,
                             OperationEnvironment &env)
{
  EosDeclarationStruct data;

  CopyStringSpacePadded(data.pilot, declaration.pilot_name,
                        sizeof(data.pilot));
  CopyStringSpacePadded(data.glider, declaration.aircraft_type,
                        sizeof(data.glider));
  CopyStringSpacePadded(data.reg_num, declaration.aircraft_registration,
                        sizeof(data.reg_num));
  CopyStringSpacePadded(data.cmp_num, declaration.competition_id,
                        sizeof(data.cmp_num));

  // Set class to standard (there is no way to get the correct class)
  data.byClass = 0;

  memset(data.observer, 0x00, sizeof(data.observer));
  memset(data.gps, 0x00, sizeof(data.gps));

  // Number of turnpoints (excluding start and finish)
  data.num_of_tp = declaration.Size() - 2;

  // Takeoff point (set to all zeroes, as it is not useful)
  data.prg[0] = 3;
  data.lat[0] = 0;
  data.lon[0] = 0;
  CopyStringSpacePadded(data.name[0], _T("TAKEOFF"), 9);

  for (uint8_t i = 1; i <= declaration.Size(); i++) {
    data.prg[i] = 1;
    data.lat[i] = ConvertCoord(declaration.GetLocation(i - 1).latitude);
    data.lon[i] = ConvertCoord(declaration.GetLocation(i - 1).longitude);
    CopyStringSpacePadded(data.name[i], declaration.GetName(i - 1), 9);
  }

  /*
   * Setting the landing seems to not work.
   * Eos will either show name of waypoint that used to be on that position in
   * list before declaration, or "NO_NAME". It is more likely a bug in the Eos
   * firmware than in the structure of the command. It seems to not cause any
   * practical issues, so let's ignore it.
   */
  data.prg[declaration.Size() + 1] = 2;
  data.lat[declaration.Size() + 1] = 0;
  data.lon[declaration.Size() + 1] = 0;
  CopyStringSpacePadded(data.name[declaration.Size() + 1], _T("LANDING"), 9);

  // Set the remaining entries to zeroes
  for (uint8_t i = declaration.Size() + 2; i < 12; i++) {
    data.prg[i] = 0;
    data.lat[i] = 0;
    data.lon[i] = 0;
    memset(data.name[i], 0x00, 9);
  }

  /* CRC is calculated only from data bytes (excluding the SYN and CMD)
  Using poly 0x69 and initial 0xFF */
  auto data_bytes_pointer = reinterpret_cast<std::byte *>(&data);
  data.crc = UpdateCRC8(std::span{data_bytes_pointer + 2, sizeof(data) - 3},
                        std::byte{0xFF});
  WriteAndWaitForACK(std::span{data_bytes_pointer, sizeof(data)}, env);
}

void
LXEosDevice::SendObsZone(uint8_t tp_nr, const Declaration &declaration,
                         OperationEnvironment &env)
{
  if (tp_nr < 1 || tp_nr > declaration.Size())
    throw std::runtime_error("Invalid TP number");

  EosObsZoneStruct data;
  const auto &turnpoint = declaration.turnpoints.at(tp_nr - 1);

  data.uiTpNr = tp_nr;
  data.uiR1 = turnpoint.radius;

  if (tp_nr == 1)                       // Start
    data.uiDirection = 2;               // Next
  else if (tp_nr == declaration.Size()) // Finish
    data.uiDirection = 3;               // Previous
  else                                  // Turnpoint
    data.uiDirection = 0;               // Symmetric

  data.bIsLine = (turnpoint.shape == Declaration::TurnPoint::LINE) ? 1 : 0;

  switch (turnpoint.shape) {
  case Declaration::TurnPoint::LINE:
    data.fA1 = DEG_TO_RAD * 90;
    break;
  case Declaration::TurnPoint::CYLINDER:
    data.fA1 = DEG_TO_RAD * 180;
    break;
  case Declaration::TurnPoint::DAEC_KEYHOLE:
    data.fA1 = DEG_TO_RAD * 45;
    data.fA2 = DEG_TO_RAD * 180;
    data.uiR2 = 500;
    break;
  case Declaration::TurnPoint::SECTOR:
  default:
    data.fA1 = DEG_TO_RAD * 45;
  }

  data.fElevation = declaration.GetWaypoint(tp_nr - 1).GetElevationOrZero();

  /* CRC is calculated from all bytes (including the SYN and CMD)
  Using poly 0x69 and initial 0x44 */
  data.crc = UpdateCRC8(
      std::span{reinterpret_cast<std::byte *>(&data), sizeof(data) - 1},
      std::byte{0x44});
  return WriteAndWaitForACK(
      std::span{reinterpret_cast<std::byte *>(&data), sizeof(data)}, env);
}

void
LXEosDevice::SendCompetitionClass(
    [[maybe_unused]] const Declaration &declaration, OperationEnvironment &env)
{
  EosClassStruct data;

  // Declaration does not provide class information, set empty
  CopyStringSpacePadded(data.name, _T(""), sizeof(data.name));

  /* CRC is calculated only from data bytes (excluding the SYN and CMD)
  Using poly 0x69 and initial 0xFF */
  auto data_bytes_pointer = reinterpret_cast<std::byte *>(&data);
  data.crc = UpdateCRC8(std::span{data_bytes_pointer + 2, sizeof(data) - 3},
                        std::byte{0xFF});

  return WriteAndWaitForACK(std::span{data_bytes_pointer, sizeof(data)}, env);
}
