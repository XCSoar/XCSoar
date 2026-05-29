// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermAutoSwitch.hpp"

#include <algorithm>
#include <cmath>

void
XCThermAutoSwitch::SetLoadedLayers(std::vector<LayerInfo> layers) noexcept
{
  /* Sort AMSL layers by altitude ascending */
  std::sort(layers.begin(), layers.end(),
            [](const LayerInfo &a, const LayerInfo &b) {
              /* AGL layers go after AMSL layers */
              if (a.is_agl != b.is_agl)
                return !a.is_agl;
              return a.altitude_m < b.altitude_m;
            });

  loaded_layers = std::move(layers);
  RecalculateThresholds();

  /* Reset hysteresis */
  threshold_pending = false;
  altitude_manual_override = false;
  manual_step_band = -1;
  last_known_band = -1;
}

void
XCThermAutoSwitch::SetLoadedTimes(std::vector<unsigned> times) noexcept
{
  std::sort(times.begin(), times.end());
  loaded_times_utc = std::move(times);
  time_manual_override = false;
  last_switch_hour = -1;
}

void
XCThermAutoSwitch::SetCurrentLayerPos(int pos) noexcept
{
  current_layer_pos = pos;
  threshold_pending = false;
}

void
XCThermAutoSwitch::SetCurrentTimePos(int pos) noexcept
{
  current_time_pos = pos;
}

void
XCThermAutoSwitch::OnManualLayerStep() noexcept
{
  altitude_manual_override = true;
  /* Snapshot the altitude band the aircraft is currently in. Auto
     resumes only once the aircraft has physically flown into a
     different band — so picking 4000 m at ground level no longer
     gets undone the moment the override grace window expires.
     If we don't yet have an altitude reading, the snapshot is
     deferred to the first UpdateAltitude() after this. */
  manual_step_band = last_known_band;
  threshold_pending = false;
}

void
XCThermAutoSwitch::OnManualTimeStep() noexcept
{
  time_manual_override = true;
}

void
XCThermAutoSwitch::RecalculateThresholds() noexcept
{
  thresholds.clear();

  /* Only compute thresholds for non-AGL layers */
  std::vector<unsigned> amsl_alts;
  for (const auto &l : loaded_layers) {
    if (!l.is_agl)
      amsl_alts.push_back(l.altitude_m);
  }

  if (amsl_alts.size() < 2)
    return;

  /* Midpoints between consecutive AMSL layers */
  for (size_t i = 0; i + 1 < amsl_alts.size(); ++i) {
    thresholds.push_back((amsl_alts[i] + amsl_alts[i + 1]) / 2.0);
  }
}

int
XCThermAutoSwitch::FindLayerForAltitude(double altitude) const noexcept
{
  if (loaded_layers.empty())
    return -1;

  /* Count AMSL layers. AGL layers are explicitly out of scope for
     auto-switch — they're picked manually only. If the user has
     downloaded only AGL forecasts (no AMSL), we return -1 so auto
     does nothing and the manual choice stays put. */
  int n_amsl = 0;
  for (const auto &l : loaded_layers) {
    if (!l.is_agl)
      ++n_amsl;
  }

  if (n_amsl == 0)
    return -1;

  /* Find which AMSL band the altitude falls into using thresholds.
     loaded_layers is sorted AMSL-first, so positions 0..n_amsl-1 are
     guaranteed to be AMSL — never AGL. */
  int amsl_pos = 0;
  for (size_t i = 0; i < thresholds.size(); ++i) {
    if (altitude >= thresholds[i])
      amsl_pos = (int)(i + 1);
  }

  /* Clamp */
  if (amsl_pos >= n_amsl)
    amsl_pos = n_amsl - 1;

  return amsl_pos;
}

