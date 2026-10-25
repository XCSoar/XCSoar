// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/Task/Stats/TaskStats.hpp"
#include "Engine/Task/TaskBehaviour.hpp"
#include "Geo/SpeedVector.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <cmath>

static GlideSettings settings;

static SpeedVector
WindFrom(double degrees, double speed) noexcept
{
  return SpeedVector(Angle::Degrees(degrees), speed);
}

static GlideState
Leg(double distance,
    double min_arrival,
    double altitude,
    SpeedVector wind) noexcept
{
  return GlideState(
    GeoVector(distance, Angle::Zero()), min_arrival, altitude, wind);
}

static GlideResult
Fly(const GlidePolar &polar, const GlideState &state) noexcept
{
  return MacCready::Solve(settings, polar, state);
}

/** Height and time follow the flown speed and the wind triangle. */
static void
CheckHeight(const GlidePolar &polar,
            const GlideState &state,
            const GlideResult &result) noexcept
{
  const double gs =
    state.CalcAverageSpeed(result.v_opt * polar.GetCruiseEfficiency());
  const double height =
    state.vector.distance / gs * polar.SinkRate(result.v_opt);

  ok1(result.IsFinalGlide());
  ok1(equals(result.height_glide, height));
  ok1(equals(result.time_elapsed.count(), state.vector.distance / gs));
}

