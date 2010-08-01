/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Defines.h"

#include <assert.h>

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
  assert(bsize < sizeof(records) / sizeof(records[0]));

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
static double
LimitLD(double LD)
{
  if (fabs(LD) > INVALID_GR)
    return INVALID_GR;

  if ((LD >= 0.0) && (LD < 1.0))
    return 1.0;

  if ((LD < 0.0) && (LD > -1.0))
    return -1.0;

  return LD;
}

double
UpdateLD(double LD, double leg_distance, double height_above_leg,
         double filter_factor)
{
  if (leg_distance <= 0)
    return LD;

  double glideangle;
  if (LD != 0)
    glideangle = 1.0 / LD;
  else
    glideangle = 1.0;

  glideangle = LowPassFilter(1.0 / LD, height_above_leg / leg_distance,
                             filter_factor);

  if (fabs(glideangle) > 1.0 / INVALID_GR)
    LD = LimitLD(1.0 / glideangle);
  else
    LD = INVALID_GR;

  return LD;
}
