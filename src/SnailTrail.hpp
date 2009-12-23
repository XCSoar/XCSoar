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

#ifndef XCSOAR_SNAIL_TRAIL_HPP
#define XCSOAR_SNAIL_TRAIL_HPP

#include "Math/fixed.hpp"
#include "Screen/shapelib/mapshape.h"
#include "GPSClock.hpp"
#include "Poco/RWLock.h"

#include <windef.h>

struct NMEA_INFO;
struct DERIVED_INFO;

typedef struct _SNAIL_POINT
{
  fixed Latitude;
  fixed Longitude;
  fixed Vario;
  fixed Time;
  short Colour;
  BOOL Circling;
  bool FarVisible;
  fixed DriftFactor;
} SNAIL_POINT;

class SnailTrail {
public:
  enum {
    /** 1000 points at 3.6 seconds average = one hour */
    TRAILSIZE = 1000,
    /** short trail is 10 minutes approx */
    TRAILSHRINK = 5,
  };

public:
  SnailTrail();
  void AddPoint(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated);

  int getIndex() const {
    return indexNext;
  }
  const SNAIL_POINT& getPoint(int index) const {
    return TrailPoints[index];
  }

  void ScanVisibility(rectObj *bounds);
  GPSClock clock;

  void ReadLock() {
    lock.readLock();
  }
  void Unlock() {
    lock.unlock();
  }
private:
  SNAIL_POINT TrailPoints[TRAILSIZE];
  int indexNext;
  Poco::RWLock lock;
};

#endif
