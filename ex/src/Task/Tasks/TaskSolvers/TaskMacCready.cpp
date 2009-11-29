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
#include "TaskMacCready.hpp"
#include "TaskSolution.hpp"
#include <algorithm>

TaskMacCready::TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                             const unsigned _activeTaskPoint,
                             const GlidePolar &gp):
  m_tps(_tps.begin(),_tps.end()),
  m_gs(_tps.size()),
  m_minHs(_tps.size(),0.0),
  m_activeTaskPoint(_activeTaskPoint),
  m_start(0),
  m_end(std::max((int)_tps.size(),1)-1),
  m_glide_polar(gp)
{
}

TaskMacCready::TaskMacCready(TaskPoint* tp,
                             const GlidePolar &gp):
  m_tps(1, tp),
  m_gs(1),
  m_minHs(1,0.0),
  m_activeTaskPoint(0),
  m_start(0),
  m_end(0),
  m_glide_polar(gp)
{
}


void
TaskMacCready::clearance_heights(const AIRCRAFT_STATE &aircraft)
{
  // set min heights (earliest climb possible)
  double minH = get_min_height(aircraft);
  for (int i=m_end; i>=m_start; i--) {
    minH = std::max(minH,m_tps[i]->get_elevation());
    m_minHs[i] = minH;
  }
  // set min heights (ensure clearance possible for latest glide)
  for (int i=m_start+1; i<=m_end; i++) {
    if (m_minHs[i-1]>m_minHs[i]) {
      AIRCRAFT_STATE aircraft_predict = aircraft;
      aircraft_predict.Altitude = m_minHs[i-1];
      GlideResult gr = tp_solution(i, aircraft_predict, m_minHs[i]);
      double dh = aircraft_predict.Altitude-gr.HeightGlide;
      if (m_minHs[i]+0.01<dh) {
        m_minHs[i] = dh;
        i--; 
        continue; // recalculate again for remainder
      }
    }
  }
}


GlideResult 
TaskMacCready::glide_solution(const AIRCRAFT_STATE &aircraft) 
{
  GlideResult acc_gr, gr;
  AIRCRAFT_STATE aircraft_predict = aircraft;
  AIRCRAFT_STATE aircraft_start = get_aircraft_start(aircraft);

  clearance_heights(aircraft);

  double excess_height = 0.0;
  excess_height = aircraft_start.Altitude-m_minHs[m_end];

  for (int i=m_end; i>=m_start; i--) {
    if (i>m_start) {
      aircraft_predict.Altitude = m_minHs[i-1]+std::max(excess_height,0.0);
    } else {
      aircraft_predict.Altitude = std::min(aircraft_start.Altitude,
                                           m_minHs[i]+std::max(excess_height,0.0));
    }

    // perform estimate, ensuring that alt is above previous taskpoint  
    gr = tp_solution(i,aircraft_predict, m_minHs[i]);
    m_gs[i] = gr;
    excess_height -= gr.HeightGlide;

    // update state
    if (i==m_end) {
      acc_gr = gr;
    } else if (i>m_start) {
      acc_gr.add(gr);
    }

    if (gr.Solution != GlideResult::RESULT_OK) {
      i = std::max(m_start,1)-1; // quick exit
    }

  }
  if (m_end>m_start) {
    gr.add(acc_gr);
  }

  aircraft_predict.Altitude = aircraft_start.Altitude;
  double alt_difference = aircraft_start.Altitude-m_minHs[m_start];
  for (int i=m_start; i<=m_end; i++) {
    aircraft_predict.Altitude -= m_gs[i].HeightGlide;
    alt_difference = std::min(alt_difference, aircraft_predict.Altitude-m_minHs[i]);
  }
  alt_difference -= gr.HeightClimb;
  gr.AltitudeDifference = alt_difference;
  gr.calc_cruise_bearing();
  return gr;
}


GlideResult 
TaskMacCready::glide_sink(const AIRCRAFT_STATE &aircraft,
                          const double S) 
{
  AIRCRAFT_STATE aircraft_predict = aircraft;
  GlideResult acc_gr;
  for (int i=m_start; i<=m_end; i++) {
    GlideResult gr = tp_sink(i, aircraft_predict, S);

    aircraft_predict.Altitude -= gr.HeightGlide;
    if (i==m_start) {
      acc_gr = gr;
    } else {
      acc_gr.AltitudeDifference = std::min(acc_gr.AltitudeDifference,
                                           gr.AltitudeDifference);
    }
  }
  return acc_gr;
}

GlideResult 
TaskMacCready::tp_sink(const unsigned i,
                       const AIRCRAFT_STATE &aircraft, 
                       const double S) const
{
  return TaskSolution::glide_solution_sink(*m_tps[i], aircraft, m_glide_polar, S);
}
