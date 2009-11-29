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
#ifndef AATISOLINESEGMENT_HPP
#define AATISOLINESEGMENT_HPP

#include "AATIsoline.hpp"


/**
 *  Specialisation of AATIsoline such that the segment of
 *  the isoline within the task point's observation zone is
 *  determined.  This allows for parametric representation
 *  of all points along the isoline within the OZ.
 * 
 *  End-points of segments are searched for and so this
 *  class is slow to instantiate.
 */
class AATIsolineSegment: public AATIsoline
{
public:
    /** 
     * Constructor.  This performs the search for the isoline
     * segment and so is slow.
     * 
     * @param ap The AAT point for which the isoline is sought
     * 
     * @return Initialised object
     */
  AATIsolineSegment(const AATPoint& ap);

/** 
 * Test whether segment is valid (nonzero length)
 * 
 * @return True if segment is valid
 */
  bool valid() const;

/** 
 * Parametric representation of points on the isoline segment.
 * 
 * @param t Parameter (0,1)
 * 
 * @return Location of point on isoline segment
 */
  GEOPOINT parametric(const double t) const;

private:
  double t_up;
  double t_down;
};

#endif
