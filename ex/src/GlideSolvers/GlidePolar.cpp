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
#include "GlidePolar.hpp"
#include "GlideState.hpp"
#include "GlideResult.hpp"
#include "MacCready.hpp"
#include "Util/ZeroFinder.hpp"
#include "Util/Tolerances.hpp"
#include "Navigation/Aircraft.hpp"

GlidePolar::GlidePolar(const fixed _mc,
                       const fixed _bugs,
                       const fixed _ballast):
  bugs(_bugs),
  ballast(_ballast),
  cruise_efficiency(fixed_one)
{
  static const fixed fixed_75 = 75.0;

  Vmax = fixed_75;
  Smax = SinkRate(Vmax);

  solve_min();

  set_mc(_mc);
}


void
GlidePolar::set_mc(const fixed _mc)
{
  mc = _mc;
  if (positive(mc)) {
    inv_mc = fixed_one/mc;
  } else {
    inv_mc = fixed_zero;
  }
  solve_ld();
}

fixed
GlidePolar::MSinkRate(const fixed V) const
{
  return SinkRate(V)+mc;
}

fixed 
GlidePolar::SinkRate(const fixed V) const
{
  /// \todo note this is hardcoded at present, will need proper polar management later
  static const fixed v0 = 1.48;
  static const fixed v1 = -0.0734;
  static const fixed v2 = 0.00157;

  return v0+v1*V+v2*V*V;
}

/**
 * Finds VOpt for a given MacCready setting
 * Intended to be used temporarily.
 */
class GlidePolarVopt: 
  public ZeroFinder
{
public:
/** 
 * Constructor.
 * 
 * @param _polar Glide polar to optimise
 * @param vmin Minimum speed to search (m/s)
 * @param vmax Maximum speed to search (m/s)
 * 
 * @return Initialised object (no search yet)
 */
  GlidePolarVopt(const GlidePolar &_polar, const fixed& vmin, const fixed &vmax):
    ZeroFinder(vmin, vmax, TOLERANCE_POLAR_BESTLD),
    polar(_polar)
    {
    };
/** 
 * Glide ratio function
 * 
 * @param V Speed (m/s)
 * 
 * @return MacCready-adjusted inverse glide ratio
 */
  fixed f(const fixed V) {
    return polar.MSinkRate(V)/V;
  }
private:
  const GlidePolar &polar;
};


void 
GlidePolar::solve_ld()
{
  GlidePolarVopt gpvopt(*this, Vmin, Vmax);
  VbestLD = gpvopt.find_min(Vmax);
  SbestLD = SinkRate(VbestLD);
}


/**
 * Finds min sink speed.
 * Intended to be used temporarily.
 */
class GlidePolarMinSink: 
  public ZeroFinder
{
public:
/** 
 * Constructor.
 * 
 * @param _polar Glide polar to optimise
 * @param vmax Maximum speed to search (m/s)
 * 
 * @return Initialised object (no search yet)
 */
  GlidePolarMinSink(const GlidePolar &_polar, const fixed &vmax):
    ZeroFinder(fixed_one, vmax, TOLERANCE_POLAR_MINSINK),
    polar(_polar)
    {
    };
  fixed f(const fixed V) {
    return polar.SinkRate(V);
  }
private:
  const GlidePolar &polar;
};


void 
GlidePolar::solve_min()
{
  GlidePolarMinSink gpminsink(*this, Vmax);
  Vmin = gpminsink.find_min(Vmax);
  Smin = SinkRate(Vmin);
}


#ifdef INSTRUMENT_TASK
long count_mc = 0;
#endif

GlideResult 
GlidePolar::solve(const GlideState &task) const
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(*this, cruise_efficiency);
  return mac.solve(task);
}

GlideResult 
GlidePolar::solve_sink(const GlideState &task,
                       const fixed S) const
{
#ifdef INSTRUMENT_TASK
  count_mc++;
#endif
  MacCready mac(*this, cruise_efficiency);
  return mac.solve_sink(task,S);
}


bool 
GlidePolar::possible_glide(const GlideState &task) const
{
  if (!positive(task.AltitudeDifference)) {
    return false;
  }
  // broad test assuming tailwind at best LD (best case)
  if ((VbestLD+task.EffectiveWindSpeed)*task.AltitudeDifference 
      < task.Vector.Distance*SbestLD) {
    return false;
  } else {
    return true;
  }
}


/**
 * Finds speed to fly for a given MacCready setting
 * Intended to be used temporarily.
 *
 * This finds the speed that maximises the glide angle over the ground
 */
class GlidePolarSpeedToFly: 
  public ZeroFinder
{
public:
/** 
 * Constructor.
 * 
 * @param _polar Glide polar to optimise
 * @param net_sink_rate Instantaneous netto sink rate (m/s), positive down
 * @param head_wind Head wind component (m/s)
 * @param vmin Minimum speed to search (m/s)
 * @param vmax Maximum speed to search (m/s)
 * 
 * @return Initialised object (no search yet)
 */
  GlidePolarSpeedToFly(const GlidePolar &_polar, 
                       const fixed& net_sink_rate,
                       const fixed& head_wind,
                       const fixed& vmin, const fixed &vmax):
    ZeroFinder(max(fixed_one,vmin-head_wind), vmax-head_wind, TOLERANCE_POLAR_DOLPHIN),
    polar(_polar),
    m_net_sink_rate(net_sink_rate),
    m_head_wind(head_wind)
    {
    };

/** 
 * Glide ratio function
 * 
 * @param V Speed over ground (m/s)
 * 
 * @return MacCready-adjusted inverse glide ratio
 */
  fixed f(const fixed V) {
    return (polar.MSinkRate(V+m_head_wind)+m_net_sink_rate)/V;
  }

/** 
 * Find best speed to fly
 * 
 * @param Vstart Initial search speed (m/s)
 * 
 * @return Speed to fly (m/s)
 */  
  fixed solve(const fixed Vstart) {
    fixed Vopt = find_min(Vstart);
    return Vopt+m_head_wind;
  }
private:
  const GlidePolar &polar;
  const fixed& m_net_sink_rate;
  const fixed& m_head_wind;
};


fixed 
GlidePolar::speed_to_fly(const AIRCRAFT_STATE &state,
                         const GlideResult &solution) const
{
  const fixed net_sink_rate = -state.NettoVario;

  if (-net_sink_rate > mc+Smin) {
    // stop to climb
    return Vmin;
  }

  const fixed head_wind = solution.is_final_glide()? solution.HeadWind : fixed_zero;

  GlidePolarSpeedToFly gp_stf(*this, net_sink_rate, head_wind, Vmin, Vmax);
  return gp_stf.solve(Vmax);
}


fixed 
GlidePolar::get_all_up_weight() const {
  return fixed_one; // TODO FIX
}

fixed 
GlidePolar::get_wing_loading() const {
  return fixed_one; // TODO FIX
}

fixed 
GlidePolar::get_bestLD() const {
  return VbestLD/SbestLD;
}
