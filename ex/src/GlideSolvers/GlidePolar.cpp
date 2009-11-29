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

GlidePolar::GlidePolar(const double _mc,
                       const double _bugs,
                       const double _ballast):
  mc(_mc),
  bugs(_bugs),
  ballast(_ballast),
  cruise_efficiency(1.0)
{
  solve();
}

void
GlidePolar::set_mc(const double _mc)
{
  mc = _mc;
  solve();
}

double
GlidePolar::MSinkRate(const double V) const
{
  return SinkRate(V)+mc;
}

double 
GlidePolar::SinkRate(const double V) const
{
  /// \todo note this is hardcoded at present, will need proper polar management later
  const double dV = (V-25.0)*0.056;
  return 0.5+(dV*dV+V*0.01)/2.0;
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
 * 
 * @return Initialised object (no search yet)
 */
  GlidePolarVopt(const GlidePolar &_polar):
    ZeroFinder(15.0, 75.0, TOLERANCE_POLAR_BESTLD),
    polar(_polar)
    {
    };
/** 
 * Glide ratio function (negative to minimise)
 * 
 * @param V Speed (m/s)
 * 
 * @return MacCready-adjusted glide ratio
 */
  double f(const double V) {
    return -V/polar.MSinkRate(V);
  }
private:
  const GlidePolar &polar;
};


void 
GlidePolar::solve()
{
  GlidePolarVopt gpvopt(*this);
  VbestLD = gpvopt.find_min(20.0);
  SbestLD = SinkRate(VbestLD);
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
                       const double S) const
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
  if (task.AltitudeDifference<=0) {
    return false;
  }
  // broad test assuming tailwind (best case)
  if ((VbestLD+task.EffectiveWindSpeed)*task.AltitudeDifference 
      < task.Vector.Distance*SbestLD) {
    return false;
  } else {
    return true;
  }
}

