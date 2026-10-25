// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Store.hpp"

template<typename Rows>
static void
EraseOrigin(Rows &rows, ObservationOrigin origin) noexcept
{
  for (unsigned i = 0; i < rows.size();) {
    if (rows[i].origin == origin)
      rows.remove(i);
    else
      ++i;
  }
}

template<typename Rows>
static unsigned
CountOrigin(const Rows &rows, ObservationOrigin origin) noexcept
{
  unsigned count = 0;
  for (const auto &row : rows)
    if (row.origin == origin)
      ++count;
  return count;
}

/** First row of this origin is the oldest append. */
template<typename Rows>
static unsigned
OldestOrigin(const Rows &rows, ObservationOrigin origin) noexcept
{
  for (unsigned i = 0; i < rows.size(); ++i)
    if (rows[i].origin == origin)
      return i;
  return 0;
}

void
ObservationStore::ReplaceThermals(
    ObservationOrigin origin,
    std::span<const ObservedThermal> rows) noexcept
{
  EraseOrigin(thermals, origin);

  const unsigned begin = rows.size() > OBSERVATION_CAP
    ? unsigned(rows.size() - OBSERVATION_CAP)
    : 0;
  for (unsigned i = begin; i < rows.size(); ++i) {
    if (thermals.full())
      break;

    ObservedThermal row = rows[i];
    row.origin = origin;
    thermals.append(row);
  }
}

void
ObservationStore::AppendThermal(const ObservedThermal &row) noexcept
{
  if (CountOrigin(thermals, row.origin) >= OBSERVATION_CAP)
    thermals.remove(OldestOrigin(thermals, row.origin));

  if (!thermals.full())
    thermals.append(row);
}

void
ObservationStore::ReplaceWaves(ObservationOrigin origin,
                               std::span<const ObservedWave> rows) noexcept
{
  EraseOrigin(waves, origin);

  const unsigned begin = rows.size() > OBSERVATION_CAP
    ? unsigned(rows.size() - OBSERVATION_CAP)
    : 0;
  for (unsigned i = begin; i < rows.size(); ++i) {
    if (waves.full())
      break;

    ObservedWave row = rows[i];
    row.origin = origin;
    waves.append(row);
  }
}

void
ObservationStore::AppendWave(const ObservedWave &row) noexcept
{
  if (CountOrigin(waves, row.origin) >= OBSERVATION_CAP)
    waves.remove(OldestOrigin(waves, row.origin));

  if (!waves.full())
    waves.append(row);
}

void
SelectThermals(std::span<const ObservedThermal> in,
               ThermalRows &out, double range_m) noexcept
{
  out.clear();

  for (const auto &candidate : in) {
    if (!candidate.location.IsValid())
      continue;

    bool dominated = false;
    for (const auto &kept : out) {
      if (!kept.location.IsValid() ||
          kept.location.DistanceS(candidate.location) > range_m)
        continue;

      if (candidate.received <= kept.received) {
        dominated = true;
        break;
      }
    }

    if (dominated)
      continue;

    for (unsigned k = 0; k < out.size();) {
      if (out[k].location.IsValid() &&
          out[k].location.DistanceS(candidate.location) <= range_m)
        out.remove(k);
      else
        ++k;
    }

    if (!out.full())
      out.append(candidate);
  }
}