static void
TestGlide() noexcept
{
  constexpr double distance = 20000;
  const GlideState high     = Leg(distance, 0, 6000, SpeedVector::Zero());
  const SpeedVector zero    = SpeedVector::Zero();

  GlidePolar mc0(0);
  const GlideResult at0 = Fly(mc0, high);
  CheckHeight(mc0, high, at0);
  ok1(std::fabs(at0.v_opt - mc0.GetVBestLD()) < 0.05);

  const MacCready mac(settings, mc0);
  for (const double v : {mc0.GetVMin(), mc0.GetVBestLD(), mc0.GetVMax()}) {
    const GlideResult other = mac.SolveGlide(high, v, false);
    ok1(at0.height_glide <= other.height_glide + 0.5);
  }

  const GlideState exact = Leg(distance, 0, at0.height_glide, zero);
  const GlideResult on   = Fly(mc0, exact);
  ok1(on.IsFinalGlide() && std::fabs(on.altitude_difference) < 1e-3);
  ok1(!Fly(mc0, Leg(distance, 0, at0.height_glide - 1, zero)).IsFinalGlide());

  GlidePolar mc2(2);
  const GlideResult at2 = Fly(mc2, high);
  CheckHeight(mc2, high, at2);
  ok1(equals(at2.v_opt, mc2.GetVBestLD()));
  ok1(at2.height_glide > at0.height_glide);

  /* Above best L/D, below the MacCready speed: still reachable,
     but the MC solution is a climb plus a glide. */
  const GlideState between = Leg(distance, 0, at0.height_glide + 40, zero);
  const GlideResult mid    = Fly(mc2, between);
  ok1(!mid.IsFinalGlide() && mid.height_climb > 0);
  ok1(MacCready::ArrivesAtBestGlide(mc2, settings, between));

  const GlideState probe = Leg(40000, 0, 1500, zero);
  ok1(Fly(mc0, probe).IsFinalGlide());
  ok1(!Fly(GlidePolar(4), probe).IsFinalGlide());
  double lo = 0, hi = 4;
  for (int i = 0; i < 20; ++i) {
    const double mid_mc = (lo + hi) / 2;
    if (Fly(GlidePolar(mid_mc), probe).IsFinalGlide())
      lo = mid_mc;
    else
      hi = mid_mc;
  }
  const GlideResult edge = Fly(GlidePolar(lo), probe);
  ok1(edge.IsFinalGlide() && std::fabs(edge.altitude_difference) < 40);
  ok1(!Fly(GlidePolar(hi), probe).IsFinalGlide());

  constexpr double windy_distance = 25000;
  const GlideState calm           = Leg(windy_distance, 0, 7000, zero);
  const GlideState head   = Leg(windy_distance, 0, 7000, WindFrom(0, 12));
  const GlideState tail   = Leg(windy_distance, 0, 7000, WindFrom(180, 12));
  const GlideState cross  = Leg(windy_distance, 0, 7000, WindFrom(90, 12));
  const GlideResult calm0 = Fly(mc0, calm);
  GlideResult head0       = Fly(mc0, head);
  const GlideResult tail0 = Fly(mc0, tail);
  GlideResult cross0      = Fly(mc0, cross);
  CheckHeight(mc0, head, head0);
  ok1(std::fabs(head0.v_opt - mc0.GetBestGlideRatioSpeed(head.head_wind)) <
      0.05);
  ok1(head0.height_glide > calm0.height_glide &&
      calm0.height_glide > tail0.height_glide);
  ok1(head0.v_opt > calm0.v_opt && calm0.v_opt > tail0.v_opt);

  /* Along-track best glide ignores a pure crosswind. The search
     still flies faster, because ground speed is the wind triangle. */
  ok1(cross0.v_opt > calm0.v_opt);
  ok1(cross0.height_glide > calm0.height_glide);

  GlideResult left = Fly(mc0, Leg(windy_distance, 0, 7000, WindFrom(270, 12)));
  cross0.CalcDeferred();
  left.CalcDeferred();
  head0.CalcDeferred();
  const double crab_right = cross0.cruise_track_bearing.AsDelta().Degrees();
  const double crab_left  = left.cruise_track_bearing.AsDelta().Degrees();
  ok1(std::fabs(crab_right) > 1 && std::fabs(crab_right + crab_left) < 0.05);
  ok1(std::fabs(head0.cruise_track_bearing.AsDelta().Degrees()) < 0.05);
  ok1(equals(left.height_glide, cross0.height_glide));

  const double need = calm0.height_glide;
  ok1(Fly(mc0, Leg(windy_distance, 0, need + 20, zero)).IsFinalGlide());
  ok1(!Fly(mc0, Leg(windy_distance, 0, need + 20, WindFrom(0, 12)))
         .IsFinalGlide());
  ok1(!Fly(mc0, Leg(windy_distance, 0, need - 50, zero)).IsFinalGlide());
  ok1(Fly(mc0, Leg(windy_distance, 0, need - 50, WindFrom(180, 12)))
        .IsFinalGlide());

  GlidePolar heavy(0);
  heavy.SetBallastLitres(heavy.GetMaxBallast());
  const GlideResult wet = Fly(heavy, high);
  ok1(wet.v_opt > at0.v_opt && wet.time_elapsed < at0.time_elapsed);
  const double ratio = wet.height_glide / at0.height_glide;
  ok1(ratio > 0.85 && ratio < 1.15);

  GlidePolar dirty(0);
  dirty.SetBugs(0.7);
  ok1(Fly(dirty, high).height_glide > at0.height_glide);

  GlidePolar high_alt(0);
  high_alt.SetDensityRatio(1.2);
  const GlideResult up = Fly(high_alt, high);
  CheckHeight(high_alt, high, up);
  ok1(up.v_opt > at0.v_opt);

  GlidePolar sloppy(2);
  sloppy.SetCruiseEfficiency(0.8);
  const GlideState room     = Leg(distance, 0, 8000, zero);
  const GlideResult perfect = Fly(mc2, room);
  const GlideResult off     = Fly(sloppy, room);
  CheckHeight(sloppy, room, off);
  ok1(equals(off.height_glide, perfect.height_glide / 0.8));

  GlidePolar limited(0);
  limited.SetMC(20);
  GlidePolar safety = limited;
  safety.SetMCLimitedByCruise(false);
  const GlideState long_glide = Leg(distance, 0, 15000, zero);
  const GlideResult capped    = Fly(limited, long_glide);
  const GlideResult free      = Fly(safety, long_glide);
  ok1(equals(capped.v_opt, limited.GetVMax()));
  ok1(free.v_opt > capped.v_opt && free.height_glide > capped.height_glide);

  GlidePolar mc1(1);
  const GlideResult wide    = Fly(mc1, Leg(distance, 0, 6000, zero));
  const GlideState low_gate = Leg(distance, 0, wide.height_glide + 400, zero);
  const GlideState high_gate =
    Leg(distance, 150, wide.height_glide + 400, zero);
  const GlideResult open   = Fly(mc1, low_gate);
  const GlideResult raised = Fly(mc1, high_gate);
  ok1(equals(open.altitude_difference, raised.altitude_difference + 150));
  ok1(equals(raised.GetArrivalAltitude(), open.GetArrivalAltitude()));
  const double arrival = open.GetArrivalAltitude();
  const double start   = wide.height_glide + 400;
  ok1(Fly(mc1, Leg(distance, arrival - 10, start, zero)).IsFinalGlide() &&
      !Fly(mc1, Leg(distance, arrival + 10, start, zero)).IsFinalGlide());
  ok1(equals(Fly(mc1, Leg(distance * 2, 0, 6000, zero)).height_glide,
             wide.height_glide * 2));
}

