/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "TaskVarioComputer.hpp"
#include "Task/Stats/TaskVario.hpp"
#include "GlideSolvers/GlideResult.hpp"

TaskVarioComputer::TaskVarioComputer()
  :df(0),
   v_lpf(120, false)
{
}

void 
TaskVarioComputer::update(TaskVario &data, const GlideResult &solution)
{
  auto v = df.Update(solution.altitude_difference);
  data.value = v_lpf.Update(v);
}

void 
TaskVarioComputer::reset(TaskVario &data, const GlideResult& solution)
{
  v_lpf.Reset(0);
  df.Reset(solution.altitude_difference, 0);
  data.Reset();
}
