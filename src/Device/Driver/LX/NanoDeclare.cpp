/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "NanoDeclare.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Device/Util/NMEAReader.hpp"
#include "Device/Declaration.hpp"
#include "IGC/Generator.hpp"
#include "Time/TimeoutClock.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Operation/Operation.hpp"
#include "Util/ConvertString.hpp"

static bool
NanoWriteDecl(Port &port, OperationEnvironment &env, PortNMEAReader &reader,
              unsigned row, unsigned n_rows,
              const char *content)
{
  NarrowString<256> buffer;
  buffer.Format("$PLXVC,DECL,W,%u,%u,%s,", row, n_rows, content);

  if (!PortWriteNMEA(port, buffer, env))
    return false;

  buffer.UnsafeFormat("PLXVC,DECL,C,%u", row);
  char *response = reader.ExpectLine(buffer, TimeoutClock(2000));
  return response != nullptr && (*response == 0 || *response == ',');
}

template<typename... Args>
static bool
NanoWriteDeclFormat(Port &port, OperationEnvironment &env,
                    PortNMEAReader &reader,
                    unsigned row, unsigned n_rows,
                    const char *fmt, Args&&... args)
{
  NarrowString<256> buffer;
  buffer.Format(fmt, args...);
  return NanoWriteDecl(port, env, reader, row, n_rows, buffer);
}

template<typename... Args>
static bool
NanoWriteDeclString(Port &port, OperationEnvironment &env,
                    PortNMEAReader &reader,
                    unsigned row, unsigned n_rows,
                    const char *prefix, const TCHAR *value)
{
  WideToUTF8Converter narrow_value(value);
  if (!narrow_value.IsValid())
    return false;

  return NanoWriteDeclFormat(port, env, reader, row, n_rows,
                             "%s%s", prefix, (const char *)narrow_value);
}

static bool
NanoWriteDeclMeta(Port &port, OperationEnvironment &env,
                  PortNMEAReader &reader,
                  const Declaration &declaration, unsigned total_size)
{
  return NanoWriteDeclString(port, env, reader, 1, total_size,
                             "HFPLTPILOT:", declaration.pilot_name) &&
    NanoWriteDecl(port, env, reader, 2, total_size, "HFCM2CREW2:") &&
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
                          const Declaration &declaration, unsigned total_size)
{
  // TODO: use GPS clock instead?
  const BrokenDateTime date_time = BrokenDateTime::NowUTC();

  char buffer[64];
  FormatIGCTaskTimestamp(buffer, date_time, total_size - 2);
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
                 PortNMEAReader &reader, unsigned total_size)
{
  return NanoWriteDecl(port, env, reader, total_size, total_size,
                       IGCMakeTaskLanding());
}

bool
Nano::Declare(Port &port, const Declaration &declaration,
              OperationEnvironment &env)
{
  constexpr unsigned prefix_size = 8;
  constexpr unsigned suffix_size = 1;
  const unsigned task_size = declaration.Size();
  const unsigned total_size = prefix_size + task_size + suffix_size;

  env.SetProgressRange(total_size);

  port.StopRxThread();
  PortNMEAReader reader(port, env);

  if (!NanoBeginDeclaration(port, env, reader, declaration, total_size))
    return false;

  unsigned i = prefix_size + 1;
  for (const auto &tp : declaration.turnpoints) {
    char buffer[128];
    FormatIGCTaskTurnPoint(buffer, tp.waypoint.location,
                           tp.waypoint.name.c_str());
    if (!NanoWriteDecl(port, env, reader, i++, total_size,
                       buffer))
      return false;
  }

  assert(i == total_size);

  return NanoWriteLanding(port, env, reader, total_size);
}
