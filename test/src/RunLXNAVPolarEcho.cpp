// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Hardware check for LXNAV PLXV0 POLAR coefficient conversion (#2397).
// Connect an LXNAV V7 / S80 / S10 (or similar) on a serial/USB port.
//
// Usage: RunLXNAVPolarEcho PORT BAUD [OPTION]...
//
// Default: write a known polar, read it back, verify SI round-trip.
// --read-only: only request POLAR and print SI coefficients.
// --preserve-crew: after a successful read, rewrite full POLAR with a
//   changed pilot weight and verify a,b,c / max / name are preserved
//   (regression for empty-field polar wipes).

#include "DebugPort.hpp"
#include "Device/Driver/LX/LXNAVPolarConversion.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "ProductName.hpp"
#include "Version.hpp"
#include "event/CoarseTimerEvent.hxx"
#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "io/DataHandler.hpp"
#include "system/Args.hpp"
#include "system/StandardVersion.hpp"
#include "util/BindMethod.hxx"
#include "util/PrintException.hxx"
#include "util/StringCompare.hxx"

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <thread>

#include <stdlib.h>

static constexpr const char canonical_name[] = "RunLXNAVPolarEcho";

struct PolarNmea {
  double a_lx = 0, b_lx = 0, c_lx = 0;
  double polar_load = 0, polar_weight = 0, max_weight = 0;
  double empty_weight = 0, pilot_weight = 0, stall = 0;
  char name[32]{};
  bool have = false;
};

class PolarEchoHandler final : public DataHandler {
  std::string buf;

  EventLoop *event_loop = nullptr;

public:
  PolarNmea polar;

  void SetEventLoop(EventLoop &_loop) noexcept {
    event_loop = &_loop;
  }

  void ClearBuffer() noexcept {
    buf.clear();
    polar = {};
  }

  bool DataReceived(std::span<const std::byte> s) noexcept override {
    buf.append(reinterpret_cast<const char *>(s.data()), s.size());

    for (;;) {
      const auto pos = buf.find('\n');
      if (pos == std::string::npos)
        break;

      std::string line = buf.substr(0, pos);
      buf.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      const char *p = strstr(line.c_str(), ",POLAR,W,");
      if (p != nullptr)
        p += 9;
      else {
        p = strstr(line.c_str(), ",POL,W,");
        if (p == nullptr)
          continue;
        p += 7;
      }

      PolarNmea parsed{};
      int n = std::sscanf(
        p,
        "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%31[^,],%lf",
        &parsed.a_lx, &parsed.b_lx, &parsed.c_lx,
        &parsed.polar_load, &parsed.polar_weight, &parsed.max_weight,
        &parsed.empty_weight, &parsed.pilot_weight,
        parsed.name, &parsed.stall);
      if (n >= 3) {
        parsed.have = true;
        polar = parsed;
        if (event_loop != nullptr)
          event_loop->InjectBreak();
        break;
      }
    }

    return true;
  }
};

struct Session final {
  EventLoop &event_loop;
  PolarEchoHandler &handler;
  CoarseTimerEvent timeout;

  Session(EventLoop &_event_loop, PolarEchoHandler &_handler) noexcept
    :event_loop(_event_loop), handler(_handler),
     timeout(_event_loop, BIND_THIS_METHOD(OnTimeout)) {}

  void OnTimeout() noexcept {
    event_loop.Break();
  }

  void ArmTimeout(std::chrono::seconds seconds) noexcept {
    timeout.Schedule(seconds);
  }
};

