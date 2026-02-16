// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugPort.hpp"
#include "system/Args.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "io/NullDataHandler.hpp"
#include "util/SpanCast.hxx"
#include "util/StaticString.hxx"
#include "util/PrintException.hxx"
#include "Math/Util.hpp"
#include "time/PeriodClock.hpp"
#include "time/Cast.hxx"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT");
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *global_cares_channel, handler);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  PeriodClock start_clock;
  start_clock.Update();

  PeriodClock pressure_clock;
  PeriodClock battery_clock;

  double pressure = 101300;
  unsigned battery_level = 11;
  while (true) {
    if (pressure_clock.CheckUpdate(std::chrono::milliseconds(48))) {
      StaticString<16> sentence;

      const auto elapsed = ToFloatSeconds(start_clock.Elapsed());
      auto vario = sin(elapsed / 3) * cos(elapsed / 10) *
        cos(elapsed / 20 + 2) * 3;

      auto pressure_vario = -vario * 12.5;
      auto delta_pressure = pressure_vario * 48 / 1000;
      pressure += delta_pressure;

      sentence = "_PRS ";
      sentence.AppendFormat("%08X", uround(pressure));
      sentence += "\n";

      port->Write(AsBytes(std::string_view{sentence}));
    }

    if (battery_clock.CheckUpdate(std::chrono::seconds(11))) {
      StaticString<16> sentence;

      sentence = "_BAT ";
      if (battery_level <= 10)
        sentence.AppendFormat("%X", battery_level);
      else
        sentence += "*";

      sentence += "\n";
      port->Write(sentence);

      if (battery_level == 0)
        battery_level = 11;
      else
        battery_level--;
    }
  }
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
