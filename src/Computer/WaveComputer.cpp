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

#include "WaveComputer.hpp"
#include "WaveResult.hpp"
#include "WaveSettings.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/FlyingState.hpp"
#include "Geo/Flat/FlatPoint.hpp"
#include "Geo/Flat/FlatLine.hpp"

void
WaveComputer::Initialise()
{
  delta_time.Reset();
  ResetCurrent();
  waves.clear();
}

void
WaveComputer::ResetCurrent()
{
  last_location_available.Clear();
  last_netto_vario_available.Clear();
  sinking_clock.Clear();
  ls.Reset();
}

/**
 * Obtain the #Validity instance that applies to
 * #NMEAInfo::netto_vario_available.
 *
 * TODO: this is a kludge that duplicates code from #BasicComputer.
 * We should have new attributes in #NMEAInfo to get that piece of
 * information right away without this ugly code duplication.
 */
gcc_pure
static Validity
GetNettoVarioAvailable(const NMEAInfo &basic)
{
  if (basic.netto_vario_available)
    return basic.netto_vario_available;

  if (basic.total_energy_vario_available)
    return basic.total_energy_vario_available;

  if (basic.noncomp_vario_available)
    return basic.noncomp_vario_available;

  if (basic.pressure_altitude_available)
    return basic.pressure_altitude_available;

  if (basic.baro_altitude_available)
    return basic.baro_altitude_available;

  return basic.gps_altitude_available;
}

/**
 * Convert a #LeastSquares to a #WaveInfo.  Returns
 * WaveInfo::Undefined() if there is no valid result in the
 * #LeastSquares instance.
 */
gcc_pure
static WaveInfo
GetWaveInfo(const LeastSquares &ls, const FlatProjection &projection,
            double time)
{
  if (!ls.HasResult())
    return WaveInfo::Undefined();

  const FlatPoint flat_location(ls.GetMiddleX(), ls.GetAverageY());
  const GeoPoint location(projection.Unproject(flat_location));

  const GeoPoint a(projection.Unproject(FlatPoint(ls.GetMinX(),
                                                  ls.GetYAtMinX())));
  const GeoPoint b(projection.Unproject(FlatPoint(ls.GetMaxX(),
                                                  ls.GetYAtMaxX())));

  Angle bearing = a.Bearing(b);
  Angle normal = (bearing + Angle::QuarterCircle()).AsBearing();

  return {location, a, b, normal, time};
}

void
WaveComputer::Decay(double min_time)
{
  for (auto i = waves.begin(), end = waves.end(); i != end;) {
    if (i->time < min_time)
      i = waves.erase(i);
    else
      ++i;
  }
}

/**
 * Is this line ratio in range to allow merging two lines?  This
 * hard-codes the allowed distance to be 10% of the smaller line
 * length.
 */
static constexpr bool
IsRatioInRange(double ratio)
{
  return ratio > -0.1 && ratio < 1.1;
}

/**
 * Wrapper for FlatLine::Interpolate() which will clip the ratio to
 * [0..1] so the result stays within the [a..b] bounds.
 */
static constexpr FlatPoint
InterpolateClip(const FlatLine line, double ratio)
{
  return ratio <= 0
    ? line.a
    : (ratio >= 1
       ? line.b
       : line.Interpolate(ratio));
}

struct RatioAndDistance {
  double ratio, squared_distance;
};

gcc_pure
static RatioAndDistance
CalcRatioAndDistance(const FlatLine line, const FlatPoint point)
{
  RatioAndDistance result;
  result.ratio = line.ProjectedRatio(point);
  FlatPoint projected = InterpolateClip(line, result.ratio);
  result.squared_distance = (point - projected).MagnitudeSquared();
  return result;
}

/**
 * Check if the two wave lines match, and attempt to merge them.
 *
 * TODO: this is clumsy and unoptimised code; please improve!
 *
 * @return true if the lines have been merged into #a
 */
static bool
MergeLines(FlatLine &a, const FlatLine b)
{
  const double a_sq = a.GetSquaredDistance();
  const double b_sq = b.GetSquaredDistance();

  const double max_sq_distance =
    (std::max(a_sq, b_sq) + 2 * std::min(a_sq, b_sq)) / 250;

  const RatioAndDistance results[] = {
    CalcRatioAndDistance(a, b.a),
    CalcRatioAndDistance(a, b.b),
    CalcRatioAndDistance(b, a.a),
    CalcRatioAndDistance(b, a.b),
  };

  /* if two out of 4 line endpoints must be near the other line */
  unsigned n_distance_ok = 0;
  for (auto i : results)
    if (i.squared_distance <= max_sq_distance)
      ++n_distance_ok;

  if (n_distance_ok < 2)
    return false;

  /* the ratio of at least one point must be in range */
  unsigned n_ratio_ok = 0;
  for (auto i : results)
    if (IsRatioInRange(i.ratio))
      ++n_ratio_ok;

  if (n_ratio_ok == 0)
    return false;

  /* we can merge!  now check if we should extend the original (old)
     line because the new one exceeds it */

  if (results[0].ratio < 0)
    a.a = b.a;

  if (results[1].ratio > 1)
    a.b = b.b;

  return true;
}

/**
 * Check if the two wave lines match, and attempt to merge them.
 *
 * TODO: this is clumsy and unoptimised code; please improve!
 *
 * @return true if the lines have been merged into #i
 */
