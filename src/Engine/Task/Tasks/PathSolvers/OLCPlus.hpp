/* Copyright_License {

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

#ifndef OLC_PLUS_HPP
#define OLC_PLUS_HPP

#include "AbstractContest.hpp"

/**
 * Abstract class for contest searches using dijkstra algorithm
 *
 */
class OLCPlus:
  public AbstractContest
{
  ContestTraceVector solution_classic;
  ContestTraceVector solution_fai;
  ContestResult result_classic;
  ContestResult result_fai;

public:
  OLCPlus(const Trace &_trace, const unsigned &_handicap);

  virtual void copy_solution(ContestTraceVector &vec) const;

protected:
  virtual fixed calc_distance() const;
  virtual fixed calc_score() const;
  virtual fixed calc_time() const;

public:
  virtual void reset();

  bool solve(bool exhaustive);

  ContestTraceVector& get_solution_classic() {
    return solution_classic;
  }
  ContestTraceVector& get_solution_fai() {
    return solution_fai;
  }
  ContestResult& get_result_classic() {
    return result_classic;
  }
  ContestResult& get_result_fai() {
    return result_fai;
  }
};

#endif
