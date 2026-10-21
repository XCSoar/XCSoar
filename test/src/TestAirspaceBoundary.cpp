// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* One sample for each answer AirspaceWarningManager can give.
 *
 * Cylinder radius 5000 m, floor 1000 m, ceiling 2000 m, unless a
 * fix says otherwise. Ground speed 40 m/s and the 30 s warning
 * look 1200 m ahead. MacCready is 2 m/s.
 *
 * q quiet  I inside  n near  a above  b below  c covered  k acked
 */

#include "Airspace/SpokenAlert.hpp"
#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Stats/TaskStats.hpp"
#include "Geo/GeoVector.hpp"
#include "Radio/TransponderCode.hpp"
#include "TestUtil.hpp"
#include "time/FloatDuration.hxx"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>

namespace {

constexpr GeoPoint
P(double lon, double lat) noexcept
{
  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

constexpr GeoPoint kOrigin = P(10.0, 50.0);

struct Fix
{
  const char *name;
  double east, north, alt;
  double track, gs, vario;
  int dt;
  char want_a, want_b;
};

GeoPoint
Offset(GeoPoint from, double east, double north) noexcept
{
  if (east != 0)
    from = GeoVector(std::fabs(east), Angle::Degrees(east > 0 ? 90.0 : 270.0))
             .EndPoint(from);
  if (north != 0)
    from = GeoVector(std::fabs(north), Angle::Degrees(north > 0 ? 0.0 : 180.0))
             .EndPoint(from);
  return from;
}

AirspaceAltitude
Alt(double metres) noexcept
{
  AirspaceAltitude altitude{};
  altitude.altitude = metres;
  altitude.reference = AltitudeReference::MSL;
  return altitude;
}

AirspacePtr
Circle(double east, double north, double radius, double base, double top,
       AirspaceClass cls = AirspaceClass::RESTRICTED)
{
  auto airspace =
    std::make_shared<AirspaceCircle>(Offset(kOrigin, east, north), radius);
  TransponderCode code;
  airspace->SetProperties("volume", "", std::move(code), cls, cls, Alt(base),
                          Alt(top));
  return airspace;
}

AirspacePtr
Volume()
{
  return Circle(0, 0, 5000, 1000, 2000);
}

char
Classify(const AirspaceWarningManager &mgr, const AbstractAirspace &airspace,
         double altitude) noexcept
{
  const AirspaceWarning *warning = mgr.GetWarningPtr(airspace);
  if (warning == nullptr || !warning->IsWarning())
    return 'q';
  if (warning->IsCoveredByClearance())
    return 'c';
  if (warning->IsCleared() || !warning->IsAckExpired())
    return 'k';
  if (warning->IsInside())
    return 'I';

  const AirspaceInterceptSolution &solution = warning->GetSolution();
  const SpokenAirspaceAlert::Kind kind =
    SpokenAirspaceAlert::From(warning->GetWarningState(), solution, altitude)
      .kind;
  if (kind == SpokenAirspaceAlert::Kind::ABOVE)
    return 'a';
  if (solution.IsValid() && !(solution.distance > 0))
    return 'b';
  return 'n';
}

struct Scene
{
  Airspaces db;
  AirspaceWarningConfig cfg{};
  AircraftState state{};
  GlidePolar polar;
  TaskStats task_stats{};
  AirspacePtr a;
  AirspacePtr b;
  std::unique_ptr<AirspaceWarningManager> mgr;

  explicit Scene(AirspacePtr first, AirspacePtr second = nullptr)
    : polar(2.0), a(std::move(first)), b(std::move(second))
  {
    db.Add(a);
    if (b != nullptr)
      db.Add(b);
    db.Optimise();
    cfg.SetDefaults();
    task_stats.reset();
    mgr = std::make_unique<AirspaceWarningManager>(cfg, db);
    state.Reset();
    state.flying = true;
    state.location = kOrigin;
    state.time = TimeStamp{FloatDuration{1}};
    mgr->Reset(state);
  }

  void Fly(const Fix &fix, bool fresh)
  {
    state.location = Offset(kOrigin, fix.east, fix.north);
    state.altitude = fix.alt;
    state.altitude_agl = fix.alt;
    state.track = Angle::Degrees(fix.track);
    state.ground_speed = fix.gs;
    state.true_airspeed = fix.gs;
    state.vario = fix.vario;
    state.netto_vario = fix.vario;
    state.flying = true;

    if (fresh) {
      state.time = TimeStamp{FloatDuration{1}};
      mgr->Reset(state);
    }

    const auto dt = std::chrono::seconds{fix.dt};
    state.time += dt;
    mgr->Update(state, polar, task_stats, false, dt);

    const char heard_a = Classify(*mgr, *a, state.altitude);
    if (heard_a != fix.want_a)
      diag("%s a: %c", fix.name, heard_a);
    ok1(heard_a == fix.want_a);

    if (fix.want_b == '-')
      return;

    const char heard_b = Classify(*mgr, *b, state.altitude);
    if (heard_b != fix.want_b)
      diag("%s b: %c", fix.name, heard_b);
    ok1(heard_b == fix.want_b);
  }
};

template <std::size_t N>
constexpr unsigned
Checks(const Fix (&fixes)[N]) noexcept
{
  unsigned n = 0;
  for (const auto &fix : fixes)
    n += fix.want_b == '-' ? 1u : 2u;
  return n;
}

template <std::size_t N>
void
Run(Scene &scene, const Fix (&fixes)[N], bool fresh)
{
  for (const auto &fix : fixes)
    scene.Fly(fix, fresh);
}

/* Easting is metres from the centre. Track 90 is east. */

constexpr Fix kOnce[] = {
  {"beyond 30 s, toward", -6400, 0, 1500, 90, 40, 0, 1, 'q', '-'},
  {"inside 30 s, toward", -5600, 0, 1500, 90, 40, 0, 1, 'n', '-'},
  {"on the rim", -5000, 0, 1500, 90, 40, 0, 1, 'I', '-'},
  {"just outside, away", 5150, 0, 1500, 90, 40, 0, 1, 'q', '-'},
  {"abeam, track misses", 0, 5200, 1500, 90, 40, 0, 1, 'q', '-'},
  /* No ground speed: the path has no length, so the climb is not
     projected. */
  {"floor ahead, stopped", 0, 0, 920, 90, 0, 4, 1, 'q', '-'},
  {"on the floor", 0, 0, 1000, 90, 0, 0, 1, 'I', '-'},
  {"inside, climbing out soon", 0, 0, 1500, 90, 0, 20, 1, 'I', '-'},
  {"ceiling ahead, stopped", 0, 0, 2080, 90, 0, -4, 1, 'q', '-'},
  /* 80 m at MacCready 2 m/s is 40 s, past the warning. */
  {"toward wall, climb too slow", -5600, 0, 920, 90, 40, 4, 1, 'q', '-'},
  {"under floor, MC reaches", 0, 0, 960, 90, 40, 4, 1, 'a', '-'},
  {"over ceiling, sink reaches", 0, 0, 2040, 90, 40, -4, 1, 'b', '-'},
  {"below floor, flying out", -4600, 0, 600, 90, 40, 0, 1, 'q', '-'},
  {"above ceiling, flying out", -4600, 0, 2400, 90, 40, 0, 1, 'q', '-'},
  {"toward wall, below floor", -5200, 0, 960, 90, 40, 4, 1, 'n', '-'},
  {"toward wall, above ceiling", -5600, 0, 2080, 90, 40, -4, 1, 'n', '-'},
};

/* Still climbing after leaving the ceiling. The polar can descend
   back in, until the ceiling is far enough. */
constexpr Fix kClimbOut[] = {
  {"on the ceiling, climbing", 0, 0, 2000, 0, 0, 4, 1, 'I', '-'},
  {"just above, still climbing", 0, 0, 2080, 0, 0, 4, 20, 'b', '-'},
  {"well above, still climbing", 0, 0, 2300, 0, 0, 4, 55, 'q', '-'},
};

/* Same footprint. a is 0..1500, b is 1700..3000.
   The gap stays "below" only while the filter still remembers
   the lower volume, so those two samples are one flight. */
constexpr Fix kGap[] = {
  {"in the lower", 0, 0, 800, 0, 0, 4, 1, 'I', 'q'},
  {"gap, polar can descend", 0, 0, 1550, 0, 0, 4, 20, 'b', 'q'},
};

constexpr Fix kStack[] = {
  /* Stopped, so the floor 60 m above is not projected. */
  {"upper floor, stopped", 0, 0, 1640, 0, 0, 4, 1, 'q', 'q'},
  {"in the upper", 0, 0, 1900, 0, 0, 4, 1, 'q', 'I'},
};

/* a outer r 8000, b inner r 2500. */
constexpr Fix kNested[] = {
  {"near outer", -9000, 0, 1500, 90, 40, 0, 1, 'n', 'q'},
  {"in outer", -7000, 0, 1500, 90, 40, 0, 1, 'I', 'q'},
  {"near inner", -3100, 0, 1500, 90, 40, 0, 1, 'I', 'n'},
  {"in both", -1500, 0, 1500, 90, 40, 0, 1, 'I', 'I'},
  {"left inner", 3200, 0, 1500, 90, 40, 0, 1, 'I', 'q'},
  {"left outer", 9000, 0, 1500, 90, 40, 0, 1, 'q', 'q'},
};

/* West circle, then a gap, then the east circle. */
constexpr Fix kAdjacent[] = {
  {"in the west", -4000, 0, 1500, 90, 40, 0, 1, 'I', 'q'},
  {"in the gap", -500, 0, 1500, 90, 40, 0, 1, 'q', 'q'},
  {"near the east", 1400, 0, 1500, 90, 40, 0, 1, 'q', 'n'},
  {"in the east", 4000, 0, 1500, 90, 40, 0, 1, 'q', 'I'},
};

/* a at 0 r 3000, b at east 2000 r 3000. */
constexpr Fix kLens[] = {
  {"in a, b ahead", -2000, 0, 1500, 90, 40, 0, 1, 'I', 'n'},
  {"in both", 0, 0, 1500, 90, 40, 0, 1, 'I', 'I'},
  {"left a", 3600, 0, 1500, 90, 40, 0, 1, 'q', 'I'},
};

constexpr unsigned kFixChecks = Checks(kOnce) + Checks(kClimbOut) +
                                Checks(kGap) + Checks(kStack) +
                                Checks(kNested) + Checks(kAdjacent) +
                                Checks(kLens);

/* Exit, the descent filter, ack, clearance, class E. */
constexpr unsigned kFollowOn = 2 + 5 + 11 + 8 + 4 + 1;

void
TestExitAndFilter()
{
  constexpr Fix exit_now[] = {
    {"exit inside", 0, 0, 1500, 90, 40, 0, 1, 'I', '-'},
    {"exit outside", 5400, 0, 1500, 90, 40, 0, 1, 'q', '-'},
  };
  Scene scene{Volume()};
  Run(scene, exit_now, false);

  /* The cruise filter has not accepted the descent yet. */
  constexpr Fix descend[] = {
    {"descend inside", 0, 0, 1500, 0, 0, -4, 1, 'I', '-'},
    {"descend 1300", 0, 0, 1300, 0, 0, -4, 50, 'I', '-'},
    {"descend 1100", 0, 0, 1100, 0, 0, -4, 50, 'I', '-'},
    {"descend on the floor", 0, 0, 1000, 0, 0, -4, 10, 'I', '-'},
    {"descend just below", 0, 0, 920, 0, 0, -4, 20, 'a', '-'},
  };
  Scene down{Volume()};
  Run(down, descend, false);
}

void
TestAck()
{
  {
    Scene scene{Volume()};
    scene.Fly({"ack day inside", 0, 0, 1500, 90, 40, 0, 1, 'I', '-'}, false);
    scene.mgr->AcknowledgeDay(scene.a, true);
    scene.Fly({"ack day outside", -8000, 0, 1500, 270, 40, 0, 5, 'q', '-'},
              false);
    scene.Fly({"ack day reentry", 0, 0, 1500, 90, 40, 0, 5, 'k', '-'}, false);
  }

  {
    Scene scene{Volume()};
    scene.Fly({"ack expire inside", 0, 0, 1500, 90, 0, 0, 1, 'I', '-'},
              false);
    scene.mgr->AcknowledgeInside(scene.a, true);
    scene.Fly({"ack expire done", 0, 0, 1500, 90, 0, 0, 30, 'I', '-'}, false);
  }

  {
    Scene scene{Volume()};
    scene.Fly({"ack leave inside", 0, 0, 1500, 90, 40, 0, 1, 'I', '-'},
              false);
    scene.mgr->AcknowledgeInside(scene.a, true);
    scene.Fly({"ack leave gone", 8000, 0, 1500, 90, 40, 0, 5, 'q', '-'},
              false);
    scene.Fly({"ack leave back", 0, 0, 1500, 270, 40, 0, 5, 'I', '-'}, false);
  }

  {
    Scene scene{Volume()};
    scene.Fly({"ack step inside", 4600, 0, 1500, 90, 40, 0, 1, 'I', '-'},
              false);
    scene.mgr->AcknowledgeInside(scene.a, true);
    scene.Fly(
      {"ack step outside toward", 5400, 0, 1500, 270, 40, 0, 5, 'k', '-'},
      false);
    scene.Fly({"ack step back inside", 4600, 0, 1500, 270, 40, 0, 5, 'k', '-'},
              false);
  }
}

void
TestClearance()
{
  {
    Scene scene(Circle(0, 0, 2000, 0, 3000), Circle(0, 0, 8000, 0, 3000));
    scene.mgr->SetCleared(scene.a, true);
    scene.Fly({"clearance deep", 0, 0, 1500, 90, 40, 0, 1, 'k', 'c'}, false);
    scene.Fly(
      {"clearance near inner wall", 1600, 0, 1500, 90, 40, 0, 5, 'k', 'n'},
      false);
  }

  /* Cleared 0..2500, restricted from 2530. 2480 m is inside the
     MacCready reach; 2000 m is not. */
  {
    Scene scene(Circle(0, 0, 5000, 0, 2500), Circle(0, 0, 5000, 2530, 4000));
    scene.mgr->SetCleared(scene.a, true);
    scene.Fly({"stack clear too far", 0, 0, 2000, 90, 40, 0, 1, 'k', 'q'},
              false);
    scene.Fly({"stack clear climbing", 0, 0, 2480, 90, 40, 5, 1, 'k', 'a'},
              false);
  }
}

void
TestAckOther()
{
  Scene scene(Circle(0, 0, 2000, 0, 2500), Circle(0, 0, 8000, 0, 4000));
  scene.Fly({"ack other before", 0, 0, 1500, 90, 40, 0, 1, 'I', 'I'}, false);
  scene.mgr->AcknowledgeDay(scene.a, true);
  scene.Fly({"ack other after", 0, 0, 1500, 90, 40, 0, 1, 'k', 'I'}, false);
}

void
TestClassDisabled()
{
  Scene scene(Circle(0, 0, 5000, 1000, 2000, AirspaceClass::CLASSE));
  scene.Fly({"class E inside", 0, 0, 1500, 90, 40, 0, 1, 'q', '-'}, false);
}

} // namespace

int
main()
{
  plan_tests(kFixChecks + kFollowOn);

  Scene scene{Volume()};
  Run(scene, kOnce, true);
  {
    Scene climb{Volume()};
    Run(climb, kClimbOut, false);
  }

  TestExitAndFilter();

  {
    Scene stack(Circle(0, 0, 5000, 0, 1500), Circle(0, 0, 5000, 1700, 3000));
    Run(stack, kGap, false);
    Run(stack, kStack, true);
  }
  {
    Scene nested(Circle(0, 0, 8000, 1000, 2000),
                 Circle(0, 0, 2500, 1000, 2000));
    Run(nested, kNested, true);
  }
  {
    Scene adjacent(Circle(-4000, 0, 2000, 1000, 2000),
                   Circle(4000, 0, 2000, 1000, 2000));
    Run(adjacent, kAdjacent, true);
  }
  {
    Scene lens(Circle(0, 0, 3000, 1000, 2000),
               Circle(2000, 0, 3000, 1000, 2000));
    Run(lens, kLens, true);
  }

  TestAck();
  TestClearance();
  TestAckOther();
  TestClassDisabled();

  return exit_status();
}
