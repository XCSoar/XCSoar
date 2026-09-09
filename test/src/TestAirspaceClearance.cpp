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
#include "Engine/Airspace/AirspacePolygon.hpp"
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
#include "Geo/GeoVector.hpp"
#include "TransponderCode.hpp"
#include "TestUtil.hpp"

#include <chrono>
#include <cmath>
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

AirspacePtr
MakeRectangle(double west_lon, double south_lat,
              double east_lon, double north_lat,
              double base_m, double top_m)
{
  std::vector<GeoPoint> pts = {
    P(west_lon, south_lat),
    P(east_lon, south_lat),
    P(east_lon, north_lat),
    P(west_lon, north_lat),
  };
  auto as = std::make_shared<AirspacePolygon>(pts);
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
  plan_tests(33);

  /* Place airspaces near 50N where 0.01 deg lon ~ 716m.
     We choose simple longitudinal layouts (heading east) so that
     path distances correspond directly to lon differences. */
  const GeoPoint origin = P(10.0, 50.0);

  GlidePolar polar(2.0);
  ok1(polar.IsValid());

  TaskStats task_stats;
  task_stats.reset();

  /* --- DistanceToBoundary: circle analytic, polygon via integer
     grid (accurate to about one ~111 m cell) --- */
  {
    Airspaces airspaces;
    auto circle = MakeCircle(origin, 5000.0, 0.0, 3000.0);
    /* rectangle ~7.2 km (E-W) x ~11.1 km (N-S) around origin */
    auto rect = MakeRectangle(9.95, 49.95, 10.05, 50.05,
                              0.0, 3000.0);
    airspaces.Add(circle);
    airspaces.Add(rect);
    airspaces.Optimise();
    const FlatProjection &proj = airspaces.GetProjection();

    /* Deep inside the circle: distance to the rim, not zero. */
    ok1(fabs(circle->DistanceToBoundary(origin, proj) - 5000.0) < 1.0);
    /* 100 m inside / 200 m outside the rim.  GeoVector::EndPoint
       places points with the accurate great-circle formula while the
       circle uses the simplified DistanceS (~0.3 % short at 5 km),
       hence the ~20 m slack. */
    const GeoPoint rim_in =
      GeoVector(4900.0, Angle::Degrees(90.0)).EndPoint(origin);
    const GeoPoint rim_out =
      GeoVector(5200.0, Angle::Degrees(90.0)).EndPoint(origin);
    ok1(fabs(circle->DistanceToBoundary(rim_in, proj) - 100.0) < 20.0);
    ok1(fabs(circle->DistanceToBoundary(rim_out, proj) - 200.0) < 20.0);

    /* Polygon: deep inside (origin, ~3.6 km from the west/east
       edges) must report a large distance, not zero. */
    ok1(rect->DistanceToBoundary(origin, proj) > 3000.0);
    /* 100 m inside and 200 m outside the west edge; integer grid
       rounding allows about one cell of slack. */
    const GeoPoint west_mid = P(9.95, 50.0);
    const GeoPoint poly_in =
      GeoVector(100.0, Angle::Degrees(90.0)).EndPoint(west_mid);
    const GeoPoint poly_out =
      GeoVector(200.0, Angle::Degrees(270.0)).EndPoint(west_mid);
    ok1(rect->DistanceToBoundary(poly_in, proj) < 350.0);
    ok1(rect->DistanceToBoundary(poly_out, proj) < 550.0);
  }

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

  /* --- Scenario 8: polygon analogue of Scenario 6 ---
     Same setup but with rectangular polygons. Regression coverage
     for the polygon-edge analogue of the circle boundary glitch:
     FlatRay::DistinctIntersection rejects t==0 on the edge that
     contains state.location, which used to corrupt A's interval
     and unmask B for a single cycle as the aircraft crossed A's
     western edge. */
  {
    Airspaces airspaces;
    /* A: cleared rectangle, ~10.003..10.017 east-west,
       50.0 - 0.005 .. 50.0 + 0.005 (~700m N/S). */
    auto a = MakeRectangle(10.003, 49.995, 10.017, 50.005,
                           0.0, 3000.0);
    /* B: smaller non-cleared rectangle fully inside A on the path. */
    auto b = MakeRectangle(10.0098, 49.999, 10.0107, 50.001,
                           0.0, 3000.0);
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
    for (int i = 0; i < 80; ++i) {
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

  /* --- Scenario 9: state.location exactly on cleared rectangle
         A's western edge ---
     Polygon analogue of Scenario 7: place state at the integer-
     projected western edge of A so AirspacePolygon::Intersects sees
     a t==0 hit that DistinctIntersection used to drop. */
  {
    Airspaces airspaces;
    constexpr double a_west = 10.005;
    constexpr double a_east = 10.020;
    auto a = MakeRectangle(a_west, 49.995, a_east, 50.005,
                           0.0, 3000.0);
    auto b = MakeRectangle(10.010, 49.999, 10.012, 50.001,
                           0.0, 3000.0);
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

    /* Snap state to the integer-projected western edge of A so
       AirspacePolygon::Intersects encounters a t==0 case. */
    const FlatProjection &proj = airspaces.GetProjection();
    const FlatGeoPoint f_nw =
      proj.ProjectInteger(P(a_west, 50.005));
    const FlatGeoPoint f_sw =
      proj.ProjectInteger(P(a_west, 49.995));
    const FlatGeoPoint f_on_edge{f_nw.x,
                                 (f_nw.y + f_sw.y) / 2};
    state.location = proj.Unproject(f_on_edge);
    state.time += std::chrono::seconds{1};
    mgr.Update(state, polar, task_stats, false,
               std::chrono::seconds{1});

    auto *bw = mgr.GetWarningPtr(*b);
    ok1(bw == nullptr || bw->IsCoveredByClearance() ||
        !bw->IsAckExpired());
  }

  /* --- Scenario 10: nested airspaces sharing an exit edge snapped
         together by the integer projection ---
     B (non-cleared) sits a few metres inside cleared A and shares
     A's eastern edge.  Flying east while inside both, the coarse
     integer projection snaps the aircraft onto the shared edge, so
     B's exit crossing is dropped (t==0) and both exits coincide.
     B must stay suppressed throughout, never flashing a spurious
     INSIDE or downgraded approach warning. */
  {
    auto a = MakeRectangle(10.000, 49.990, 10.020, 50.010,
                           0.0, 3000.0);
    auto b = MakeRectangle(10.010, 49.990, 10.020 - 0.0008,
                           50.000, 0.0, 3000.0);
    Airspaces airspaces;
    airspaces.Add(a);
    airspaces.Add(b);
    airspaces.Optimise();
    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);
    auto state = MakeAircraft(P(10.015, 49.995), 1500.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(a, true);

    bool ever_spuriously_warned = false;
    for (int i = 0; i < 40; ++i) {
      state.location = P(10.015 + 0.0005 * i, 49.995);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *bw = mgr.GetWarningPtr(*b);
      if (bw != nullptr
          && bw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
          && !bw->IsCoveredByClearance()
          && bw->IsAckExpired()) {
        printf("step %d lon=%.5f: B fires unexpectedly (state=%d)\n",
               i, state.location.longitude.Degrees(),
               bw->GetWarningState());
        ever_spuriously_warned = true;
      }
    }
    ok1(!ever_spuriously_warned);
  }


  /* --- Scenario 11: exit across a near-coincident shared boundary
         (real CTR LAHR / CTR SEKTOR ALTDORF geometry) ---
     These two airspaces from DE-ASP-National-OpenAIP.txt share the
     exact SE vertex 48:15:41N 007:49:35E, and Altdorf's NE edge runs
     ~4 m inside Lahr's SE edge, far below the ~111 m integer-
     projection grid.  With Lahr cleared, flying east out of both must
     never raise an Altdorf warning: the snapped-together exits make
     Altdorf's inside interval short (< kMinFragmentLength) but it is
     still fully consumed by Lahr's clearance.  Regression for the
     one-cycle INSIDE-then-NEAR glitch seen on exit. */
  {
    auto DMS = [](int d, int m, double s) {
      return d + m / 60.0 + s / 3600.0;
    };
    std::vector<GeoPoint> lahr = {
      P(DMS(7,44,9),  DMS(48,21,40)),
      P(DMS(7,44,40), DMS(48,19,38)),
      P(DMS(7,44,2),  DMS(48,19,5)),
      P(DMS(7,41,57), DMS(48,18,23)),
      P(DMS(7,41,56), DMS(48,18,22)),
      P(DMS(7,49,35), DMS(48,15,41)),
      P(DMS(7,57,50), DMS(48,25,55)),
      P(DMS(7,49,52), DMS(48,28,45)),
    };
    std::vector<GeoPoint> altdorf = {
      P(DMS(7,49,33), DMS(48,17,54)),
      P(DMS(7,48,34), DMS(48,16,3)),
      P(DMS(7,49,35), DMS(48,15,41)),
      P(DMS(7,51,5),  DMS(48,17,33)),
    };
    auto lahr_as = std::make_shared<AirspacePolygon>(lahr);
    auto alt_as = std::make_shared<AirspacePolygon>(altdorf);
    lahr_as->SetProperties("CTR LAHR", "", TransponderCode{},
                           AirspaceClass::CTR, AirspaceClass::CTR,
                           Alt(0.0), Alt(762.0));
    alt_as->SetProperties("ALTDORF", "", TransponderCode{},
                          AirspaceClass::CTR, AirspaceClass::CTR,
                          Alt(0.0), Alt(609.0));
    Airspaces airspaces;
    airspaces.Add(lahr_as);
    airspaces.Add(alt_as);
    airspaces.Optimise();
    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);
    const double lat = 48.277;
    auto state = MakeAircraft(P(7.835, lat), 400.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(lahr_as, true);

    bool ever_spuriously_warned = false;
    for (int i = 0; i < 40; ++i) {
      state.location = P(7.835 + 0.0002 * i, lat);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *aw = mgr.GetWarningPtr(*alt_as);
      if (aw != nullptr
          && aw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
          && !aw->IsCoveredByClearance()
          && aw->IsAckExpired()) {
        printf("step %d lon=%.5f: Altdorf fires unexpectedly "
               "(state=%d)\n", i,
               state.location.longitude.Degrees(),
               aw->GetWarningState());
        ever_spuriously_warned = true;
      }
    }
    ok1(!ever_spuriously_warned);
  }


  /* --- Scenario 12: non-cleared airspace genuinely protruding past
         the cleared one must still warn (no over-suppression) ---
     B extends ~500 m east beyond cleared A, well past the corridor
     tolerance (one ~111 m projection grid cell), so there is a real
     region inside B but outside A.  When the aircraft is in that
     region (left the clearance, still in restricted airspace), B's
     INSIDE warning must fire.  (A sub-tolerance protrusion would by
     design be treated as a digitisation artifact and suppressed.) */
  {
    auto a = MakeRectangle(10.000, 49.990, 10.020, 50.010,
                           0.0, 3000.0);
    auto b = MakeRectangle(10.010, 49.990, 10.020 + 0.007,
                           50.000, 0.0, 3000.0);
    Airspaces airspaces;
    airspaces.Add(a);
    airspaces.Add(b);
    airspaces.Optimise();
    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);
    auto state = MakeAircraft(P(10.015, 49.995), 1500.0,
                              Angle::Degrees(90.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(a, true);

    bool warned_in_protrusion = false;
    for (int i = 0; i < 40; ++i) {
      state.location = P(10.015 + 0.0005 * i, 49.995);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *bw = mgr.GetWarningPtr(*b);
      /* Aircraft inside B but outside the cleared A: the protrusion. */
      if (!a->Inside(state.location) && b->Inside(state.location)
          && bw != nullptr
          && bw->GetWarningState() == AirspaceWarning::WARNING_INSIDE
          && !bw->IsCoveredByClearance()
          && bw->IsAckExpired())
        warned_in_protrusion = true;
    }
    ok1(warned_in_protrusion);
  }

  /* --- Scenario 13: flying parallel to a slanted near-coincident
         shared boundary (integer-projection non-nesting) ---
     Real CTR LAHR (cleared) / CTR SEKTOR ALTDORF geometry: their
     shared SE edge is *slanted*, so Lahr's and Altdorf's separate but
     ~coincident edges round to different integer lines.  Flying nearly
     parallel to that edge, the same predicted path crosses them at
     different integer distances, so Lahr's interval fails to contain
     Altdorf's and subtraction leaves a phantom residual.  Without the
     float-geometry cross-check, Altdorf would warn here even though it
     is fully inside cleared Lahr (regression for that bug).  Axis-
     aligned rectangles do NOT reproduce this (their shared edge rounds
     to a single integer line), hence the real slanted polygons. */
  {
    auto DMS = [](int d, int m, double s) {
      return d + m / 60.0 + s / 3600.0;
    };
    std::vector<GeoPoint> lahr = {
      P(DMS(7,44,9),  DMS(48,21,40)),
      P(DMS(7,44,40), DMS(48,19,38)),
      P(DMS(7,44,2),  DMS(48,19,5)),
      P(DMS(7,41,57), DMS(48,18,23)),
      P(DMS(7,41,56), DMS(48,18,22)),
      P(DMS(7,49,35), DMS(48,15,41)),
      P(DMS(7,57,50), DMS(48,25,55)),
      P(DMS(7,49,52), DMS(48,28,45)),
    };
    std::vector<GeoPoint> altdorf = {
      P(DMS(7,49,33), DMS(48,17,54)),
      P(DMS(7,48,34), DMS(48,16,3)),
      P(DMS(7,49,35), DMS(48,15,41)),
      P(DMS(7,51,5),  DMS(48,17,33)),
    };
    auto lahr_as = std::make_shared<AirspacePolygon>(lahr);
    auto alt_as = std::make_shared<AirspacePolygon>(altdorf);
    lahr_as->SetProperties("CTR LAHR", "", TransponderCode{},
                           AirspaceClass::CTR, AirspaceClass::CTR,
                           Alt(0.0), Alt(762.0));
    alt_as->SetProperties("ALTDORF", "", TransponderCode{},
                          AirspaceClass::CTR, AirspaceClass::CTR,
                          Alt(0.0), Alt(609.0));
    Airspaces airspaces;
    airspaces.Add(lahr_as);
    airspaces.Add(alt_as);
    airspaces.Optimise();
    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);
    /* Start inside both just NW of the shared C->D edge and fly along
       its ~28 deg (NNE) bearing, parallel to the boundary. */
    auto state = MakeAircraft(P(7.8380, 48.27695), 400.0,
                              Angle::Degrees(28.0), 30.0);
    mgr.Reset(state);
    mgr.SetCleared(lahr_as, true);

    bool ever_spuriously_warned = false;
    for (int i = 0; i < 40; ++i) {
      /* Step NNE parallel to the C->D edge. */
      state.location = P(7.8380 + 0.00012 * i,
                         48.27695 + 0.00023 * i);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *aw = mgr.GetWarningPtr(*alt_as);
      /* Only count cycles where the aircraft is genuinely inside both
         (Altdorf fully within cleared Lahr, must stay suppressed). */
      if (lahr_as->Inside(state.location)
          && alt_as->Inside(state.location)
          && aw != nullptr
          && aw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
          && !aw->IsCoveredByClearance()
          && aw->IsAckExpired()) {
        printf("step %d lon=%.5f lat=%.5f: Altdorf fires "
               "unexpectedly (state=%d)\n", i,
               state.location.longitude.Degrees(),
               state.location.latitude.Degrees(),
               aw->GetWarningState());
        ever_spuriously_warned = true;
      }
    }
    ok1(!ever_spuriously_warned);
  }

  /* --- Scenarios 14-18: thin sliver between near-coincident
         boundaries (clearance-test2 geometry analogue) ---
     B (non-cleared) with cleared A nested inside; A's south edge
     lies 1 arcsec (~31 m) north of B's south edge, leaving a thin
     artifact strip of B that hugs A's boundary.  B extends ~3.3 km
     north of A and ~2.9 km east of A (genuine interior). */
  {
    constexpr double kArcsec = 1.0 / 3600.0;  /* ~30.9 m of lat */

    const auto make_airspaces = [&](Airspaces &airspaces,
                                    AirspacePtr &a, AirspacePtr &b) {
      b = MakeRectangle(10.000, 49.950, 10.100, 50.020,
                        0.0, 3000.0);
      a = MakeRectangle(10.010, 49.950 + kArcsec, 10.060, 49.990,
                        0.0, 3000.0);
      airspaces.Add(a);
      airspaces.Add(b);
      airspaces.Optimise();
    };

    /* March the aircraft from @p start along @p step_bearing and
       report whether B's warning ever alerted. */
    const auto march_alerts = [&](const GeoPoint &start,
                                  Angle track, Angle step_bearing,
                                  double step_m, int steps) {
      Airspaces airspaces;
      AirspacePtr a, b;
      make_airspaces(airspaces, a, b);
      AirspaceWarningConfig cfg;
      cfg.SetDefaults();
      AirspaceWarningManager mgr(cfg, airspaces);
      auto state = MakeAircraft(start, 1500.0, track, 40.0);
      mgr.Reset(state);
      mgr.SetCleared(a, true);

      bool alerted = false;
      for (int i = 0; i < steps; ++i) {
        state.location =
          GeoVector(step_m * i, step_bearing).EndPoint(start);
        state.time += std::chrono::seconds{1};
        mgr.Update(state, polar, task_stats, false,
                   std::chrono::seconds{1});
        auto *bw = mgr.GetWarningPtr(*b);
        if (bw != nullptr
            && bw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
            && !bw->IsCoveredByClearance()
            && bw->IsAckExpired()) {
          printf("step %d lon=%.5f lat=%.5f: B fires (state=%d)\n",
                 i, state.location.longitude.Degrees(),
                 state.location.latitude.Degrees(),
                 bw->GetWarningState());
          alerted = true;
        }
      }
      return alerted;
    };

    /* Scenario 14: northbound entry through the sliver into A (the
       original clearance-test2 symptom: INSIDE warning popping up
       in the strip with no preceding approach warning).  March from
       south of B across the sliver well into A. */
    ok1(!march_alerts(P(10.030, 49.9495), Angle::Degrees(0.0),
                      Angle::Degrees(0.0), 11.0, 35));

    /* Scenario 15: southbound exit from A through the sliver and
       out of B (no one-cycle exit blip). */
    ok1(!march_alerts(P(10.030, 49.9520), Angle::Degrees(180.0),
                      Angle::Degrees(180.0), 11.0, 35));

    /* Scenario 16: genuine deep intrusion guard.  Aircraft inside B
       ~1.4 km east of A (far beyond the corridor tolerance), heading
       west: the INSIDE warning must fire. */
    {
      Airspaces airspaces;
      AirspacePtr a, b;
      make_airspaces(airspaces, a, b);
      AirspaceWarningConfig cfg;
      cfg.SetDefaults();
      AirspaceWarningManager mgr(cfg, airspaces);
      auto state = MakeAircraft(P(10.080, 49.970), 1500.0,
                                Angle::Degrees(270.0), 40.0);
      mgr.Reset(state);
      mgr.SetCleared(a, true);
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *bw = mgr.GetWarningPtr(*b);
      ok1(bw != nullptr
          && bw->GetWarningState() == AirspaceWarning::WARNING_INSIDE
          && !bw->IsCoveredByClearance()
          && bw->IsAckExpired());
    }

    /* Scenario 17: shallow corridor.  Flying east *along* the sliver
       (predicted path stays in the strip, hugging A's boundary, for
       over a kilometre).  No short fragment ever exists here, so the
       fragment-length logic alone cannot suppress it; the corridor
       walk must. */
    ok1(!march_alerts(P(10.020, 49.950 + kArcsec / 2),
                      Angle::Degrees(90.0), Angle::Degrees(90.0),
                      14.0, 30));

    /* Scenario 18: direction guard.  Starting in the sliver near A's
       SE corner heading east, the predicted path leaves the corridor
       into B's genuine interior: B must alert. */
    ok1(march_alerts(P(10.0550, 49.950 + kArcsec / 2),
                     Angle::Degrees(90.0), Angle::Degrees(90.0),
                     14.0, 40));
  }

  /* --- Scenario 19: circle variant of the sliver (B2/A2 analogue
         of clearance-test2) ---
     Cleared circle A2 nested in non-cleared circle B2 with
     near-coincident north rims (~80 m gap, below the ~111 m
     tolerance).  Northbound through A2, the rim sliver and out of
     B2: B2 must never alert.  Exercises the analytic
     AirspaceCircle::DistanceToBoundary in gate and samples. */
  {
    const GeoPoint b2_center = P(10.0, 49.90);
    const GeoPoint a2_center =
      GeoVector(1920.0, Angle::Degrees(0.0)).EndPoint(b2_center);
    Airspaces airspaces;
    auto b2 = MakeCircle(b2_center, 5000.0, 0.0, 3000.0);
    auto a2 = MakeCircle(a2_center, 3000.0, 0.0, 3000.0);
    airspaces.Add(a2);
    airspaces.Add(b2);
    airspaces.Optimise();
    AirspaceWarningConfig cfg;
    cfg.SetDefaults();
    AirspaceWarningManager mgr(cfg, airspaces);

    const GeoPoint start =
      GeoVector(4800.0, Angle::Degrees(0.0)).EndPoint(b2_center);
    auto state = MakeAircraft(start, 1500.0,
                              Angle::Degrees(0.0), 40.0);
    mgr.Reset(state);
    mgr.SetCleared(a2, true);

    bool ever_spuriously_warned = false;
    for (int i = 0; i < 25; ++i) {
      state.location =
        GeoVector(16.0 * i, Angle::Degrees(0.0)).EndPoint(start);
      state.time += std::chrono::seconds{1};
      mgr.Update(state, polar, task_stats, false,
                 std::chrono::seconds{1});
      auto *bw = mgr.GetWarningPtr(*b2);
      if (bw != nullptr
          && bw->GetWarningState() > AirspaceWarning::WARNING_CLEAR
          && !bw->IsCoveredByClearance()
          && bw->IsAckExpired()) {
        printf("step %d lat=%.5f: B2 fires unexpectedly (state=%d)\n",
               i, state.location.latitude.Degrees(),
               bw->GetWarningState());
        ever_spuriously_warned = true;
      }
    }
    ok1(!ever_spuriously_warned);
  }

  return exit_status();
}
