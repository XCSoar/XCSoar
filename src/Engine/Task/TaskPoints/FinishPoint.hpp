/*
  Copyright_License {

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


#ifndef FINISHPOINT_HPP
#define FINISHPOINT_HPP

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"

/**
 * A FinishPoint is an abstract OrderedTaskPoint,
 * can manage finish transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present following a FinishPoint.
 *
 * Entry requires previous point to have entered to prevent spurious crossing.
 *
 * \todo
 * - currently we don't track crossing the actual line, rather it currently
 *   allows any border crossing
 * - adjustments in FAI finish for min finish height
 * - consider making all OTP's require previous tps to have entered for this to
 */
class FinishPoint : public OrderedTaskPoint {
public:
/** 
 * Constructor.  Sets task area to non-scorable; distances
 * are relative to crossing point or origin.
 * 
 * @param _oz Observation zone for this task point
 * @param tp Global projection 
 * @param wp Waypoint origin of turnpoint
 * @param tb Task Behaviour defining options (esp safety heights)
 * 
 * @return Partially-initialised object
 */
    FinishPoint(ObservationZonePoint* _oz,
                const TaskProjection& tp,
                const Waypoint & wp,
                const TaskBehaviour& tb) : 
      OrderedTaskPoint(_oz,tp,wp,tb) { };

/** 
 * Set previous/next taskpoints in sequence.
 * Specialises base method to check next is NULL.
 * 
 * @param prev Previous task point 
 * @param next Next task point (must be null!)
 */
  void set_neighbours(OrderedTaskPoint* prev,
                      OrderedTaskPoint* next);

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.
 * 
 * @return Minimum allowable elevation of start point
 */
  fixed get_elevation() const;

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  bool equals(const OrderedTaskPoint* other) const;

private:
  bool score_first_entry() const {
    return true;
  }
  bool entry_precondition() const;

public:
  DEFINE_VISITABLE()
};

#endif