static void
TestBranches() noexcept
{
  GlidePolar polar(2);
  const double v            = polar.GetVBestLD();
  const double sink         = polar.SinkRate(v);
  constexpr double distance = 30000;
  const double need         = distance / v * sink;
  const SpeedVector zero    = SpeedVector::Zero();
  const GlideState half     = Leg(distance, 0, need / 2, zero);
  const MacCready mac(settings, polar);

  const GlideResult partial = mac.SolveGlide(half, v, true);
  ok1(equals(partial.vector.distance, distance / 2));
  ok1(is_zero(partial.altitude_difference));

  const GlideState negative = Leg(distance, 0, -50, zero);
  ok1(is_zero(mac.SolveGlide(negative, v, true).vector.distance));

  const GlideResult mixed = Fly(polar, half);
  ok1(!mixed.IsFinalGlide() && mixed.height_climb > 0);
  GlideState lifted = half;
  lifted.altitude_difference += -mixed.altitude_difference;
  ok1(std::fabs(Fly(polar, lifted).altitude_difference) < 1);
  GlideState above = lifted;
  above.altitude_difference += 1;
  ok1(Fly(polar, above).IsFinalGlide());

  const GlideResult straight = mac.SolveStraight(half);
  ok1(!straight.IsFinalGlide() && is_zero(straight.height_climb) &&
      equals(straight.vector.distance, distance));

  const GlideState easy    = Leg(distance, 0, need + 500, zero);
  const GlideResult solved = Fly(polar, easy);
  ok1(equals(solved.height_glide, mac.SolveStraight(easy).height_glide));
  ok1(equals(mac.SolveSink(easy, sink * 2).height_glide,
             solved.height_glide * 2));
  /* SolveSink keeps the full distance; partial is a SolveGlide flag. */
  ok1(equals(mac.SolveSink(half, sink).vector.distance, distance));

  constexpr double deficit = 600;
  const GlideResult climb  = Fly(polar, Leg(0, 0, -deficit, zero));
  ok1(!climb.IsFinalGlide());
  ok1(equals(climb.time_elapsed.count(), deficit / polar.GetMC()));
  ok1(Fly(polar, Leg(0, 400, 900, WindFrom(0, 80))).IsFinalGlide());
  ok1(Fly(GlidePolar(0), Leg(0, 0, -deficit, zero)).validity ==
      GlideResult::Validity::MACCREADY_INSUFFICIENT);

  const double wind = 8;
  GlidePolar weak(0.2);
  ok1(Fly(weak, Leg(0, 0, -deficit, WindFrom(0, wind))).validity ==
      GlideResult::Validity::MACCREADY_INSUFFICIENT);
  GlidePolar strong(4);
  const GlideResult drifted =
    Fly(strong, Leg(0, 0, -deficit, WindFrom(0, wind)));
  const double denom1     = strong.GetVBestLD() - wind;
  const double denom2     = strong.GetMC() * denom1 - wind;
  const double time_climb = deficit * denom1 / denom2;
  ok1(equals(drifted.time_elapsed.count(),
             time_climb + wind * time_climb / denom1));
  ok1(Fly(strong, Leg(0, 0, -deficit, WindFrom(0, strong.GetVBestLD() + 1)))
        .validity == GlideResult::Validity::WIND_EXCESSIVE);

  GlidePolar slow(0.1);
  const double hole         = (slow.GetVBestLD() + slow.GetVMax()) / 2;
  const GlideState hole_leg = Leg(15000, 0, 12000, WindFrom(0, hole));
  ok1(MacCready(settings, slow)
        .SolveGlide(hole_leg, slow.GetVBestLD(), false)
        .validity == GlideResult::Validity::WIND_EXCESSIVE);
  GlidePolar searching = slow;
  searching.SetMC(0);
  const GlideResult searched = Fly(searching, hole_leg);
  ok1(searched.IsOk() && searched.v_opt > hole);

  GlidePolar fast0(0);
  const GlideResult almost =
    Fly(fast0, Leg(5000, 0, 15000, WindFrom(0, fast0.GetVMax() - 1)));
  ok1(almost.IsOk() && almost.altitude_difference < 0);
  ok1(Fly(fast0, Leg(5000, 0, 15000, WindFrom(0, fast0.GetVMax() + 5)))
        .validity == GlideResult::Validity::WIND_EXCESSIVE);
  ok1(Fly(fast0, Leg(10000, 0, 8000, WindFrom(90, fast0.GetVMax() + 1)))
        .validity == GlideResult::Validity::WIND_EXCESSIVE);

  const GlidePolar bad      = GlidePolar::Invalid();
  const GlideState anywhere = Leg(10000, 0, 2000, zero);
  ok1(!Fly(bad, anywhere).IsDefined());
  ok1(!MacCready::ArrivesAtBestGlide(bad, settings, anywhere));
}

