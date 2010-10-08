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
#include "AbstractTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskPoints/StartPoint.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Task/TaskPoints/ASTPoint.hpp"
#include "Task/TaskPoints/FinishPoint.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"

#include <algorithm>

StartPoint* 
AbstractTaskFactory::createStart(ObservationZonePoint* oz,
                                 const Waypoint& wp) const
{
  return new StartPoint(oz, m_task.get_task_projection(), wp, m_behaviour,
                        get_ordered_task_behaviour());
}

FinishPoint* 
AbstractTaskFactory::createFinish(ObservationZonePoint* oz,
                                 const Waypoint& wp) const
{
  return new FinishPoint(oz, m_task.get_task_projection(), wp, m_behaviour,
                         get_ordered_task_behaviour());
}

AATPoint* 
AbstractTaskFactory::createAAT(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new AATPoint(oz, m_task.get_task_projection(), wp, m_behaviour,
                      get_ordered_task_behaviour());
}

ASTPoint* 
AbstractTaskFactory::createAST(ObservationZonePoint* oz,
                               const Waypoint& wp) const
{
  return new ASTPoint(oz, m_task.get_task_projection(), wp, m_behaviour,
                      get_ordered_task_behaviour());
}

StartPoint* 
AbstractTaskFactory::createStart(const Waypoint &wp) const
{
  return createStart(*m_start_types.begin(), wp);
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const Waypoint &wp) const
{
  if (get_ordered_task_behaviour().homogeneous_tps && (m_task.task_size()>1)) {
    LegalPointType_t type = getType(m_task.get_tp(1));
    if (validIntermediateType(type))
      return createIntermediate(type, wp);
  }
  return createIntermediate(*m_intermediate_types.begin(), wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const Waypoint &wp) const
{
  return createFinish(*m_finish_types.begin(), wp);
}

AbstractTaskFactory::LegalPointType_t 
AbstractTaskFactory::getType(const OrderedTaskPoint* point) const
{
  const ObservationZonePoint* oz = point->get_oz();

  switch (point->type) {
  case TaskPoint::START:
    switch (oz->shape) {
    case ObservationZonePoint::FAI_SECTOR:
      return START_SECTOR;

    case ObservationZonePoint::LINE:
      return START_LINE;

    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
      return START_CYLINDER;
    }
    break;

  case TaskPoint::AAT:
    switch (oz->shape) {
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::FAI_SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
    case ObservationZonePoint::LINE:
      return AAT_SEGMENT;

    case ObservationZonePoint::CYLINDER:
      return AAT_CYLINDER;
    } 
    break;

  case TaskPoint::AST:
    switch (oz->shape) {
    case ObservationZonePoint::FAI_SECTOR:
      return FAI_SECTOR;

    case ObservationZonePoint::KEYHOLE:
      return KEYHOLE_SECTOR;

    case ObservationZonePoint::BGAFIXEDCOURSE:
      return BGAFIXEDCOURSE_SECTOR;

    case ObservationZonePoint::BGAENHANCEDOPTION:
      return BGAENHANCEDOPTION_SECTOR;

    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::LINE:
      return AST_CYLINDER;
    } 
    break;

  case TaskPoint::FINISH:
    switch (oz->shape) {
    case ObservationZonePoint::FAI_SECTOR:
      return FINISH_SECTOR;

    case ObservationZonePoint::LINE:
      return FINISH_LINE;

    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
      return FINISH_CYLINDER;
    } 
    break;
  } 

  // fail, should never get here
  assert(1);
  return START_LINE;
}


OrderedTaskPoint* 
AbstractTaskFactory::createPoint(const LegalPointType_t type,
                                 const Waypoint &wp) const
{
  switch (type) {
  case START_SECTOR:
    return createStart(new FAISectorZone(wp.Location, false), wp);
  case START_LINE:
    return createStart(new LineSectorZone(wp.Location, (fixed)1000), wp);
  case START_CYLINDER:
    return createStart(new CylinderZone(wp.Location, (fixed)1000), wp);
  case FAI_SECTOR:
    return createAST(new FAISectorZone(wp.Location, true), wp);
  case KEYHOLE_SECTOR:
    return createAST(new KeyholeZone(wp.Location), wp);
  case BGAFIXEDCOURSE_SECTOR:
    return createAST(new BGAFixedCourseZone(wp.Location), wp);
  case BGAENHANCEDOPTION_SECTOR:
    return createAST(new BGAEnhancedOptionZone(wp.Location), wp);
  case AST_CYLINDER:
    return createAST(new CylinderZone(wp.Location, (fixed)500), wp);
  case AAT_CYLINDER:
    return createAAT(new CylinderZone(wp.Location), wp);
  case AAT_SEGMENT:
    return createAAT(new SectorZone(wp.Location), wp);
  case FINISH_SECTOR:
    return createFinish(new FAISectorZone(wp.Location, false), wp);
  case FINISH_LINE:
    return createFinish(new LineSectorZone(wp.Location, (fixed)1000), wp);
  case FINISH_CYLINDER:
    return createFinish(new CylinderZone(wp.Location, (fixed)1000), wp);
  };
  assert(1);
  return NULL;
}

StartPoint* 
AbstractTaskFactory::createStart(const LegalPointType_t type,
                                 const Waypoint &wp) const
{
  if (!validStartType(type))
    // error, invalid type!
    return NULL;

  return (StartPoint*)createPoint(type, wp);
}

IntermediatePoint* 
AbstractTaskFactory::createIntermediate(const LegalPointType_t type,
                                        const Waypoint &wp) const
{
  if (!validIntermediateType(type))
    return NULL;

  return (IntermediatePoint*)createPoint(type, wp);
}

FinishPoint* 
AbstractTaskFactory::createFinish(const LegalPointType_t type,
                                  const Waypoint &wp) const
{
  if (!validFinishType(type))
    return NULL;

  return (FinishPoint*)createPoint(type, wp);
}

bool 
AbstractTaskFactory::append(OrderedTaskPoint *new_tp, const bool auto_mutate)
{
  if (!new_tp)
    return false;
  if (m_task.is_max_size())
    return false;

  if (auto_mutate) {
    if (!m_task.task_size()) {
      // empty task, so add as a start point
      if (validType(new_tp, m_task.task_size())) {
        // candidate is ok, so add it
        return m_task.append(new_tp);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.append(sp);
      }
    }

    // non-empty task

    if (m_task.has_finish()) {
      // old finish must be mutated into an intermediate point
      IntermediatePoint* sp = createIntermediate(m_task.getTaskPoint(
          m_task.task_size() - 1)->get_waypoint());

      m_task.replace(sp, m_task.task_size()-1);
    }

    if (validType(new_tp, m_task.task_size()))
      // ok to append directly
      return m_task.append(new_tp);

    // this point must be mutated into a finish
    FinishPoint* sp = createFinish(new_tp->get_waypoint());
    // delete original since we own it now
    delete new_tp;
    return m_task.append(sp);
  }

  return m_task.append(new_tp);
}

bool 
AbstractTaskFactory::replace(OrderedTaskPoint *new_tp, const unsigned position,
                             const bool auto_mutate)
{
  if (!new_tp)
    return false;

  if (auto_mutate) {
    if (validType(new_tp, position))
      // ok to replace directly
      return m_task.replace(new_tp, position);

    // will need to convert type of candidate
    if (position == 0) {
      // candidate must be transformed into a startpoint
      StartPoint* sp = createStart(new_tp->get_waypoint());
      // delete original since we own it now
      delete new_tp;
      return m_task.replace(sp, position);
    } else if (position + 1 == m_task.task_size()) {
      // this point must be mutated into a finish
      FinishPoint* sp = createFinish(new_tp->get_waypoint());
      // delete original since we own it now
      delete new_tp;
      return m_task.replace(sp, position);
    } else {
      // this point must be mutated into an intermediate
      IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
      // delete original since we own it now
      delete new_tp;
      return m_task.replace(sp, position);
    }
  }

  return m_task.replace(new_tp, position);
}

bool 
AbstractTaskFactory::insert(OrderedTaskPoint *new_tp, const unsigned position,
                            const bool auto_mutate)
{
  if (!new_tp)
    return false;

  if (position >= m_task.task_size())
    return append(new_tp, auto_mutate);

  if (auto_mutate) {
    if (position == 0) {
      if (m_task.has_start()) {
        // old start must be mutated into an intermediate point
        IntermediatePoint* sp =
            createIntermediate(m_task.getTaskPoint(0)->get_waypoint());
        m_task.replace(sp, 0);
      }
      if (validType(new_tp, 0)) {
        return m_task.insert(new_tp, 0);
      } else {
        // candidate must be transformed into a startpoint
        StartPoint* sp = createStart(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.insert(sp, 0);
      }
    } else {
      if (new_tp->is_intermediate()) {
        // candidate ok for direct insertion
        return m_task.insert(new_tp, position);
      } else {
        // candidate must be transformed into a intermediatepoint
        IntermediatePoint* sp = createIntermediate(new_tp->get_waypoint());
        // delete original since we own it now
        delete new_tp;
        return m_task.insert(sp, position);
      }
    }
  }

  return m_task.insert(new_tp, position);
}

bool 
AbstractTaskFactory::remove(const unsigned position, 
                            const bool auto_mutate)
{
  if (position >= m_task.task_size())
    return false;

  if (auto_mutate) {
    if (position == 0) {
      // special case, remove start point..
      if (m_task.task_size() == 1) {
        return m_task.remove(0);
      } else {
        // create new start point from next point
        StartPoint* sp = createStart(m_task.getTaskPoint(1)->get_waypoint());
        return m_task.remove(0) && m_task.replace(sp, 0);
      }
    } else if (position + 1 == m_task.task_size()) {
      // create new finish from previous point
      FinishPoint* sp = createFinish(
          m_task.getTaskPoint(position - 1)->get_waypoint());
      return m_task.remove(position) && m_task.replace(sp, position - 1);
    } else {
      // intermediate point deleted, nothing special to do
      return m_task.remove(position);
    }
  }

  return m_task.remove(position);
}

bool 
AbstractTaskFactory::has_entered(unsigned position) const
{
  if (m_task.getTaskPoint(position))
    return m_task.getTaskPoint(position)->has_entered();

  return true;
}

bool 
AbstractTaskFactory::swap(const unsigned position, const bool auto_mutate)
{
  if (m_task.task_size() <= 1)
    return false;
  if (position >= m_task.task_size() - 1)
    return false;

  const OrderedTaskPoint* orig = m_task.getTaskPoint(position+1);
  OrderedTaskPoint* copy = orig->clone(m_behaviour, 
                                       get_ordered_task_behaviour(),
                                       orig->get_task_projection(),
                                       NULL);
  bool retval = insert(copy, position, auto_mutate);
  if (!retval) {
    delete copy;
    return false;
  }
  return remove(position+2, auto_mutate);
}

const OrderedTaskPoint&
AbstractTaskFactory::relocate(const unsigned position, 
                              const Waypoint& waypoint)
{
  m_task.relocate(position, waypoint);  
  return *m_task.getTaskPoint(position);
}

const OrderedTaskBehaviour& 
AbstractTaskFactory::get_ordered_task_behaviour() const {
  return m_task.get_ordered_task_behaviour();
}

void 
AbstractTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
}

bool 
AbstractTaskFactory::is_position_intermediate(const unsigned position) const
{
  if (is_position_start(position))
    return false;
  if (position >= get_ordered_task_behaviour().max_points)
    return false;
  if (position + 1 < get_ordered_task_behaviour().min_points)
    return true;

  if (get_ordered_task_behaviour().is_fixed_size())
    return (position + 1 < get_ordered_task_behaviour().max_points);
  else if (m_task.task_size() < get_ordered_task_behaviour().min_points)
    return true;
  else
    return (position <= m_task.task_size());
}

bool 
AbstractTaskFactory::is_position_finish(const unsigned position) const
{
  if (is_position_start(position))
    return false;

  if (position + 1 < get_ordered_task_behaviour().min_points)
    return false;
  if (position + 1 > get_ordered_task_behaviour().max_points)
    return false;

  if (get_ordered_task_behaviour().is_fixed_size())
    return (position + 1 == get_ordered_task_behaviour().max_points);
  else
    return (position + 1 >= m_task.task_size());
}

bool
AbstractTaskFactory::validAbstractType(LegalAbstractPointType_t type, 
                                       const unsigned position) const
{
  const bool is_start = is_position_start(position);
  const bool is_finish = is_position_finish(position);
  const bool is_intermediate = is_position_intermediate(position);

  switch (type) {
  case POINT_START:
    return is_start;
  case POINT_FINISH:
    return is_finish;
  case POINT_AST:
    return is_intermediate &&
      (validIntermediateType(FAI_SECTOR) 
       || validIntermediateType(AST_CYLINDER)
       || validIntermediateType(KEYHOLE_SECTOR)
       || validIntermediateType(BGAFIXEDCOURSE_SECTOR)
       || validIntermediateType(BGAENHANCEDOPTION_SECTOR));
  case POINT_AAT:
    return is_intermediate &&
      (validIntermediateType(AAT_CYLINDER) || validIntermediateType(AAT_SEGMENT));
  };
  return false;
}

bool 
AbstractTaskFactory::validType(OrderedTaskPoint *new_tp, unsigned position) const
{
  /// @todo also check OZ type is legal?

  switch (new_tp->type) {
  case TaskPoint::START:
    return validAbstractType(POINT_START, position);

  case TaskPoint::AST:
    return validAbstractType(POINT_AST, position);

  case TaskPoint::AAT:
    return validAbstractType(POINT_AAT, position);

  case TaskPoint::FINISH:
    return validAbstractType(POINT_FINISH, position);
  }

  return false;
}

bool
AbstractTaskFactory::validIntermediateType(LegalPointType_t type) const 
{
  return (std::find(m_intermediate_types.begin(), m_intermediate_types.end(), type) 
          != m_intermediate_types.end());
}

bool
AbstractTaskFactory::validStartType(LegalPointType_t type) const 
{
  return (std::find(m_start_types.begin(), m_start_types.end(), type) 
          != m_start_types.end());
}

bool
AbstractTaskFactory::validFinishType(LegalPointType_t type) const 
{
  return (std::find(m_finish_types.begin(), m_finish_types.end(), type) 
          != m_finish_types.end());
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::getValidTypes(unsigned position) const
{
  LegalPointVector v;
  if (validAbstractType(POINT_START, position))
    v.insert(v.end(), m_start_types.begin(), m_start_types.end());

  LegalPointVector i = getValidIntermediateTypes(position);
  if (!i.empty())
    v.insert(v.end(), i.begin(), i.end());

  if (validAbstractType(POINT_FINISH, position))
    v.insert(v.end(), m_finish_types.begin(), m_finish_types.end());

  return v;
}

AbstractTaskFactory::LegalPointVector 
AbstractTaskFactory::getValidIntermediateTypes(unsigned position) const
{
  LegalPointVector v;

  if (!is_position_intermediate(position))
    return v;

  if (get_ordered_task_behaviour().homogeneous_tps 
      && (position>1) && (m_task.task_size()>1)) {
    LegalPointType_t type = getType(m_task.get_tp(1));
    if (validIntermediateType(type)) {
      v.push_back(type);
      return v;
    }
  }
  if (validAbstractType(POINT_AAT, position) ||
      validAbstractType(POINT_AST, position))
    v.insert(v.end(), m_intermediate_types.begin(), m_intermediate_types.end());
  return v;
}

bool 
AbstractTaskFactory::is_closed() const
{
  if (m_task.task_size() < 3)
    return false;

  const Waypoint& wp_start = m_task.get_tp(0)->get_waypoint();
  const Waypoint& wp_finish =
      m_task.get_tp(m_task.task_size() - 1)->get_waypoint();

  return (wp_start == wp_finish);
}

bool 
AbstractTaskFactory::is_unique() const
{
  const unsigned size = m_task.task_size();
  for (unsigned i = 0; i + 1 < size; i++) {
    const Waypoint& wp_0 = m_task.get_tp(i)->get_waypoint();

    for (unsigned j = i + 1; j < size; j++) {
      if ((i == 0) && (j + 1 == size)) {
        // ok to be the same
      } else {
        const Waypoint& wp_1 = m_task.get_tp(j)->get_waypoint();
        if (wp_1 == wp_0)
          return false;
      }
    }
  }
  return true;
}

