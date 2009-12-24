/* Copyright_License {

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
#ifndef DISTANCE_STAT_HPP
#define DISTANCE_STAT_HPP

#ifdef DO_PRINT
#include <iostream>
#endif

#include "Util/Filter.hpp"
#include "Util/AvFilter.hpp"
#include "Util/DiffFilter.hpp"

class ElementStat;

/**
 * Simple distance statistics with derived values (speed, incremental speed)
 * Incremental speeds track the short-term variation of distance with time,
 * whereas the overall speed is defined by the distance divided by a time value.
 */
class DistanceStat
{
public:
/** 
 * Constructor; initialises all to zero
 * 
 */
  DistanceStat(const bool is_positive=true);

    virtual ~DistanceStat() {};

/** 
 * Setter for distance value
 * 
 * @param d Distance value (m)
 */
  void set_distance(const double d) {
    distance = d;
  }

/** 
 * Accessor for distance value
 * 
 * @return Distance value (m)
 */
  double get_distance() const {
    return distance;
  };

/** 
 * Accessor for speed
 * 
 * @return Speed (m/s)
 */  double get_speed() const {
    return speed;
  };

/** 
 * Accessor for incremental speed (rate of change of
 * distance over dt, low-pass filtered)
 * 
 * @return Speed incremental (m/s)
 */
  double get_speed_incremental() const {
    return speed_incremental;
  };

/** 
 * Calculate bulk speed (distance/time), abstract base method
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es) = 0;

/** 
 * Calculate incremental speed from previous step.
 * Resets incremental speed to speed if dt=0
 * 
 * @param dt Time step (s)
 */
  void calc_incremental_speed(const double dt);

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const DistanceStat& ds);
#endif

protected:
  double distance; /**< Distance (m) of metric */
  double speed; /**< Speed (m/s) of metric */
  double speed_incremental; /**< Incremental speed (m/s) of metric */
private:
  AvFilter av_dist;
  DiffFilter df;
  Filter v_lpf;
  bool is_positive; // ideally const but then non-copyable

  void reset_incremental_speed();
};

/**
 * Specialisation of DistanceStat for remaining distances
 */
class DistanceRemainingStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance remaining/time remaining)
 * 
 * @param es ElementStat (used for time access)
 */
  void calc_speed(const ElementStat* es);
};

/**
 * Specialisation of DistanceStat for planned distances
 */
class DistancePlannedStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance planned/time planned)
 * 
 * @param es ElementStat (used for time access)
 */
  void calc_speed(const ElementStat* es);
};

/**
 * Specialisation of DistanceStat for travelled distances
 */
class DistanceTravelledStat:
  public DistanceStat
{
public:
  DistanceTravelledStat();

/** 
 * Calculate speed (distance travelled/time elapsed)
 * 
 * @param es ElementStat (used for time access)
 */
  void calc_speed(const ElementStat* es);
};



#endif
