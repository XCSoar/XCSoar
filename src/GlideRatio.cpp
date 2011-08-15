/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "GlideRatio.hpp"
#include "Math/LowPassFilter.hpp"
#include "SettingsComputer.hpp"
#include "Util/Macros.hpp"

#include <assert.h>

/** over this, show INVALID_GR */
static const int MAXEFFICIENCYSHOW = 200;

void
GlideRatioCalculator::init(const SETTINGS_COMPUTER &settings)
{
  unsigned bsize;

  switch (settings.AverEffTime) {
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
  default:
    bsize = 3; // make it evident
    break;
  }

  assert(bsize >= 3);
  assert(bsize < ARRAY_SIZE(records));

  totaldistance = 0;
  start = -1;
  size = bsize;
  valid = false;
}

void
GlideRatioCalculator::add(unsigned distance, int altitude)
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
int
GlideRatioCalculator::calculate() const
{
  int altdiff, eff;
  short bcold;

  if (start >= size)
    return 0;

  if (!valid) {
    if (start == 0)
      return 0; // unavailable

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

  eff = (int)totaldistance / altdiff;
  if (eff > MAXEFFICIENCYSHOW)
    eff = INVALID_GR;

  return eff;
}

// existing methods (moving average via low pass filter)

// limit to reasonable values
gcc_const
static fixed
LimitLD(fixed LD)
{
  if (fabs(LD) > fixed(INVALID_GR))
    return fixed(INVALID_GR);

  if (LD >= fixed_zero && LD < fixed_one)
    return fixed_one;

  if (LD < fixed_zero && LD > fixed_minus_one)
    return fixed_minus_one;

  return LD;
}

fixed
UpdateLD(fixed LD, fixed leg_distance, fixed height_above_leg,
         fixed filter_factor)
{
  if (!positive(leg_distance))
    return LD;

  fixed glideangle;
  if (LD != fixed_zero)
    glideangle = fixed_one / LD;
  else
    glideangle = fixed_one;

  glideangle = LowPassFilter(fixed_one / LD, height_above_leg / leg_distance,
                             filter_factor);

  if (fabs(glideangle) > fixed_one / INVALID_GR)
    LD = LimitLD(fixed_one / glideangle);
  else
    LD = fixed(INVALID_GR);

  return LD;
}
