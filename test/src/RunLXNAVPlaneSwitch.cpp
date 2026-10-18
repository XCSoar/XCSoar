// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
//
// Hardware checks for plane-switch and over-MTOW sequences on an
// LXNAV vario.
//
// 1. A full PLXV0,POLAR,W that changes the empty mass, with no
//    PLXV0,BAL afterwards.  Prints whether the overload factor stays
//    and the derived litres drift.
// 2. A POLAR read, then a full write of a pending empty mass, then a
//    full write of another plane's empty mass.  Prints whether each
//    write replaces the mass the vario stores.
// 3. A ballast factor whose all-up weight exceeds the maximum weight,
//    a maximum weight written below the loaded mass, and an empty
//    mass that already exceeds the maximum weight.
//
// The original polar and ballast are written back before exit.

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
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

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include <stdlib.h>

static constexpr const char canonical_name[] = "RunLXNAVPlaneSwitch";

struct PolarNmea {
  double a_lx = 0, b_lx = 0, c_lx = 0;
  double polar_load = 0, polar_weight = 0, max_weight = 0;
  double empty_weight = 0, pilot_weight = 0, stall = 0;
  char name[64]{};
  bool have = false;
};

enum class Want : uint8_t {
  NONE,
  POLAR,
  BALLAST,
};

static bool
ParseOverload(const std::string &line, double &overload) noexcept;

class ReplyHandler final : public DataHandler {
  std::string buf;
  EventLoop *event_loop = nullptr;

public:
  Want want = Want::NONE;
  PolarNmea polar;
  double overload = 0;
  bool have_overload = false;

  void SetEventLoop(EventLoop &_loop) noexcept {
    event_loop = &_loop;
  }

  void ClearWanted() noexcept {
    buf.clear();
    polar.have = false;
    have_overload = false;
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

      NoteLine(line);
    }

    return true;
  }

private:
  void NoteLine(const std::string &line) noexcept {
    if (want == Want::POLAR && ParsePolar(line)) {
      std::printf("  rx %s\n", line.c_str());
      if (event_loop != nullptr)
        event_loop->InjectBreak();
      return;
    }

    double value;
    if (want == Want::BALLAST && ParseOverload(line, value)) {
      overload = value;
      have_overload = true;
      std::printf("  rx %s\n", line.c_str());
      if (event_loop != nullptr)
        event_loop->InjectBreak();
    }
  }

  bool ParsePolar(const std::string &line) noexcept {
    const char *p = strstr(line.c_str(), ",POLAR,W,");
    if (p != nullptr)
      p += 9;
    else {
      p = strstr(line.c_str(), ",POL,W,");
      if (p == nullptr)
        return false;
      p += 7;
    }

    PolarNmea parsed{};
    const int n = std::sscanf(
      p, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%63[^,],%lf",
      &parsed.a_lx, &parsed.b_lx, &parsed.c_lx,
      &parsed.polar_load, &parsed.polar_weight, &parsed.max_weight,
      &parsed.empty_weight, &parsed.pilot_weight,
      parsed.name, &parsed.stall);
    if (n < 8)
      return false;

    parsed.have = true;
    polar = parsed;
    return true;
  }
};

static bool
ParseOverload(const std::string &line, double &overload) noexcept
{
  if (const char *p = strstr(line.c_str(), "$LXWP2,")) {
    double mc, ballast;
    if (std::sscanf(p, "$LXWP2,%lf,%lf", &mc, &ballast) == 2 &&
        ballast >= 0.5 && ballast <= 2.5) {
      overload = ballast;
      return true;
    }
  }

  const char *bal = strstr(line.c_str(), ",BAL,");
  if (bal == nullptr)
    return false;

  /* PLXV0,BAL,A,1.20 or PLXV0,BAL,W,1.20 */
  double value;
  if (std::sscanf(bal, ",BAL,%*[^,],%lf", &value) == 1 &&
      value >= 0.5 && value <= 2.5) {
    overload = value;
    return true;
  }

  return false;
}

struct Session final {
  EventLoop &event_loop;
  CoarseTimerEvent timeout;