static void
TestSpeedToFly() noexcept
{
  GlidePolar polar(2);
  const GlideResult into = Fly(polar, Leg(20000, 0, 6000, WindFrom(0, 10)));
  const GlideResult away = Fly(polar, Leg(20000, 0, 6000, WindFrom(180, 10)));

  AircraftState ac;
  ac.Reset();
  const double cruise = polar.SpeedToFly(ac, into, false);
  ok1(std::fabs(cruise - polar.GetVBestLD()) < 0.05);
  ok1(std::fabs(polar.SpeedToFly(ac, away, false) - cruise) < 0.05);

  ac.netto_vario = -1.5;
  ok1(polar.SpeedToFly(ac, into, false) > cruise + 0.5);
  ok1(std::fabs(polar.SpeedToFly(ac, into, true) - cruise) < 0.05);

  ac.netto_vario       = 0.8;
  const double lifting = polar.SpeedToFly(ac, into, false);
  ok1(lifting < cruise - 0.5 && lifting > polar.GetVMin());

  ac.netto_vario = polar.GetMC() + polar.GetSMin() + 1;
  ok1(std::fabs(polar.SpeedToFly(ac, into, false) - polar.GetVMin()) < 0.01);
  ok1(std::fabs(polar.SpeedToFly(ac, into, true) - cruise) < 0.05);

  ac.netto_vario      = 0;
  ac.g_load           = 4;
  const double loaded = polar.SpeedToFly(ac, into, false);
  ok1(std::fabs(loaded - 2 * cruise) < 0.15);
  ok1(std::fabs(polar.SpeedToFly(ac, into, true) - cruise) < 0.05);
  ac.g_load = 0;
  ok1(std::fabs(polar.SpeedToFly(ac, into, false) - polar.GetVMin()) < 0.01);
  ac.g_load = -4;
  ok1(std::fabs(polar.SpeedToFly(ac, into, false) - loaded) < 0.15);

  ok1(polar.SpeedToFly(1.5, 0) > polar.SpeedToFly(0, 0) &&
      polar.SpeedToFly(0, 0) > polar.SpeedToFly(-0.4, 0));
  ok1(polar.SpeedToFly(0, 8) > polar.SpeedToFly(0, 0) &&
      polar.SpeedToFly(0, 0) > polar.SpeedToFly(0, -8));

  GlidePolar mc0(0);
  const GlideResult into0 = Fly(mc0, Leg(20000, 0, 6000, WindFrom(0, 10)));
  const GlideResult away0 = Fly(mc0, Leg(20000, 0, 6000, WindFrom(180, 10)));
  ac.Reset();
  const double stf_still = mc0.SpeedToFly(0, 0);
  ok1(mc0.SpeedToFly(ac, into0, false) > stf_still + 0.5);
  ok1(mc0.SpeedToFly(ac, away0, false) < stf_still - 0.5);
  ac.netto_vario = -1;
  ok1(mc0.SpeedToFly(ac, into0, false) >
      mc0.SpeedToFly(ac, into0, true) + 0.3);
}