static bool
MergeLines(WaveInfo &i, const WaveInfo &new_wave, double new_length,
           FlatLine new_line, const FlatProjection &projection)
{
  Angle delta_angle = (new_wave.normal - i.normal).AsDelta();
  if (delta_angle > Angle::QuarterCircle())
    delta_angle -= Angle::HalfCircle();
  else if (delta_angle < -Angle::QuarterCircle())
    delta_angle += Angle::HalfCircle();

  /* basic direction check */
  constexpr Angle max_delta_angle = Angle::Degrees(20);
  if (delta_angle > max_delta_angle || delta_angle < -max_delta_angle)
    return false;

  /* basic range check */
  const double max_distance = std::max(new_length, i.GetLength());
  if (i.a.DistanceS(new_wave.a) > max_distance &&
      i.b.DistanceS(new_wave.a) > max_distance &&
      i.a.DistanceS(new_wave.b) > max_distance &&
      i.a.DistanceS(new_wave.b) > max_distance)
    return false;

  FlatLine other_line(projection.ProjectFloat(i.a),
                      projection.ProjectFloat(i.b));
  if (other_line.a.x > other_line.b.x)
    /* rearrange a and b to get well-defined order */
    std::swap(other_line.a, other_line.b);

  if (!MergeLines(other_line, new_line))
    return false;

  i.a = projection.Unproject(other_line.a);
  i.b = projection.Unproject(other_line.b);
  Angle bearing = i.a.Bearing(i.b);
  i.normal = (bearing + Angle::QuarterCircle()).AsBearing();
  i.time = new_wave.time;

  return true;
}

inline void
WaveComputer::FoundWave(const WaveInfo &new_wave)
{
  /* check if we can merge the new wave with an existing one to
     unclutter the screen */

  FlatProjection projection(new_wave.location);
  const double new_length = new_wave.GetLength();

  FlatLine new_line(projection.ProjectFloat(new_wave.a),
                    projection.ProjectFloat(new_wave.b));
  if (new_line.a.x > new_line.b.x)
    /* rearrange a and b to get well-defined order */
    std::swap(new_line.a, new_line.b);

  for (auto &i : waves)
    if (MergeLines(i, new_wave, new_length, new_line, projection))
      return;

  waves.push_front(new_wave);
}

void
WaveComputer::Compute(const NMEAInfo &basic,
                      const FlyingState &flight,
                      WaveResult &result,
                      const WaveSettings &settings)
{
  const bool new_enabled = settings.enabled;
  if (new_enabled != last_enabled) {
    last_enabled = new_enabled;

    if (new_enabled) {
      /* the WaveComputer has just been enabled - initialise internal
         state */
      Initialise();
    } else
      /* the WaveComputer has just been disabled - clear the result */
      result.Clear();
  }

  if (!new_enabled) {
    /* we're disabled: bail out */
    assert(result.waves.empty());
    return;
  }

  if (!flight.IsGliding()) {
    /* no wave calculations while not in gliding free-flight */
    ResetCurrent();
    return;
  }

  const auto netto_vario_available = GetNettoVarioAvailable(basic);
  if (!basic.location_available.Modified(last_location_available) ||
      !netto_vario_available.Modified(last_netto_vario_available))
    /* no new data since the last call; need both a new GPS location
       and a vario value */
    return;

  const auto dt = delta_time.Update(basic.time, 0.5, 20);
  if (dt < 0)
    /* time warp */
    Reset();

  if (dt <= 0)
    /* throttle */
    return;

  const auto vario = basic.netto_vario;

  constexpr double threshold(0.5);
  if (vario > threshold) {
    /* positive vario value - feed it to the #LeastSquares instance */

    if (ls.IsEmpty())
      /* initialise the projection each time we inspect a new wave
         "candidate" */
      projection.SetCenter(basic.location);

    auto flat = projection.ProjectFloat(basic.location);
    ls.Update(flat.x, flat.y, vario - threshold / 2);
  }

  if (vario < 0)
    sinking_clock.Add(dt);
  else
    sinking_clock.Subtract(dt);

  const bool sinking = sinking_clock >= dt + 1;
  if (sinking) {
    /* we've been sinking; stop calculating the current wave; prepare
       to flush the #LeastSquares instance */
    if (ls.GetCount() > 30) {
      /* we've been lifting in the wave for some time; see if we
         really spotted a wave */
      const WaveInfo wave = GetWaveInfo(ls, projection,
                                        basic.time_available ? basic.time : 0);
      if (wave.IsDefined())
        /* yes, spotted a wave: copy it from the #LeastSquares
           instance to the list of waves */
        FoundWave(wave);
    }

    ls.Reset();
  }

  if (basic.time_available)
    /* forget all waves which are older than 8 hours */
    Decay(basic.time - 8 * 3600);

  /* fill the #WaveResult */

  result.Clear();

  /* first copy the wave that is currently being calculated (partial
     data) */
  WaveInfo wave = GetWaveInfo(ls, projection,
                              basic.time_available ? basic.time : 0);
  if (wave.IsDefined())
    result.waves.push_back(wave);

  /* now copy the rest */
  for (auto i = waves.begin(), end = waves.end();
       i != end && !result.waves.full(); ++i)
    result.waves.push_back(*i);

  /* remember some data for the next iteration */
  last_location_available = basic.location_available;
  last_netto_vario_available = netto_vario_available;
}
