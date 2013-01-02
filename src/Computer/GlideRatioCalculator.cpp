/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "GlideRatioCalculator.hpp"
#include "Math/LowPassFilter.hpp"
#include "ComputerSettings.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

/** over this, show INVALID_GR */
static const fixed MAXEFFICIENCYSHOW = fixed(200);

void
GlideRatioCalculator::Initialize(const ComputerSettings &settings)
{
  unsigned bsize;

  switch (settings.average_eff_time) {
  case ae15seconds:
    bsize = 15; // useless, LDinst already there
    break;
  case ae30seconds:
    bsize = 30; // limited useful
    break;
  case ae60seconds:
    bsize = 60; // starting to be valuable
    break;
  case ae90seconds:
    bsize = 90; // good interval
    break;
  case ae2minutes:
    bsize = 120; // other software's interval
    break;
  case ae3minutes:
    bsize = 180; // probably too long interval
    break;
#ifndef __clang__
  default:
    bsize = 3; // make it evident
    break;
#endif
  }

  assert(bsize >= 3);
  assert(bsize <= ARRAY_SIZE(records));

  totaldistance = 0;
  start = -1;
  size = bsize;
  valid = false;
}

void
GlideRatioCalculator::Add(unsigned distance, int altitude)
{
  static short errs = 0;

  if (distance < 3 || distance > 150) { // just ignore, no need to reset rotary
    if (errs > 2) {
      errs = 0;
      return;
    }
    errs++;
    return;
  }
  errs = 0;

  if (++start >= size) {
    start = 0;
    valid = true; // flag for a full usable buffer
  }

  // need to fill up buffer before starting to empty it
  if (valid)
    totaldistance -= records[start].distance;
  totaldistance += distance;
  records[start].distance = distance;
  records[start].altitude = altitude;
}

/*
 * returns 0 if invalid, 999 if too high
 */
fixed
GlideRatioCalculator::Calculate() const
{
  int altdiff;
  short bcold;

  if (start >= size)
    return fixed_zero;

  if (!valid) {
    if (start == 0)
      return fixed_zero; // unavailable

    bcold = 0;
  } else {
    if (start < size - 1)
      bcold = start + 1;
    else
      bcold = 0;
  }

  altdiff = records[bcold].altitude - records[start].altitude;
  if (altdiff == 0)
    return INVALID_GR; // infinitum

  fixed eff = (fixed) totaldistance / (fixed) altdiff;
  if (eff > MAXEFFICIENCYSHOW)
    eff = INVALID_GR;

  return eff;
}

// existing methods (moving average via low pass filter)

// limit to reasonable values
gcc_const
static fixed
LimitGR(fixed gr)
{
  if (fabs(gr) > INVALID_GR)
    return INVALID_GR;

  if (gr >= fixed_zero && gr < fixed_one)
    return fixed_one;

  if (gr < fixed_zero && gr > fixed_minus_one)
    return fixed_minus_one;

  return gr;
}

fixed
UpdateGR(fixed gr, fixed leg_distance, fixed height_above_leg,
         fixed filter_factor)
{
  if (!positive(leg_distance))
    return gr;

  fixed glideangle = height_above_leg / leg_distance;
  if (gr != INVALID_GR)
    glideangle = LowPassFilter(fixed_one / gr, glideangle, filter_factor);

  if (fabs(glideangle) > fixed_one / INVALID_GR)
    gr = LimitGR(fixed_one / glideangle);
  else
    gr = INVALID_GR;

  return gr;
}