  Session(EventLoop &_event_loop) noexcept
    :event_loop(_event_loop),
     timeout(_event_loop, BIND_THIS_METHOD(OnTimeout)) {}

  void OnTimeout() noexcept {
    event_loop.Break();
  }

  bool Wait(std::chrono::seconds seconds) noexcept {
    timeout.Schedule(seconds);
    event_loop.Run();
    timeout.Cancel();
    event_loop.ResetQuit();
    return true;
  }
};

static void
PrintHelp() noexcept
{
  std::printf(
    "Usage: %s PORT BAUD [OPTION]...\n"
    "\n"
    "Send the two plane-switch sequences to an LXNAV vario and print\n"
    "what it stored.  The original polar and ballast are written back\n"
    "before the program exits.  Do not interrupt it.  XCSoar must not\n"
    "hold the port.\n"
    "\n"
    "Options:\n"
    "  --read-only       print the polar and the ballast overload\n"
    "  -h, --help        display this help and exit\n"
    "  --version         output version information and exit\n"
    "\n"
    "Example:\n"
    "  %s /dev/ttyUSB0 115200\n"
    "  %s /dev/ttyUSB0 115200 --read-only\n"
    "\n"
    "Report bugs to: %s\n"
    "%s home page: %s\n",
    canonical_name, canonical_name, canonical_name,
    PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
}

static bool
WriteSentence(Port &port, OperationEnvironment &env,
              const char *sentence) noexcept
{
  std::printf("  tx $%s\n", sentence);
  try {
    PortWriteNMEA(port, sentence, env);
    port.Drain();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "Write failed: %s\n", e.what());
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  return true;
}

static bool
FormatPolar(char *dest, std::size_t size, const PolarNmea &polar,
            double empty_weight, double max_weight = -1) noexcept
{
  if (max_weight < 0)
    max_weight = polar.max_weight;

  char name[64];
  std::snprintf(name, sizeof(name), "%s", polar.name);
  for (char *p = name; *p != '\0'; ++p)
    if (*p == ',' || *p == '*' || *p == '\r' || *p == '\n')
      *p = ' ';

  const int n = std::snprintf(
    dest, size,
    "PLXV0,POLAR,W,%.6f,%.6f,%.6f,%.2f,%.1f,%.0f,%.1f,%.1f,%s,%.0f",
    polar.a_lx, polar.b_lx, polar.c_lx,
    polar.polar_load, polar.polar_weight, max_weight,
    empty_weight, polar.pilot_weight, name, polar.stall);
  return n > 0 && static_cast<std::size_t>(n) < size;
}

static bool
RequestPolar(Port &port, OperationEnvironment &env,
             Session &session, ReplyHandler &handler) noexcept
{
  handler.want = Want::POLAR;
  handler.ClearWanted();
  if (!WriteSentence(port, env, "PLXV0,POLAR,R"))
    return false;

  session.Wait(std::chrono::seconds(3));
  handler.want = Want::NONE;
  return handler.polar.have;
}

static bool
RequestBallast(Port &port, OperationEnvironment &env,
               Session &session, ReplyHandler &handler) noexcept
{
  handler.want = Want::BALLAST;
  handler.ClearWanted();
  if (!WriteSentence(port, env, "PLXV0,BAL,R"))
    return false;

  /* BAL,R may be silent.  LXWP2 carries the same overload. */
  session.Wait(std::chrono::seconds(8));
  handler.want = Want::NONE;
  return handler.have_overload;
}

static double
Litres(const PolarNmea &polar, double overload) noexcept
{
  return overload * polar.polar_weight - polar.pilot_weight -
         polar.empty_weight;
}

static void
PrintMasses(const char *label, const PolarNmea &polar,
            double overload) noexcept
{
  std::printf("  %s empty=%.1f ref=%.1f pilot=%.1f overload=%.2f "
              "litres=%.1f\n",
              label, polar.empty_weight, polar.polar_weight,
              polar.pilot_weight, overload, Litres(polar, overload));
}

/**
 * Writes the saved polar and overload back.  Runs from main and from
 * the destructor so an early return still restores the vario.
 */
struct Restore final {
  Port *port = nullptr;
  OperationEnvironment *env = nullptr;
  Session *session = nullptr;
  ReplyHandler *handler = nullptr;
  PolarNmea polar;
  double overload = 0;
  bool armed = false;
  bool done = false;

  ~Restore() noexcept {
    if (!armed || done || port == nullptr)
      return;

    std::fprintf(stderr, "Restoring the vario polar and ballast\n");
    RestoreNow();
  }

  bool RestoreNow() noexcept {
    if (port == nullptr || env == nullptr)
      return false;

    char line[256];
    if (!FormatPolar(line, sizeof(line), polar, polar.empty_weight))
      return false;

    const bool polar_ok = WriteSentence(*port, *env, line);
    char ballast[64];
    std::snprintf(ballast, sizeof(ballast), "PLXV0,BAL,W,%.2f", overload);
    const bool ballast_ok = WriteSentence(*port, *env, ballast);
    done = true;
    armed = false;

    if (!polar_ok || !ballast_ok)
      return false;

    if (session == nullptr || handler == nullptr)
      return true;

    const bool read_polar = RequestPolar(*port, *env, *session, *handler);
    const bool read_ballast =
      RequestBallast(*port, *env, *session, *handler);
    if (!read_polar || !read_ballast) {
      std::fprintf(stderr, "Restore was sent but the read-back timed out\n");
      return false;
    }

    const bool empty_ok =
      std::fabs(handler->polar.empty_weight - polar.empty_weight) < 0.2;
    const bool overload_ok =
      std::fabs(handler->overload - overload) < 0.02;
    std::printf("Restored empty=%.1f (want %.1f) overload=%.2f "
                "(want %.2f)\n",
                handler->polar.empty_weight, polar.empty_weight,
                handler->overload, overload);
    return empty_ok && overload_ok;
  }
};

static bool
Near(double a, double b) noexcept
{
  return std::fabs(a - b) < 0.2;
}

static const char *
BallastVerdict(double sent, double stored, double ceiling) noexcept
{
  if (std::fabs(stored - sent) < 0.02)
    return "STORED";
  if (stored + 0.02 < sent && stored <= ceiling + 0.03)
    return "CLAMPED";
  if (stored + 0.02 < sent)
    return "REDUCED";
  return "IGNORED";
}

/**
 * Writes values whose all-up weight exceeds the maximum weight and
 * prints what the vario stored.  A refusal is reported, not failed.
 * The caller restores the saved polar.
 */
static bool
RunOverMtow(Port &port, OperationEnvironment &env,
            Session &session, ReplyHandler &handler,
            const PolarNmea &original) noexcept
{
  if (original.polar_weight <= 0 || original.max_weight <= 0) {
    std::fprintf(stderr, "Max weight or reference mass is zero\n");
    return false;
  }

  const double ceiling = original.max_weight / original.polar_weight;
  std::printf("\nOver MTOW\n");
  std::printf("  ceiling overload %.2f (max %.0f / ref %.0f)\n",
              ceiling, original.max_weight, original.polar_weight);

  const double probes[] = {
    std::min(2.0, ceiling + 0.10),
    std::min(2.0, std::max(ceiling + 0.40, 1.50)),
  };

  for (const double sent : probes) {
    if (sent <= ceiling + 0.02) {
      std::printf("  skip BAL %.2f, not above the ceiling\n", sent);
      continue;
    }

    char ballast[64];
    std::snprintf(ballast, sizeof(ballast), "PLXV0,BAL,W,%.2f", sent);
    const double sent_total = sent * original.polar_weight;
    std::printf("BAL %.2f would weigh %.0f kg, %.0f kg over max\n",
                sent, sent_total, sent_total - original.max_weight);
    if (!WriteSentence(port, env, ballast) ||
        !RequestBallast(port, env, session, handler)) {
      std::fprintf(stderr, "Over-MTOW ballast write failed\n");
      return false;
    }

    const double stored = handler.overload;
    const double stored_total = stored * original.polar_weight;
    std::printf("  read-back %.2f (%.0f kg) %s\n",
                stored, stored_total,
                BallastVerdict(sent, stored, ceiling));
  }

  /* A legal load, then a maximum weight below that load. */
  const double loaded = std::min(1.05, ceiling);
  const double loaded_total = loaded * original.polar_weight;
  const double reduced_max = std::floor(loaded_total - 20.);
  const double dry = original.empty_weight + original.pilot_weight;
  if (loaded > 1.0 && reduced_max > dry + 5.) {
    char ballast[64];
    std::snprintf(ballast, sizeof(ballast), "PLXV0,BAL,W,%.2f", loaded);
    std::printf("Loading %.2f (%.0f kg), then max weight %.0f\n",
                loaded, loaded_total, reduced_max);
    char line[256];
    if (!WriteSentence(port, env, ballast) ||
        !RequestBallast(port, env, session, handler) ||
        !FormatPolar(line, sizeof(line), original,
                     original.empty_weight, reduced_max) ||
        !WriteSentence(port, env, line) ||
        !RequestPolar(port, env, session, handler) ||
        !RequestBallast(port, env, session, handler)) {
      std::fprintf(stderr, "Reduced max-weight write failed\n");
      return false;
    }

    const double total =
      handler.overload * original.polar_weight;
    std::printf("  max %.0f overload %.2f total %.0f kg, %s\n",
                handler.polar.max_weight, handler.overload, total,
                total > handler.polar.max_weight + 0.5
                  ? "TOTAL OVER MAX"
                  : "total within max");
  }

  /* Empty mass alone heavier than the maximum weight. */
  const double heavy_empty =
    original.max_weight - original.pilot_weight + 40.;
  if (heavy_empty > original.empty_weight + 5.) {
    char line[256];
    std::printf("Empty mass %.0f with pilot %.0f is %.0f kg over max\n",
                heavy_empty, original.pilot_weight,
                heavy_empty + original.pilot_weight -
                original.max_weight);
    if (!FormatPolar(line, sizeof(line), original, heavy_empty) ||
        !WriteSentence(port, env, line) ||
        !RequestPolar(port, env, session, handler)) {
      std::fprintf(stderr, "Heavy empty-mass write failed\n");
      return false;
    }

    const double all_up =
      handler.polar.empty_weight + handler.polar.pilot_weight;
    std::printf("  stored empty %.0f max %.0f, empty+pilot %.0f %s\n",
                handler.polar.empty_weight, handler.polar.max_weight,
                all_up,
                all_up > handler.polar.max_weight + 0.5
                  ? "OVER MAX"
                  : "within max");
  }

  return true;
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

  Args args(argc, argv, "PORT BAUD [--read-only]");
  DebugPort debug_port(args);

  bool read_only = false;
  while (args.PeekNext() != nullptr) {
    if (std::strcmp(args.PeekNext(), "--read-only") == 0) {
      read_only = true;
      args.Skip();
    } else
      break;
  }
  args.ExpectEnd();

  EventLoop event_loop;
  Cares::Channel cares(event_loop);
  ReplyHandler handler;
  handler.SetEventLoop(event_loop);
  Session session(event_loop);

  auto port = debug_port.Open(event_loop, cares, handler);
  ConsoleOperationEnvironment env;

  if (!port->StartRxThread()) {
    std::fprintf(stderr, "Failed to start the port thread\n");
    return EXIT_FAILURE;
  }

  std::printf("Reading the polar\n");
  /* Opening a closed RFCOMM port writes before the link is up, so
     the first request is lost.  Try once more. */
  if (!RequestPolar(*port, env, session, handler) &&
      !RequestPolar(*port, env, session, handler)) {
    std::fprintf(stderr,
                 "Timeout: no PLXV0 polar reply.  Check the port, the baud\n"
                 "rate, and that XCSoar is not holding the port.\n");
    return EXIT_FAILURE;
  }

  const PolarNmea original = handler.polar;
  std::printf("Reading the ballast overload\n");
  if (!RequestBallast(*port, env, session, handler)) {
    std::fprintf(stderr,
                 "Timeout: no PLXV0,BAL reply and no LXWP2.\n"
                 "The vario is not sending the overload, so the litres\n"
                 "cannot be checked.\n");
    return EXIT_FAILURE;
  }

  const double original_overload = handler.overload;
  PrintMasses("device", original, original_overload);

  if (read_only)
    return EXIT_SUCCESS;

  Restore restore;
  restore.port = port.get();
  restore.env = &env;
  restore.session = &session;
  restore.handler = &handler;
  restore.polar = original;
  restore.overload = original_overload;
  restore.armed = true;

  /* Maximum weight only.  The other polar fields stay as read. */
  std::printf("\nMaximum takeoff weight\n");
  const double new_mtow = original.max_weight + 10.;
  char line[256];
  if (!FormatPolar(line, sizeof(line), original, original.empty_weight,
                   new_mtow) ||
      !WriteSentence(*port, env, line) ||
      !RequestPolar(*port, env, session, handler)) {
    std::fprintf(stderr, "MTOW write failed\n");
    return EXIT_FAILURE;
  }
  const bool mtow_stored = std::fabs(handler.polar.max_weight - new_mtow) < 0.5;
  const bool mtow_kept_empty =
    Near(handler.polar.empty_weight, original.empty_weight);
  std::printf("  MTOW %.0f stored=%.0f %s, empty %.1f\n",
              new_mtow, handler.polar.max_weight,
              mtow_stored ? "STORED" : "NOT STORED",
              handler.polar.empty_weight);

  if (!restore.RestoreNow()) {
    std::fprintf(stderr, "Could not restore after the MTOW write\n");
    return EXIT_FAILURE;
  }
  restore.armed = true;
  restore.done = false;

  /* A pending empty mass, then the other plane's empty mass. */
  std::printf("\nPending empty mass, then the other plane\n");
  const double pending_empty = original.empty_weight + 5.;
  const double other_empty = original.empty_weight + 15.;

  std::printf("Requesting POLAR without writing\n");
  if (!RequestPolar(*port, env, session, handler) ||
      !Near(handler.polar.empty_weight, original.empty_weight)) {
    std::fprintf(stderr, "POLAR,R changed or missed the empty mass\n");
    restore.RestoreNow();
    return EXIT_FAILURE;
  }
  std::printf("  POLAR,R left empty mass at %.1f\n",
              handler.polar.empty_weight);

  if (!FormatPolar(line, sizeof(line), original, pending_empty) ||
      !WriteSentence(*port, env, line) ||
      !RequestPolar(*port, env, session, handler)) {
    std::fprintf(stderr, "Pending empty-mass write failed\n");
    return EXIT_FAILURE;
  }
  const bool pending_stored =
    Near(handler.polar.empty_weight, pending_empty);
  std::printf("  pending write empty=%.1f stored=%.1f %s\n",
              pending_empty, handler.polar.empty_weight,
              pending_stored ? "STORED" : "NOT STORED");

  if (!FormatPolar(line, sizeof(line), original, other_empty) ||
      !WriteSentence(*port, env, line) ||
      !RequestPolar(*port, env, session, handler)) {
    std::fprintf(stderr, "Other-plane empty-mass write failed\n");
    return EXIT_FAILURE;
  }
  const bool other_stored = Near(handler.polar.empty_weight, other_empty);
  std::printf("  other plane empty=%.1f stored=%.1f %s\n",
              other_empty, handler.polar.empty_weight,
              other_stored ? "REPLACED" : "NOT REPLACED");

  /* Back to the saved polar before the ballast probe. */
  if (!restore.RestoreNow()) {
    std::fprintf(stderr, "Could not restore before the ballast test\n");
    return EXIT_FAILURE;
  }
  restore.armed = true;
  restore.done = false;

  std::printf("\nPlane switch without a ballast sentence\n");
  if (original.polar_weight <= 0) {
    std::fprintf(stderr, "Reference mass is zero; litres are undefined\n");
    return EXIT_FAILURE;
  }

  double ceiling = 1.5;
  if (original.max_weight > original.polar_weight)
    ceiling = original.max_weight / original.polar_weight;

  /* Leave room for the same water after the empty mass grows, so the
     BAL sentence that preserves litres stays inside the max weight. */
  const double new_empty = original.empty_weight + 20.;
  const double room_after =
    original.max_weight - new_empty - original.pilot_weight;
  double probe = original_overload;
  double litres =
    probe * original.polar_weight - original.pilot_weight -
    original.empty_weight;
  /* 1.00 is the lowest overload this vario stores.  It is still zero
     ballast fraction, so raise it while the water fits after the
     empty mass grows. */
  if (probe < 1.05 && room_after > 20.) {
    double want = 1.05;
    if (want > ceiling)
      want = ceiling;
    const double want_litres =
      want * original.polar_weight - original.pilot_weight -
      original.empty_weight;
    if (want_litres > litres && want_litres < room_after - 5.) {
      litres = want_litres;
      probe = want;
    }
  }
  if (std::fabs(probe - original_overload) >= 0.02 && room_after > 20.) {
    char ballast[64];
    std::snprintf(ballast, sizeof(ballast), "PLXV0,BAL,W,%.2f", probe);
    std::printf("Setting overload to %.2f so there is water to watch\n",
                probe);
    if (!WriteSentence(*port, env, ballast) ||
        !RequestBallast(*port, env, session, handler) ||
        std::fabs(handler.overload - probe) >= 0.02) {
      std::fprintf(stderr, "The vario did not accept the probe overload\n");
      return EXIT_FAILURE;
    }
    probe = handler.overload;
    litres = probe * original.polar_weight - original.pilot_weight -
             original.empty_weight;
  }
  std::printf("  before empty=%.1f overload=%.2f litres=%.1f\n",
              original.empty_weight, probe, litres);

  if (!FormatPolar(line, sizeof(line), original, new_empty) ||
      !WriteSentence(*port, env, line) ||
      !RequestPolar(*port, env, session, handler) ||
      !RequestBallast(*port, env, session, handler)) {
    std::fprintf(stderr, "Polar write without ballast failed\n");
    return EXIT_FAILURE;
  }

  const double after_overload = handler.overload;
  const double after_litres =
    after_overload * original.polar_weight - original.pilot_weight -
    handler.polar.empty_weight;
  const bool overload_held = std::fabs(after_overload - probe) < 0.015;
  std::printf("  after  empty=%.1f overload=%.2f litres=%.1f\n",
              handler.polar.empty_weight, after_overload, after_litres);
  std::printf("  %s\n",
              overload_held
                ? "overload unchanged: a plane switch must send BAL"
                : "overload changed when the polar masses changed");

  bool bal_stored = true;
  const double keep_litres =
    (litres + original.pilot_weight + new_empty) / original.polar_weight;
  if (keep_litres >= 0.5 && keep_litres <= ceiling + 0.001) {
    char ballast[64];
    std::snprintf(ballast, sizeof(ballast), "PLXV0,BAL,W,%.2f",
                  keep_litres);
    std::printf("Sending BAL %.2f to keep the same litres\n", keep_litres);
    if (!WriteSentence(*port, env, ballast) ||
        !RequestBallast(*port, env, session, handler)) {
      std::fprintf(stderr, "BAL write failed\n");
      return EXIT_FAILURE;
    }
    bal_stored = std::fabs(handler.overload - keep_litres) < 0.02;
    std::printf("  BAL read-back %.2f %s\n", handler.overload,
                bal_stored ? "STORED" : "NOT STORED");
  } else
    std::printf("  keeping the litres would exceed max weight; "
                "BAL was not sent\n");

  if (!restore.RestoreNow())
    return EXIT_FAILURE;

  restore.armed = true;
  restore.done = false;
  if (!RunOverMtow(*port, env, session, handler, original)) {
    restore.RestoreNow();
    return EXIT_FAILURE;
  }

  if (!restore.RestoreNow())
    return EXIT_FAILURE;

  if (!mtow_stored || !mtow_kept_empty ||
      !pending_stored || !other_stored || !bal_stored)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
} catch (const std::exception &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
