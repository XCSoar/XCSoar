// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* Integration tests for ProcessClearanceIntervals.
 *
 * These exercise the AirspaceWarningManager end-to-end via Update()
 * with carefully constructed airspace geometries and aircraft state.
 * Tests focus on outcomes that are observable from public warning
 * state: AirspaceWarning::IsCoveredByClearance, GetWarningState, and
 * GetSolution().elapsed_time / location.
 *
 * GLIDE prediction is the workhorse here because state.GetPredictedState
 * is deterministic from a single AircraftState (uses ground_speed/track/
 * vario directly). FILTER intervals on the first cycle are usually
 * degenerate (filter history empty); we don't assert on them. */

#include "Engine/Airspace/AirspaceCircle.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Stats/TaskStats.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Flat/FlatPoint.hpp"
#include "TransponderCode.hpp"
#include "TestUtil.hpp"

#include <chrono>
#include <memory>

namespace {

constexpr GeoPoint
P(double lon, double lat) noexcept
{
  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

AirspaceAltitude
Alt(double m) noexcept
{
  AirspaceAltitude a{};
  a.altitude = m;
  a.reference = AltitudeReference::MSL;
  return a;
}

AirspacePtr
MakeCircle(const GeoPoint &center, double radius_m,
           double base_m, double top_m)
{
  auto as = std::make_shared<AirspaceCircle>(center, radius_m);
  TransponderCode code;
  as->SetProperties("test", "", std::move(code),
                    AirspaceClass::RESTRICTED,
                    AirspaceClass::RESTRICTED,
                    Alt(base_m), Alt(top_m));
  return as;
}

AircraftState
MakeAircraft(const GeoPoint &loc, double altitude,
             Angle track, double ground_speed)
{
  AircraftState s;
  s.Reset();
  s.location = loc;
  s.altitude = altitude;
  s.altitude_agl = altitude;
  s.track = track;
  s.ground_speed = ground_speed;
  s.true_airspeed = ground_speed;
  s.vario = 0;
  s.netto_vario = 0;
  s.flying = true;
  return s;
}

AirspaceWarning *
GetWarning(AirspaceWarningManager &mgr, const AbstractAirspace &as)
{
  return mgr.GetWarningPtr(as);
}

}  // namespace

int
main()
{
  plan_tests(15);

  /* Place airspaces near 50N where 0.01 deg lon ~ 716m.
     We choose simple longitudinal layouts (heading east) so that
     path distances correspond directly to lon differences. */
  const GeoPoint origin = P(10.0, 50.0);

  GlidePolar polar(2.0);
  ok1(polar.IsValid());

  TaskStats task_stats;
  task_stats.reset();

  /* --- Scenario 1: INSIDE warning fully covered by clearance ---
     Aircraft inside both W and C; C fully encloses W horizontally.
     Step 1 should subtract C's interval from W's, find no surviving
     residual, and set covered_by_clearance on W. */
  {
    Airspaces airspaces;
    auto w = MakeCircle(origin, 2000.0, 0.0, 3000.0);
    auto c = MakeCircle(origin, 5000.0, 0.0, 3000.0);
    airspaces.Add(w);
    airspaces.Add(c);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(c, true);
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *w_warn = GetWarning(mgr, *w);
    ok1(w_warn != nullptr);
    ok1(w_warn != nullptr && w_warn->IsCoveredByClearance());
  }

  /* --- Scenario 2: INSIDE warning partially covered ---
     Aircraft inside W and C, both centred at origin; C has a smaller
     radius than W, so the eastbound path exits C while still inside W.
     Step 1 should leave a residual fragment and downgrade W to a
     predicted-method state. */
  {
    Airspaces airspaces;
    auto w = MakeCircle(origin, 3000.0, 0.0, 3000.0);
    /* C: 500m radius around origin.  Path exits C at 500m east while
       W extends to ~3000m east. */
    auto c = MakeCircle(origin, 500.0, 0.0, 3000.0);
    airspaces.Add(w);
    airspaces.Add(c);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 50.0);
    mgr.Reset(state);
    mgr.SetCleared(c, true);
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *w_warn = GetWarning(mgr, *w);
    ok1(w_warn != nullptr);
    /* After partial coverage, W must NOT be fully suppressed. */
    ok1(w_warn != nullptr && !w_warn->IsCoveredByClearance());
    /* And it must be downgraded out of INSIDE (no longer the
       leading INSIDE warning since clearance covered the near
       portion). */
    ok1(w_warn != nullptr &&
        w_warn->GetWarningState() < AirspaceWarning::WARNING_INSIDE);
  }

