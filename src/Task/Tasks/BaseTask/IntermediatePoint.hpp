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

#ifndef INTERMEDIATEPOINT_H
#define INTERMEDIATEPOINT_H
#include "OrderedTaskPoint.hpp"

/**
 * An IntermediatePoint is an abstract OrderedTaskPoint,
 * does not yet have an observation zone, nor defines
 * how scoring is performed within.
 * All IntermediatePoints shall have a preceding and following
 * taskpoint.
 *
 */
class IntermediatePoint: 
  public OrderedTaskPoint 
{
public:    
/** 
 * Constructor.
 * 
 * @param _oz Observation zone attached to this point
 * @param tp Global projection 
 * @param wp Waypoint origin of turnpoint
 * @param tb Task Behaviour defining options (esp safety heights)
 * @param b_scored Whether distance within OZ is scored 
 * 
 * @return Partially-initialised object
 */

  IntermediatePoint(ObservationZonePoint* _oz,
                    const TaskProjection& tp,
                    const Waypoint & wp, 
                    const TaskBehaviour& tb,
                    const bool b_scored=false): 
    OrderedTaskPoint(_oz, tp, wp, tb, b_scored) 
    {};

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.
 * 
 * @return Minimum allowable elevation of task point
 */
  fixed get_elevation() const;

};
#endif //INTERMEDIATEPOINT_H