static void
TestPossibleAndDrift() noexcept
{
  GlidePolar polar(2);
  ok1(!polar.IsGlidePossible(Leg(1000, 0, 0, SpeedVector::Zero())));

  constexpr double alt = 300;
  const double optimistic =
    (polar.GetVBestLD() + 25) * alt / polar.GetSBestLD();
  const SpeedVector head  = WindFrom(0, 25);
  const GlideState tricky = Leg(optimistic * 0.5, 0, alt, head);
  ok1(polar.IsGlidePossible(tricky) && !Fly(polar, tricky).IsFinalGlide());
  ok1(!polar.IsGlidePossible(Leg(optimistic * 2, 0, alt, head)));

  GlidePolar mc1(1);
  const GlideResult low      = Fly(mc1, Leg(80000, 0, 200, WindFrom(0, 8)));
  GlideSettings drift        = settings;
  drift.predict_wind_drift   = true;
  GlideSettings instant      = settings;
  instant.predict_wind_drift = false;
  ok1(!low.IsFinalGlide());
  ok1(std::fabs(low.SelectAltitudeDifference(drift) -
                low.SelectAltitudeDifference(instant)) > 1);
  const GlideResult final_glide =
    Fly(mc1, Leg(10000, 0, 5000, WindFrom(0, 8)));
  ok1(final_glide.IsFinalGlide() &&
      equals(final_glide.SelectAltitudeDifference(drift),
             final_glide.SelectAltitudeDifference(instant)));
}

static void
Added(const GlideResult &a, const GlideResult &b) noexcept
{
  const double margin =
    a.GetArrivalAltitude() -
    std::max(a.min_arrival_altitude, b.GetRequiredAltitudeWithDrift());
  GlideResult sum = a;
  sum.Add(b);
  ok1(equals(sum.altitude_difference, margin));
  ok1(sum.IsFinalGlide() == (margin >= 0));
}

static void
TestTwoLegs() noexcept
{
  GlidePolar polar(0);
  constexpr double distance = 10000;
  const SpeedVector zero    = SpeedVector::Zero();

  const GlideResult gate = Fly(polar, Leg(distance, 2000, 5000, zero));
  const GlideResult easy = Fly(polar, Leg(distance, 0, 8000, zero));
  GlideResult sum        = gate;
  sum.Add(easy);
  ok1(
    equals(sum.vector.distance, gate.vector.distance + easy.vector.distance));
  Added(gate, easy);

  const GlideResult first = Fly(polar, Leg(distance, 0, 5000, zero));
  const GlideResult finish =
    Fly(polar, Leg(distance, first.GetArrivalAltitude() - 80, 8000, zero));
  ok1(finish.GetRequiredAltitudeWithDrift() > first.GetArrivalAltitude());
  Added(first, finish);
}

static void
TestRisk() noexcept
{
  GlidePolar polar(2);

  ok1(equals(polar.GetRiskMC(0.2, 0.05), polar.GetMC()));
  ok1(equals(polar.GetRiskMC(0.25, 0.95), 0.5));
  ok1(equals(polar.GetRiskMC(-1, 0.95), 0));
  ok1(equals(polar.GetRiskMC(2, 0.95), polar.GetMC()));
  ok1(polar.GetRiskMC(0.2, 0.5) < polar.GetRiskMC(0.8, 0.5));
  ok1(equals(polar.GetRiskMC(1, 0.5), polar.GetMC()));

  GlidePolar cautious = polar;
  cautious.SetMC(polar.GetRiskMC(0.2, 0.95));
  const GlideState leg = Leg(20000, 0, 6000, SpeedVector::Zero());
  ok1(Fly(cautious, leg).GetArrivalAltitude() >
      Fly(polar, leg).GetArrivalAltitude());
}

