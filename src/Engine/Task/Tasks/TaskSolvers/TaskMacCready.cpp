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
#include "TaskMacCready.hpp"
#include "TaskSolution.hpp"
#include <algorithm>

TaskMacCready::TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                             const unsigned _activeTaskPoint,
                             const GlidePolar &gp):
  m_tps(_tps.begin(), _tps.end()),
  m_gs(_tps.size()),
  m_minHs(_tps.size(), fixed_zero),
  m_activeTaskPoint(_activeTaskPoint),
  m_start(0),
  m_end(max((int)_tps.size(), 1) - 1),
  m_glide_polar(gp) {}

TaskMacCready::TaskMacCready(TaskPoint* tp, const GlidePolar &gp):
  m_tps(1, tp),
  m_gs(1),
  m_minHs(1, fixed_zero),
  m_activeTaskPoint(0),
  m_start(0),
  m_end(0),
  m_glide_polar(gp) {}

TaskMacCready::TaskMacCready(const std::vector<TaskPoint*> &_tps,
                             const GlidePolar &gp):
  m_tps(_tps.begin(), _tps.end()),
  m_gs(_tps.size()),
  m_minHs(_tps.size(), fixed_zero),
  m_activeTaskPoint(0),
  m_start(0),
  m_end(max((int)_tps.size(), 1) - 1),
  m_glide_polar(gp) {}

void
TaskMacCready::clearance_heights(const AIRCRAFT_STATE &aircraft)
{
  #define fixed_tolerance fixed(0.01)

  // set min heights (earliest climb possible)
  fixed minH = get_min_height(aircraft);
  for (int i = m_end; i >= m_start; --i) {
    minH = max(minH, m_tps[i]->get_elevation());
    m_minHs[i] = minH;
  }
  // set min heights (ensure clearance possible for latest glide)
  for (int i = m_start; i < m_end; ++i) {
    if (m_minHs[i] > m_minHs[i + 1]) {
      AIRCRAFT_STATE aircraft_predict = aircraft;
      aircraft_predict.NavAltitude = m_minHs[i];
      const GlideResult gr = tp_solution(i, aircraft_predict, m_minHs[i + 1]);
      const fixed dh = aircraft_predict.NavAltitude - gr.height_glide;
      if (m_minHs[i + 1] + fixed_tolerance < dh) {
        m_minHs[i + 1] = dh;
        // recalculate again for remainder
        i--;
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

  fixed excess_height = aircraft_start.NavAltitude - m_minHs[m_end];

  for (int i = m_end; i >= m_start; --i) {
    if (i > m_start)
      aircraft_predict.NavAltitude =
          m_minHs[i - 1] + max(excess_height, fixed_zero);
    else
      aircraft_predict.NavAltitude =
          min(aircraft_start.NavAltitude,
              m_minHs[i] + max(excess_height, fixed_zero));

    // perform estimate, ensuring that alt is above previous taskpoint  
    gr = tp_solution(i, aircraft_predict, m_minHs[i]);
    m_gs[i] = gr;
    excess_height -= gr.height_glide;

    // update state
    if (i == m_end)
      acc_gr = gr;
    else if (i > m_start)
      acc_gr.Add(gr);

    if (gr.validity != GlideResult::RESULT_OK)
      i = max(m_start, 1) - 1; // quick exit
  }

  if (m_end > m_start)
    gr.Add(acc_gr);

  aircraft_predict.NavAltitude = aircraft_start.NavAltitude;
  fixed alt_difference = aircraft_start.NavAltitude - m_minHs[m_start];

  for (int i = m_start; i <= m_end; ++i) {
    aircraft_predict.NavAltitude -= m_gs[i].height_glide;
    alt_difference =
        min(alt_difference, aircraft_predict.NavAltitude - m_minHs[i]);
  }

  alt_difference -= gr.height_climb;
  gr.altitude_difference = alt_difference;
  gr.CalcDeferred(aircraft);

  return gr;
}

GlideResult 
TaskMacCready::glide_sink(const AIRCRAFT_STATE &aircraft, const fixed S)
{
  AIRCRAFT_STATE aircraft_predict = aircraft;
  GlideResult acc_gr;

  for (int i = m_start; i <= m_end; ++i) {
    const GlideResult gr = tp_sink(i, aircraft_predict, S);

    aircraft_predict.NavAltitude -= gr.height_glide;
    if (i == m_start)
      acc_gr = gr;
    else
      acc_gr.altitude_difference =
          min(acc_gr.altitude_difference, gr.altitude_difference);
  }

  return acc_gr;
}

GlideResult 
TaskMacCready::tp_sink(const unsigned i,
                       const AIRCRAFT_STATE &aircraft, 
                       const fixed S) const
{
  return TaskSolution::glide_solution_sink(*m_tps[i], aircraft, m_glide_polar, S);
}

const GlideResult&
TaskMacCready::get_active_solution(const AIRCRAFT_STATE &aircraft) 
{
  m_gs[m_activeTaskPoint].CalcDeferred(aircraft);
  return m_gs[m_activeTaskPoint];
}