void
XCThermAutoSwitch::UpdateAltitude(double altitude, TimeStamp now) noexcept
{
  if (loaded_layers.empty() || thresholds.empty())
    return;

  int target_pos = FindLayerForAltitude(altitude);
  if (target_pos < 0)
    return;

  /* Track the band the aircraft is currently flying in. OnManualLayerStep
     reads this to snapshot the band at the moment of the manual press. */
  last_known_band = target_pos;

  /* Manual override: the pilot picked a layer. Auto resumes only once
     the aircraft physically flies into a band other than the one it was
     in when the pilot pressed. This means you can sit at 500 m and
     park the forecast on 4000 m for as long as you want — auto won't
     fight you. It re-engages naturally when you climb (or descend)
     into a new altitude band. */
  if (altitude_manual_override) {
    if (manual_step_band < 0) {
      /* No altitude was available when the manual step happened. Bind
         the snapshot to the first altitude we see now. */
      manual_step_band = target_pos;
      return;
    }
    if (target_pos == manual_step_band) {
      /* Aircraft is still in the same physical band as when the pilot
         chose a layer → keep the manual choice. */
      return;
    }
    /* Aircraft has flown into a new band → release the override and
       fall through to normal auto-switch (which will apply the 20 s
       hysteresis before actually moving the cursor). */
    altitude_manual_override = false;
  }

  if (target_pos == current_layer_pos) {
    /* No change needed, cancel any pending switch */
    threshold_pending = false;
    return;
  }

  /* Hysteresis: start or check pending timer */
  if (!threshold_pending || pending_target_pos != target_pos) {
    /* New threshold crossing detected — start timer */
    threshold_pending = true;
    pending_target_pos = target_pos;
    threshold_crossed_at = now;
    return;
  }

  /* Check if 20 seconds have elapsed */
  if (now.IsDefined() && threshold_crossed_at.IsDefined()) {
    const auto elapsed = now - threshold_crossed_at;
    if (elapsed.count() < HYSTERESIS_SECONDS)
      return; /* Not yet — keep waiting */
  }

  /* Timer expired — switch! */
  threshold_pending = false;
  current_layer_pos = target_pos;

  if (current_layer_pos >= 0 &&
      current_layer_pos < (int)loaded_layers.size() &&
      on_layer_switch)
    on_layer_switch(loaded_layers[current_layer_pos].index);
}

void
XCThermAutoSwitch::UpdateTime(unsigned utc_hour, unsigned utc_minute) noexcept
{
  if (loaded_times_utc.empty())
    return;

  /* Switch at :45 — use the NEXT hour's forecast */
  unsigned target_hour = utc_hour;
  if (utc_minute >= 45)
    target_hour = (utc_hour + 1) % 24;

  /* If manual override, check if a new hour boundary was crossed */
  if (time_manual_override) {
    if ((int)target_hour != last_switch_hour) {
      /* New hour boundary — resume auto */
      time_manual_override = false;
    } else {
      return;
    }
  }

  /* Already switched for this hour? */
  if ((int)target_hour == last_switch_hour)
    return;

  /* Find this hour in loaded times */
  auto it = std::find(loaded_times_utc.begin(), loaded_times_utc.end(),
                      target_hour);
  if (it == loaded_times_utc.end()) {
    /* Target hour not loaded — find the nearest loaded hour */
    int best = -1;
    int best_dist = 999;
    for (int i = 0; i < (int)loaded_times_utc.size(); ++i) {
      int dist = std::abs((int)loaded_times_utc[i] - (int)target_hour);
      if (dist > 12) dist = 24 - dist; /* wrap around */
      if (dist < best_dist) {
        best_dist = dist;
        best = i;
      }
    }
    if (best < 0) return;

    current_time_pos = best;
  } else {
    current_time_pos = (int)std::distance(loaded_times_utc.begin(), it);
  }

  last_switch_hour = (int)target_hour;

  if (on_time_switch)
    on_time_switch(current_time_pos);
}

void
XCThermAutoSwitch::Update(double gps_alt, double baro_alt,
                          unsigned utc_hour, unsigned utc_minute,
                          TimeStamp now) noexcept
{
  if (!enabled)
    return;

  /* Pick best available altitude */
  double alt = -1;
  if (gps_alt >= 0)
    alt = gps_alt;
  else if (baro_alt >= 0)
    alt = baro_alt;

  if (alt >= 0)
    UpdateAltitude(alt, now);

  UpdateTime(utc_hour, utc_minute);
}