static void
TestFlightMode() noexcept
{
  TaskStats stats;
  stats.reset();
  TaskBehaviour behaviour{};
  behaviour.auto_mc      = false;
  behaviour.auto_mc_mode = TaskBehaviour::AutoMCMode::FINALGLIDE;

  stats.total.solution_remaining.validity = GlideResult::Validity::OK;
  stats.total.solution_remaining.altitude_difference = 10;
  ok1(stats.calc_flight_mode(behaviour) && stats.flight_mode_final_glide);
  ok1(!stats.calc_flight_mode(behaviour));

  stats.total.solution_remaining.altitude_difference = -119;
  ok1(!stats.calc_flight_mode(behaviour) && stats.flight_mode_final_glide);
  stats.total.solution_remaining.altitude_difference = -120;
  ok1(stats.calc_flight_mode(behaviour) && !stats.flight_mode_final_glide);

  stats.total.solution_remaining.altitude_difference = 0;
  ok1(!stats.calc_flight_mode(behaviour));
  stats.total.solution_remaining.altitude_difference = 0.1;
  ok1(stats.calc_flight_mode(behaviour));

  stats.total.solution_remaining.altitude_difference = -500;
  stats.calc_flight_mode(behaviour);
  stats.total.solution_remaining.validity = GlideResult::Validity::NO_SOLUTION;
  stats.total.solution_remaining.altitude_difference = 1000;
  ok1(!stats.calc_flight_mode(behaviour));

  stats.total.solution_remaining.validity = GlideResult::Validity::OK;
  stats.total.solution_remaining.altitude_difference = -400;
  stats.total.solution_mc0.validity            = GlideResult::Validity::OK;
  stats.total.solution_mc0.altitude_difference = 40;
  behaviour.auto_mc                            = true;
  behaviour.auto_mc_mode = TaskBehaviour::AutoMCMode::CLIMBAVERAGE;
  ok1(!behaviour.IsAutoMCFinalGlideEnabled() &&
      behaviour.IsAutoMCCruiseEnabled() && !stats.calc_flight_mode(behaviour));
  behaviour.auto_mc_mode = TaskBehaviour::AutoMCMode::FINALGLIDE;
  ok1(behaviour.IsAutoMCFinalGlideEnabled() &&
      !behaviour.IsAutoMCCruiseEnabled());
  ok1(stats.calc_flight_mode(behaviour) && stats.flight_mode_final_glide);

  behaviour.auto_mc                                  = false;
  stats.total.solution_remaining.altitude_difference = -400;
  ok1(stats.calc_flight_mode(behaviour) && !stats.flight_mode_final_glide);

  behaviour.auto_mc      = true;
  behaviour.auto_mc_mode = TaskBehaviour::AutoMCMode::BOTH;
  stats.total.solution_mc0.altitude_difference = 40;
  ok1(behaviour.IsAutoMCFinalGlideEnabled() &&
      behaviour.IsAutoMCCruiseEnabled());
  ok1(stats.calc_flight_mode(behaviour) && stats.flight_mode_final_glide);

  stats.total.solution_remaining.altitude_difference = 800;
  stats.total.solution_mc0.altitude_difference       = -400;
  ok1(stats.calc_flight_mode(behaviour) && !stats.flight_mode_final_glide);
}

/**
 * What a pilot checks before leaving, and what the arrival does on
 * the way in. MacCready 0 is the backup bug on the final-glide bar.
 * The glide ratio is distance over height above the arrival. Extra
 * sink and a dirty wing spend a margin that was only just enough.
 */
static void
TestPilotWatch() noexcept
{
  GlidePolar day(2);
  GlidePolar flat(0);
  const SpeedVector zero    = SpeedVector::Zero();
  constexpr double distance = 20000;

  const GlideResult best    = Fly(flat, Leg(distance, 0, 6000, zero));
  const GlideState band     = Leg(distance, 0, best.height_glide + 40, zero);
  const GlideResult at_day  = Fly(day, band);
  const GlideResult at_zero = Fly(flat, band);
  ok1(at_zero.IsFinalGlide() &&
      at_zero.altitude_difference > at_day.altitude_difference);
  ok1(!at_day.IsFinalGlide());

  const double ld = flat.GetBestLD();
  ok1(std::fabs(distance / best.height_glide - ld) < 0.2);
  const double short_height = best.height_glide * 0.9;
  ok1(distance / short_height > ld);
  ok1(!Fly(flat, Leg(distance, 0, short_height, zero)).IsFinalGlide());

  const GlideResult into = Fly(flat, Leg(distance, 0, 6000, WindFrom(0, 12)));
  ok1(distance / into.height_glide < ld);

  const double v          = day.GetVBestLD();
  const double sink       = day.SinkRate(v);
  constexpr double pad    = 80;
  const GlideState padded = Leg(distance, 0, distance / v * sink + pad, zero);
  const GlideResult clean = Fly(day, padded);
  constexpr double extra  = 0.5;
  const GlideResult bleeding =
    MacCready::SolveSink(settings, day, padded, sink + extra);
  ok1(clean.IsFinalGlide() && equals(clean.altitude_difference, pad));
  ok1(equals(bleeding.height_glide,
             clean.height_glide + extra * clean.time_elapsed.count()));
  ok1(!bleeding.IsFinalGlide());

  GlidePolar bugs = flat;
  bugs.SetBugs(0.7);
  const GlideState just = Leg(distance, 0, best.height_glide, zero);
  ok1(Fly(flat, just).IsFinalGlide());
  ok1(!Fly(bugs, just).IsFinalGlide());
}

