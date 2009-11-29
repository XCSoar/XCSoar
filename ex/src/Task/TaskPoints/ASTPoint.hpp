/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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


#ifndef ASTPOINT_HPP
#define ASTPOINT_HPP

#include "Task/Tasks/BaseTask/IntermediatePoint.hpp"

/**
 * An ASTPoint is an abstract IntermediatePoint,
 * in which the observation zone area is not used for
 * scored distance calculations (the aircraft merely has
 * to enter the observation zone)
 * but does not yet have an observation zone.
 */
class ASTPoint : public IntermediatePoint 
{
public:
/** 
 * Constructor.
 * Ownership of oz is transferred to this object.  Note that AST boundaries are not scored.
 * 
 * @param _oz Observation zone for this task point
 * @param tp Projection used for internal representations
 * @param wp Waypoint associated with this task point
 * @param tb Task Behaviour defining options (esp safety heights)
 * 
 * @return Partially initialised object 
 */
  ASTPoint(ObservationZonePoint* _oz,
           const TaskProjection&tp,
           const Waypoint & wp,
           const TaskBehaviour &tb) 
    : IntermediatePoint(_oz,tp,wp,tb)
    { };

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  bool equals(const OrderedTaskPoint* other) const;

public:
  DEFINE_VISITABLE()

};

#endif
