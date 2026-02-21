// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NanoDeclare.hpp"
#include "LXNavDeclare.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Device/Declaration.hpp"
#include "IGC/Generator.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/TimeoutClock.hpp"
#include "time/BrokenDateTime.hpp"
#include "Operation/Operation.hpp"

static bool
NanoWriteDecl(Port &port, OperationEnvironment &env, PortNMEAReader &reader,
              unsigned row, unsigned n_rows,
              const char *content)
{
  StaticString<256> buffer;
  buffer.Format("$PLXVC,DECL,W,%u,%u,%s", row, n_rows, content);

  PortWriteNMEA(port, buffer, env);

  buffer.UnsafeFormat("PLXVC,DECL,C,%u", row);
  char *response = reader.ExpectLine(buffer,
                                     TimeoutClock(std::chrono::seconds(2)));
  return response != nullptr && (*response == 0 || *response == ',');
}

template<typename... Args>
static bool
NanoWriteDeclFormat(Port &port, OperationEnvironment &env,
                    PortNMEAReader &reader,
                    unsigned row, unsigned n_rows,
                    const char *fmt, Args&&... args)
{
  StaticString<256> buffer;
  buffer.Format(fmt, args...);
  return NanoWriteDecl(port, env, reader, row, n_rows, buffer);
}

template<typename... Args>
static bool
NanoWriteDeclString(Port &port, OperationEnvironment &env,
                    PortNMEAReader &reader,
                    unsigned row, unsigned n_rows,
                    const char *prefix, const char *value)
{
  if (!value)
    return false;

  return NanoWriteDeclFormat(port, env, reader, row, n_rows,
                             "%s%s", prefix, value);
}

static bool
NanoWriteDeclMeta(Port &port, OperationEnvironment &env,
                  PortNMEAReader &reader,
                  const Declaration &declaration, unsigned total_size)
{
  return NanoWriteDeclString(port, env, reader, 1, total_size,
                             "HFPLTPILOT:", declaration.pilot_name) &&
    NanoWriteDeclString(port, env, reader, 2, total_size,
                        "HFCM2CREW2:", declaration.copilot_name) &&
    NanoWriteDeclString(port, env, reader, 3, total_size,
                        "HFGTYGLIDERTYPE:", declaration.aircraft_type) &&
    NanoWriteDeclString(port, env, reader, 4, total_size,
                        "HFGIDGLIDERID:", declaration.aircraft_registration) &&
    NanoWriteDeclString(port, env, reader, 5, total_size,
                        "HFCIDCOMPETITIONID:", declaration.competition_id) &&
    NanoWriteDecl(port, env, reader, 6, total_size, "HFCCLCOMPETITIONCLASS:");
}

static bool
NanoWriteStartDeclaration(Port &port, OperationEnvironment &env,
                          PortNMEAReader &reader,
                          const Declaration &declaration,
                          unsigned total_size)
{
  // TODO: use GPS clock instead?
  const BrokenDateTime date_time = BrokenDateTime::NowUTC();

  char buffer[64];
  FormatIGCTaskTimestamp(buffer, date_time, declaration.Size());
  return NanoWriteDecl(port, env, reader, 7, total_size, buffer);
}

static bool
NanoWriteTakeoff(Port &port, OperationEnvironment &env,
                 PortNMEAReader &reader, unsigned total_size)
{
  return NanoWriteDecl(port, env, reader, 8, total_size, IGCMakeTaskTakeoff());
}

static bool
NanoBeginDeclaration(Port &port, OperationEnvironment &env,
                     PortNMEAReader &reader,
                     const Declaration &declaration, unsigned total_size)
{
  return NanoWriteDeclMeta(port, env, reader, declaration, total_size) &&
    NanoWriteStartDeclaration(port, env, reader, declaration, total_size) &&
    NanoWriteTakeoff(port, env, reader, total_size);
}

static bool
NanoWriteLanding(Port &port, OperationEnvironment &env,
                 PortNMEAReader &reader, unsigned row, unsigned total_size)
{
  return NanoWriteDecl(port, env, reader, row, total_size,
                       IGCMakeTaskLanding());
}

static bool
NanoWriteTurnPoint(Port &port, OperationEnvironment &env,
                   PortNMEAReader &reader, unsigned row,
                   unsigned total_size,
                   const Declaration::TurnPoint &tp)
{
  const auto content = LXNavDeclare::FormatTurnPointCRecord(tp);
  return NanoWriteDecl(port, env, reader, row, total_size, content.c_str());
}

static bool
NanoWriteOZ(Port &port, OperationEnvironment &env, PortNMEAReader &reader,
            unsigned row, unsigned total_size,
            const Declaration &declaration,
            unsigned tp_index)
{
  const auto line = LXNavDeclare::FormatOZLine(declaration, tp_index);
  return NanoWriteDecl(port, env, reader, row, total_size, line.c_str());
}

/**
 * Write the LLXVTSK task options line.
 */
static bool
NanoWriteTaskOptions(Port &port, OperationEnvironment &env,
                     PortNMEAReader &reader, unsigned row,
                     unsigned total_size,
                     const Declaration &declaration)
{
  StaticString<128> content;
  content = "LLXVTSK";

  if (declaration.is_aat_task && declaration.aat_min_time.count() > 0)
    content.AppendFormat(",TaskTime=%us",
                         declaration.aat_min_time.count());

  content += ",StartOnEntry=false,Short=false,Near=true";

  return NanoWriteDecl(port, env, reader, row, total_size, content);
}

bool
Nano::Declare(Port &port, const Declaration &declaration,
              OperationEnvironment &env)
{
  const unsigned task_size = declaration.Size();

  /*
   * Declaration file layout:
   *   Rows 1-6:          H-records (pilot, glider, etc.)
   *   Row 7:             C-record timestamp
   *   Row 8:             C-record takeoff
   *   Rows 9..8+N:       C-record turnpoints (with elevation)
   *   Row 9+N:           C-record landing
   *   Rows 10+N..9+2N:   LLXVOZ observation zone lines
   *   Row 10+2N:         LLXVTSK task options
   */
  constexpr unsigned prefix_size = 8;
  const unsigned landing_row = prefix_size + task_size + 1;
  const unsigned oz_start_row = landing_row + 1;
  const unsigned task_options_row = oz_start_row + task_size;
  const unsigned total_size = task_options_row;

  env.SetProgressRange(total_size);

  port.StopRxThread();
  PortNMEAReader reader(port, env);

  if (!NanoBeginDeclaration(port, env, reader, declaration, total_size))
    return false;

  /* Write C-record turnpoints with elevation */
  unsigned row = prefix_size + 1;
  for (const auto &tp : declaration.turnpoints) {
    if (!NanoWriteTurnPoint(port, env, reader, row++, total_size, tp))
      return false;
  }

  /* Write C-record landing */
  if (!NanoWriteLanding(port, env, reader, row++, total_size))
    return false;

  assert(row == oz_start_row);

  /* Write LLXVOZ observation zone lines */
  for (unsigned i = 0; i < task_size; i++) {
    if (!NanoWriteOZ(port, env, reader, row++, total_size,
                     declaration, i))
      return false;
  }

  assert(row == task_options_row);

  /* Write LLXVTSK task options */
  return NanoWriteTaskOptions(port, env, reader, row, total_size,
                              declaration);
}