/**
 * Cruise-climb below the target, glide-then-cruise when only part of
 * the height is available, drift during that climb, and the
 * instantaneous-speed blend.
 */
static void
TestStrategies() noexcept
{
  GlidePolar polar(2);
  const double v         = polar.GetVBestLD();
  const double sink      = polar.GetSBestLD();
  const double mc        = polar.GetMC();
  const double k         = 1 + sink / mc;
  const SpeedVector zero = SpeedVector::Zero();

  /* Below the arrival height the whole leg is cruise-climb. */
  constexpr double distance = 20000;
  constexpr double deficit  = 400;
  const GlideResult cruise  = Fly(polar, Leg(distance, 0, -deficit, zero));
  ok1(cruise.IsOk() && !cruise.IsFinalGlide());
  ok1(equals(cruise.vector.distance, distance));
  ok1(equals(cruise.height_glide, distance / v * sink));
  ok1(equals(cruise.height_climb, cruise.height_glide + deficit));
  ok1(equals(cruise.time_elapsed.count(), distance / (v / k) + deficit / mc));

  /* Spare height is glided; the shortfall is cruise-climbed. */
  constexpr double have       = 300;
  const GlideResult split     = Fly(polar, Leg(distance, 0, have, zero));
  const double glide_distance = v * have / sink;
  const GlideResult rest =
    Fly(polar, Leg(distance - glide_distance, 0, 0, zero));
  ok1(equals(split.vector.distance, distance));
  ok1(equals(split.height_climb, rest.height_climb));
  ok1(equals(split.height_glide, have + rest.height_glide));
  ok1(!split.IsFinalGlide() && glide_distance < distance);

  /* Circling in a headwind moves the target further away. */
  constexpr double wind  = 8;
  const GlideState blown = Leg(distance, 0, -deficit, WindFrom(0, wind));
  const FloatDuration climb_drift{deficit / mc};
  const double drifted        = blown.DriftedDistance(climb_drift);
  const double gs             = blown.CalcAverageSpeed(v / k);
  const GlideResult with_wind = Fly(polar, blown);
  ok1(drifted > distance);
  ok1(equals(with_wind.height_glide, drifted / gs / k * sink));
  ok1(equals(with_wind.height_climb, with_wind.height_glide + deficit));
  ok1(with_wind.height_glide > cruise.height_glide);

  /* MC 0 reports the track component. Climb and sink blend around it,
     including once the arrival is already a final glide. */
  const GlideResult above =
    Fly(polar, Leg(distance, 0, 6000, WindFrom(0, wind)));
  AircraftState ac;
  ac.Reset();
  ac.track        = Angle::Zero();
  ac.ground_speed = 30;
  ac.vario        = 3;
  ok1(above.IsFinalGlide());
  ok1(equals(above.InstantSpeed(ac, above, GlidePolar(0)), 30));

  const double head  = above.head_wind;
  ac.vario           = 4;
  const double omega = ac.vario / mc;
  const double rho_g = omega / (omega + sink / mc);
  ok1(equals(above.InstantSpeed(ac, above, polar),
             (1 - rho_g) * ac.ground_speed + rho_g * (v - head)));

  ac.vario                = -2;
  const double omega_sink = ac.vario / mc;
  const double rho_c      = omega_sink / (omega_sink - 1);
  ok1(equals(above.InstantSpeed(ac, above, polar),
             (1 - rho_c) * ac.ground_speed + rho_c * -head));
}

int
main()
{
  plan_tests(114 + 7 + 4 + 17 + 11);

  settings.SetDefaults();
  TestGlide();
  TestBranches();
  TestSpeedToFly();
  TestPossibleAndDrift();
  TestTwoLegs();
  TestRisk();
  TestFlightMode();
  TestPilotWatch();
  TestStrategies();
  return exit_status();
}