static void
PrintHelp() noexcept
{
  std::printf(
    "Usage: %s PORT BAUD [OPTION]...\n"
    "\n"
    "Talk to an LXNAV V7/S80/S10 (or compatible) and verify PLXV0 POLAR\n"
    "coefficient conversion used by XCSoar (#2397).\n"
    "PORT is a serial device such as /dev/ttyUSB0 or COM3.\n"
    "\n"
    "Options:\n"
    "  --read-only       only request POLAR; print SI coefficients\n"
    "  --preserve-crew   after read, rewrite full POLAR with pilot weight\n"
    "                    95 kg and check a,b,c/max/name stay intact\n"
    "  -h, --help        display this help and exit\n"
    "  --version         output version information and exit\n"
    "\n"
    "Examples:\n"
    "  %s /dev/ttyUSB0 115200\n"
    "  %s COM3 115200\n"
    "  %s /dev/ttyUSB0 115200 --read-only\n"
    "  %s /dev/ttyUSB0 115200 --preserve-crew\n"
    "\n"
    "Report bugs to: %s\n"
    "%s home page: %s\n",
    canonical_name, canonical_name, canonical_name, canonical_name,
    canonical_name,
    PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static bool
RequestPolar(Port &port, OperationEnvironment &env,
             EventLoop &event_loop, Session &session,
             PolarEchoHandler &handler) noexcept
{
  handler.ClearBuffer();
  PortWriteNMEA(port, "PLXV0,POLAR,R", env);
  port.Drain();
  session.ArmTimeout(std::chrono::seconds(3));
  event_loop.Run();
  session.timeout.Cancel();
  event_loop.ResetQuit();
  return handler.polar.have;
}

static void
PrintPolar(const PolarNmea &p) noexcept
{
  std::printf("Received LX a,b,c: %.9f %.9f %.9f\n",
              p.a_lx, p.b_lx, p.c_lx);
  std::printf("  load=%.2f ref=%.1f max=%.0f empty=%.1f pilot=%.1f "
              "name='%s' stall=%.0f\n",
              p.polar_load, p.polar_weight, p.max_weight,
              p.empty_weight, p.pilot_weight, p.name, p.stall);

  const PolarCoefficients si =
    LXNAVPolar::FromNmeaPolar(p.a_lx, p.b_lx, p.c_lx);
  std::printf("Converted SI a,b,c: %.9f %.9f %.9f\n", si.a, si.b, si.c);
}

int
main(int argc, char **argv)
try {
  for (int i = 1; i < argc; ++i) {
    if (StringIsEqual(argv[i], "--help") || StringIsEqual(argv[i], "-h")) {
      PrintHelp();
      return EXIT_SUCCESS;
    }
    if (StringIsEqual(argv[i], "--version")) {
      PrintStandardVersion(canonical_name, XCSoar_Version);
      return EXIT_SUCCESS;
    }
  }

  Args args(argc, argv, "PORT BAUD [--read-only] [--preserve-crew]");
  DebugPort debug_port(args);

  bool read_only = false;
  bool preserve_crew = false;
  while (args.PeekNext() != nullptr) {
    if (std::strcmp(args.PeekNext(), "--read-only") == 0) {
      read_only = true;
      args.Skip();
    } else if (std::strcmp(args.PeekNext(), "--preserve-crew") == 0) {
      preserve_crew = true;
      args.Skip();
    } else
      break;
  }
  args.ExpectEnd();

  EventLoop event_loop;
  Cares::Channel cares(event_loop);
  PolarEchoHandler handler;
  handler.SetEventLoop(event_loop);
  Session session(event_loop, handler);

  auto port = debug_port.Open(event_loop, cares, handler);
  ConsoleOperationEnvironment env;

  if (!port->StartRxThread()) {
    std::fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  /* Default SI polar (206 Hornet sample) and masses matching unit tests */
  const PolarCoefficients expected_si(0.0022032, -0.08784, 1.47);
  constexpr double ref_mass = 318.;
  constexpr double empty_mass = 228.;
  constexpr double crew_mass = 90.;
  constexpr double wing_area = 9.8;
  const double polar_load =
    (wing_area > 0 && ref_mass > 0) ? ref_mass / wing_area : 0.;
  constexpr double max_weight = 550.;
  constexpr double stall = 25.;

  if (!read_only) {
    double a_lx, b_lx, c_lx;
    LXNAVPolar::ToNmeaPolar(expected_si, a_lx, b_lx, c_lx);

    char line[256];
    const int n = std::snprintf(
      line, sizeof(line),
      "PLXV0,POLAR,W,%.6f,%.6f,%.6f,%.2f,%.1f,%.0f,%.1f,%.1f,%s,%.0f",
      a_lx, b_lx, c_lx, polar_load, ref_mass, max_weight,
      empty_mass, crew_mass, "ECHO", stall);
    if (n <= 0 || unsigned(n) >= sizeof(line)) {
      std::fprintf(stderr, "POLAR command buffer too small\n");
      return EXIT_FAILURE;
    }

    std::printf("Writing test polar (name=ECHO)...\n");
    PortWriteNMEA(*port, line, env);
    port->Drain();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }

  if (!RequestPolar(*port, env, event_loop, session, handler)) {
    std::fprintf(stderr, "Timeout: no $PLXV0,...,POLAR,W,... line received.\n"
                 "Check cabling, baud rate, and that XCSoar is not holding "
                 "the port.\n");
    return EXIT_FAILURE;
  }

  PrintPolar(handler.polar);

  if (!read_only) {
    const PolarCoefficients back =
      LXNAVPolar::FromNmeaPolar(handler.polar.a_lx, handler.polar.b_lx,
                                handler.polar.c_lx);
    const bool ok_a = std::fabs(back.a - expected_si.a) < 1e-9;
    const bool ok_b = std::fabs(back.b - expected_si.b) < 1e-9;
    const bool ok_c = std::fabs(back.c - expected_si.c) < 1e-9;
    std::printf("Round-trip vs sent SI: %s\n",
                (ok_a && ok_b && ok_c) ? "OK" : "MISMATCH");
    if (!ok_a || !ok_b || !ok_c) {
      std::printf("Expected SI: %.9f %.9f %.9f\n",
                  expected_si.a, expected_si.b, expected_si.c);
      return EXIT_FAILURE;
    }
  }

  if (preserve_crew) {
    const PolarNmea before = handler.polar;
    if (!before.have) {
      std::fprintf(stderr, "--preserve-crew needs a readable device polar\n");
      return EXIT_FAILURE;
    }

    constexpr double new_crew = 95.;
    char line[256];
    const int n = std::snprintf(
      line, sizeof(line),
      "PLXV0,POLAR,W,%.6f,%.6f,%.6f,%.2f,%.1f,%.0f,%.1f,%.1f,%s,%.0f",
      before.a_lx, before.b_lx, before.c_lx,
      before.polar_load, before.polar_weight, before.max_weight,
      before.empty_weight, new_crew, before.name, before.stall);
    if (n <= 0 || unsigned(n) >= sizeof(line)) {
      std::fprintf(stderr, "POLAR command buffer too small\n");
      return EXIT_FAILURE;
    }

    std::printf("Rewriting full POLAR with pilot=%.1f (preserve a,b,c)...\n",
                new_crew);
    PortWriteNMEA(*port, line, env);
    port->Drain();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    if (!RequestPolar(*port, env, event_loop, session, handler)) {
      std::fprintf(stderr, "Timeout after crew-weight rewrite\n");
      return EXIT_FAILURE;
    }

    PrintPolar(handler.polar);

    const bool ok =
      std::fabs(handler.polar.a_lx - before.a_lx) < 1e-6 &&
      std::fabs(handler.polar.b_lx - before.b_lx) < 1e-6 &&
      std::fabs(handler.polar.c_lx - before.c_lx) < 1e-6 &&
      std::fabs(handler.polar.max_weight - before.max_weight) < 0.5 &&
      std::strcmp(handler.polar.name, before.name) == 0 &&
      std::fabs(handler.polar.pilot_weight - new_crew) < 0.2;
    std::printf("Preserve coefficients after crew rewrite: %s\n",
                ok ? "OK" : "FAIL");
    if (!ok)
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  PrintException(e);
  PrintHelp();
  return EXIT_FAILURE;
}