  /* --- Scenario 3: NEAR warning behind cleared (just outside) ---
     Aircraft approaches W from the west; a cleared C exists nearby
     but does not horizontally overlap the path's entry into W.
     Warning should fire unchanged on W. */
  {
    Airspaces airspaces;
    /* W: small restricted circle 500m east of aircraft start. */
    auto w = MakeCircle(P(10.007, 50.0), 200.0, 0.0, 3000.0);
    /* C: cleared circle north of the path; doesn't touch the path. */
    auto c = MakeCircle(P(10.005, 50.005), 300.0, 0.0, 3000.0);
    airspaces.Add(w);
    airspaces.Add(c);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 50.0);
    mgr.Reset(state);
    mgr.SetCleared(c, true);
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *w_warn = GetWarning(mgr, *w);
    ok1(w_warn != nullptr);
    ok1(w_warn != nullptr && !w_warn->IsCoveredByClearance());
  }

  /* --- Scenario 4: NEAR warning whose entry is inside cleared ---
     Aircraft approaches W from the west; C horizontally covers the
     near portion of W's predicted-path interval but not the far end.
     Step 2 should clip W's interval and rebuild its solution at the
     residual entry, not the original entry. */
  {
    Airspaces airspaces;
    /* W: 1500m radius around (10.008, 50.0).  Entry from origin
       is at lon ~ 10.00768 (centre minus radius), exit at ~10.00832.
       Distance from origin: ~550m (entry) to ~590m (exit). */
    auto w = MakeCircle(P(10.008, 50.0), 1500.0, 0.0, 3000.0);
    /* C: cleared circle covering W's entry side, 1000m radius around
       (10.006, 50.0). */
    auto c = MakeCircle(P(10.006, 50.0), 1000.0, 0.0, 3000.0);
    airspaces.Add(w);
    airspaces.Add(c);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 50.0);
    mgr.Reset(state);
    mgr.SetCleared(c, true);
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *w_warn = GetWarning(mgr, *w);
    ok1(w_warn != nullptr);
    if (w_warn != nullptr) {
      /* Not fully covered (W exits C). */
      ok1(!w_warn->IsCoveredByClearance());
      /* Solution location should be past C's east boundary, i.e.
         east of (10.006, 50.0) + 1000m east ~= 10.014 degrees.
         The new entry must be east of the original W entry on the
         clipped interval. */
      const auto sol_loc = w_warn->GetSolution().location;
      /* Original W entry is at lon ~10.00768.  After clipping by C
         (which extends to ~10.00697 east), the residual W entry is
         where C ends and W's interval picks up. But since C and W
         overlap, the residual starts at C's east boundary if that
         lies inside W. */
      /* Loose check: residual entry is east of W's original entry. */
      ok1(sol_loc.longitude.Degrees() > 10.0076);
    }
  }

  /* --- Scenario 5: multiple cleared creating a hole; nearest
         fragment kept (above kMinFragmentLength threshold) ---
     Two cleared airspaces inside W with a gap > 50m between them on
     the predicted path. Step 2 should keep the near fragment. */
  {
    Airspaces airspaces;
    /* W: 5000m radius around (10.020, 50.0). Eastbound path from
       origin enters W well before the centre. */
    auto w = MakeCircle(P(10.020, 50.0), 5000.0, 0.0, 3000.0);
    /* Two cleared circles inside W, both fully horizontally inside W,
       with a gap on the path between them. */
    auto c1 = MakeCircle(P(10.018, 50.0), 700.0, 0.0, 3000.0);
    auto c2 = MakeCircle(P(10.022, 50.0), 700.0, 0.0, 3000.0);
    airspaces.Add(w);
    airspaces.Add(c1);
    airspaces.Add(c2);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    /* Lengthen warning_time so the path reaches into W. */
    cfg.warning_time = std::chrono::seconds{120};
    AirspaceWarningManager mgr(cfg, airspaces);

    /* Place aircraft fast and approaching so the predicted GLIDE
       path reaches well into W. */
    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 60.0);
    mgr.Reset(state);
    mgr.SetCleared(c1, true);
    mgr.SetCleared(c2, true);
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *w_warn = GetWarning(mgr, *w);
    ok1(w_warn != nullptr);
    if (w_warn != nullptr) {
      /* Not fully covered: there's open W between origin and c1,
         and between c1 and c2 if not overlapping. */
      ok1(!w_warn->IsCoveredByClearance());
    }
  }

  /* --- Scenario 6: traversal across cleared A's entry boundary ---
     A (cleared) starts slightly before B (non-cleared) along the
     path. Across many cycles, B's warning must stay suppressed.
     Regression coverage for the "warning briefly appears for one
     cycle as the plane enters A" glitch. */
  {
    Airspaces airspaces;
    /* A: cleared, large. Centre at (10.01, 50.0), radius 500m.
       Entry on path at ~10.0029, exit at ~10.0171. */
    auto a = MakeCircle(P(10.01, 50.0), 500.0, 0.0, 3000.0);
    /* B: non-cleared, smaller, fully inside A.  Entry on path
       at ~10.0098, exit at ~10.0107. */
    auto b = MakeCircle(P(10.01025, 50.0), 30.0, 0.0, 3000.0);
    airspaces.Add(a);
    airspaces.Add(b);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(a, true);

    bool ever_spuriously_warned = false;
    /* Step plane eastward in increments fine enough to hit cycles
       both before and right after A's entry. */
    for (int i = 0; i < 60; ++i) {
      state.location = P(10.0 + 0.00005 * i, 50.0);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});

      auto *bw = mgr.GetWarningPtr(*b);
      if (bw != nullptr
          && bw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
          && !bw->IsCoveredByClearance()
          && bw->IsAckExpired()) {
        printf("step %d lon=%.5f: B fires unexpectedly "
               "(state=%d covered=%d)\n",
               i, state.location.longitude.Degrees(),
               bw->GetWarningState(),
               bw->IsCoveredByClearance());
        ever_spuriously_warned = true;
      }
    }
    ok1(!ever_spuriously_warned);
  }

  /* --- Scenario 7: state.location exactly on cleared A's entry
         boundary ---
     make sure no glitch occurs. */
  {
    Airspaces airspaces;
    /* A: cleared, large, centred east of origin. */
    const GeoPoint a_center = P(10.01, 50.0);
    constexpr double a_radius = 500.0;
    auto a = MakeCircle(a_center, a_radius, 0.0, 3000.0);
    /* B: non-cleared, fully inside A on the predicted path. */
    auto b = MakeCircle(P(10.011, 50.0), 60.0, 0.0, 3000.0);
    airspaces.Add(a);
    airspaces.Add(b);
    airspaces.Optimise();

    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    auto state = MakeAircraft(origin, 1500.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(a, true);

    /* Place the aircraft exactly on A's algebraic western flat
       boundary: unproject (f_center.x - f_radius, f_center.y) so
       AirspaceCircle::Intersects gets f_p1 == f_state and the
       parametric position for that intersection is zero modulo
       floating-point noise. */
    const FlatProjection &proj = airspaces.GetProjection();
    const FlatPoint f_center = proj.ProjectFloat(a_center);
    const double f_radius = proj.ProjectRangeFloat(a_center, a_radius);
    state.location = proj.Unproject(FlatPoint{f_center.x - f_radius,
                                              f_center.y});
    state.time += std::chrono::seconds{1};
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *bw = mgr.GetWarningPtr(*b);
    /* B exists because its path-interval is non-empty, but it
       must be covered by A's clearance and therefore silent. */
    ok1(bw == nullptr || bw->IsCoveredByClearance() ||
        !bw->IsAckExpired());
  }

  return exit_status();
}
