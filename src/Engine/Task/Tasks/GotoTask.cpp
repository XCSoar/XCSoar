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

#include "GotoTask.hpp"
#include "BaseTask/UnorderedTaskPoint.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

GotoTask::GotoTask(const TaskEvents &te, 
                   const TaskBehaviour &tb,
                   TaskAdvance &ta,
                   GlidePolar &gp): 
  UnorderedTask(te,tb,ta,gp),
  tp(NULL) 
{
}

GotoTask::~GotoTask() 
{
  if (tp) {
    delete tp;
  }
}

TaskPoint* 
GotoTask::getActiveTaskPoint() const
{ 
  return tp;
}

bool 
GotoTask::validTaskPoint(const int index_offset) const
{
  if (index_offset !=0) 
    return false;
  return (tp != NULL);
}


void 
GotoTask::setActiveTaskPoint(unsigned index)
{
  // nothing to do
}


bool 
GotoTask::update_sample(const AIRCRAFT_STATE &state,
                        const bool full_update)
{
  return false; // nothing to do
}


bool 
GotoTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

bool 
GotoTask::do_goto(const Waypoint & wp)
{
  if (task_behaviour.goto_nonlandable || wp.is_landable()) {
    if (tp) {
      delete tp;
    }
    tp = new UnorderedTaskPoint(wp, task_behaviour);
    return true;
  } else {
    return false;
  }
}

void 
GotoTask::Accept(TaskPointVisitor& visitor, const bool reverse) const
{
  if (tp) {
    tp->Accept(visitor);
  }
}

unsigned 
GotoTask::task_size() const
{
  if (tp) {
    return 1;
  } else {
    return 0;
  }
}

void
GotoTask::reset()
{
  if (tp) {
    delete tp;
  }
  UnorderedTask::reset();
}
