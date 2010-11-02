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

#ifndef ABSTRACT_CONTEST_HPP
#define ABSTRACT_CONTEST_HPP

#include "Navigation/TracePoint.hpp"
#include "ScanTaskPoint.hpp"
#include "Math/fixed.hpp"

struct ContestResult;

/**
 * Abstract class for contest searches
 *
 */
class AbstractContest
{
public:
  /**
   * Constructor
   *
   * @param _trace TracePointVector object reference to use for solving
   * @param n_legs Maximum number of legs in Contest task
   * @param finish_alt_diff Maximum height loss from start to finish (m)
   */
  AbstractContest(const TracePointVector &_trace,
                  const unsigned finish_alt_diff = 1000);

  /**
   * Calculate the scored values of the Contest path
   *
   * @param result The ContestResult reference
   * in which the solution will be written
   *
   * @return True is solution was found, False otherwise
   */
  virtual bool score(ContestResult &result);

  /**
   * Copy the best Contest path solution
   *
   * @param vec output vector
   */
  virtual void copy_solution(TracePointVector &vec) const = 0;

  /**
   * Calculate distance of best path
   *
   * @return Distance (m)
   */
  virtual fixed calc_distance() const = 0;

  /**
   * Calculate score of best path
   * This is specialised by each contest type, defaults to
   * speed in kph measured by weighted distance divided by time
   * 
   * @return Score (pts)
   */
  virtual fixed calc_score() const = 0;

  /**
   * Calculate elapsed time of best path
   *
   * @return Distance (m)
   */
  virtual fixed calc_time() const = 0;

  /**
   * Reset the optimiser as if never flown
   */
  virtual void reset();

  /**
   * Update the solver.  The solver is incremental, so this method can
   * be safely called every time step.
   *
   * @return True if solver completed in this call
   */
  virtual bool solve() = 0;

protected:

  /**
   * Perform check on whether score needs to be
   * updated (even if score isn't improved, due to
   * new conditions occuring, e.g. closure of path)
   *
   * @return true if score is updated
   */
  virtual bool update_score();

  bool finish_altitude_valid(const TracePoint& start,
                             const TracePoint& finish) const;

protected:
  const TracePointVector &trace;
  const unsigned m_finish_alt_diff;
  virtual bool save_solution();

private:
  fixed best_score;
  fixed best_distance;
  fixed best_speed;
  fixed best_time;
};

#endif
