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
#ifndef GLIDEPOLAR_HPP
#define GLIDEPOLAR_HPP

struct GlideState;
struct GlideResult;

#include "Math/fixed.hpp"

/**
 * Class implementing basic glide polar performance model
 * 
 * Implements aircraft-specific glide performance, including
 * bugs/ballast, MacCready setting and cruise efficiency.
 *
 * Cruise efficiency is the ratio of actual cruise speed to 
 * target to the classical MacCready speed.
 * Cruise efficiency is stored in this class for convenience,
 * it is used in MacCready class.
 * 
 * The MacCready class uses this GlidePolar data to calculate
 * specific GlideSolutions. 
 *
 * \todo
 * - currently the polar itself and Vmax is hard-coded
 * - currently bugs/ballast are ignored
 * - implement wing loading
 * - implement AUW
 */

class GlidePolar
{
public:
/** 
 * Constructor.  Performs search for best LD at instantiation.
 * 
 * @param _mc MacCready value at construction
 * @param _bugs Bugs value at construction (currently unimplemented)
 * @param _ballast Ballast value at construction (currently unimplemented)
 */
  GlidePolar(const fixed _mc,
             const fixed _bugs,
             const fixed _ballast);

/** 
 * Accesses sink rate at min airspeed
 * 
 * @return Sink rate (m/s, positive down)
 */
  fixed get_Smin() const {
    return Smin;
  }

/** 
 * Accesses airspeed for minimum sink
 * 
 * @return Speed (m/s)
 */
  fixed get_Vmin() const {
    return Vmin;
  }

/** 
 * Accesses maximum airspeed
 * 
 * @return Speed (m/s)
 */
  fixed get_Vmax() const {
    return Vmax;
  }

/** 
 * Accesses sink rate at max airspeed
 * 
 * @return Sink rate (m/s, positive down)
 */
  fixed get_Smax() const {
    return Smax;
  }

/** 
 * Accesses best L/D speed
 * 
 * @return Speed of best LD (m/s)
 */
  fixed get_VbestLD() const {
    return VbestLD;
  }

/** 
 * Accesses best L/D sink rate (positive down)
 * 
 * @return Sink rate at best L/D (m/s)
 */
  fixed get_SbestLD() const {
    return SbestLD;
  }

/** 
 * Accesses best L/D ratio
 * 
 * @return Best L/D ratio
 */
  fixed get_bestLD() const {
    return VbestLD/SbestLD;
  }

/** 
 * Set cruise efficiency value.  1.0 = perfect MacCready speed
 * 
 * @param _ce The new cruise efficiency value
 */
  void set_cruise_efficiency(const fixed _ce) {
    cruise_efficiency = _ce;
  }

/** 
 * Set MacCready value.  Internally this performs search
 * for best LD values corresponding to this setting.
 * 
 * @param _mc The new MacCready ring setting (m/s)
 */
  void set_mc(const fixed _mc);

/** 
 * Accessor for MC setting
 * 
 * @return The current MacCready ring setting (m/s)
 */
  fixed get_mc() const {
    return mc;
  }

/** 
 * Accessor for inverse of MC setting
 * 
 * @return The inverse of current MacCready ring setting (s/m)
 */
  fixed get_inv_mc() const {
    return inv_mc;
  }

/** 
 * Calculate all up weight 
 *
 * @return Mass (kg) of aircraft including ballast
 */
  fixed get_all_up_weight() const {
    return fixed_one; // TODO FIX
  }

/** 
 * Calculate wing loading 
 *
 * @return Wing loading (all up mass divided by reference area, kg/m^2)
 */
  fixed get_wing_loading() const {
    return fixed_one; // TODO FIX
  }

/** 
 * Sink rate model (actual glide polar) function.
 * 
 * @param V Speed at which sink rate is to be evaluated
 * 
 * @return Sink rate (m/s, positive down)
 */
  fixed SinkRate(const fixed V) const;

/** 
 * Sink rate model adjusted by MC setting.  This is used
 * to accomodate speed ring (MC) settings in optimal glide
 * calculations. 
 * 
 * @param V Speed at which sink rate is to be evaluated
 * 
 * @return Sink rate plus MC setting (m/s, positive down)
 */
  fixed MSinkRate(const fixed V) const;

/** 
 * Use classical MC theory to compute the optimal glide solution
 * for a given task.
 * 
 * @param task The glide task for which to compute a solution
 * 
 * @return Glide solution
 */
  GlideResult solve(const GlideState &task) const;

/** 
 * Neglecting the actual glide sink rate, calculate the
 * glide solution with an externally supplied sink rate,
 * assuming the glider flies at speeds according to classical
 * MacCready theory.  This is used to calculate the sink rate
 * required for glide-only solutions.
 * 
 * @param task The glide task for which to compute a solution
 * @param S Imposed sink rate
 * 
 * @return Glide solution for the virtual task
 */
  GlideResult solve_sink(const GlideState &task,
                          const fixed S) const;

/** 
 * Quickly determine whether a task is achievable without 
 * climb, assuming favorable wind.  This can be used to quickly
 * pre-filter waypoints for arrival altitude before performing
 * expensive optimal glide solution searches.
 * 
 * @param task The glide task for which to estimate a solution
 * 
 * @return True if a glide solution is feasible (optimistically)
 */
  bool possible_glide(const GlideState &task) const;

private:
/** 
 * Solve for best LD at current MC/bugs/ballast setting.
 */
  void solve_ld();

/** 
 * Solve for min sink rate at current bugs/ballast setting.
 */
  void solve_min();

  fixed mc;                  
  fixed inv_mc;                  
  fixed bugs;
  fixed ballast;
  fixed cruise_efficiency;
  fixed VbestLD;
  fixed SbestLD;
  fixed Smax;
  fixed Vmax;
  fixed Smin;
  fixed Vmin;

  /** @link dependency */
  /*#  MacCready lnkMacCready; */
};

#endif

