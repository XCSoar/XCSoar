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
#ifndef AATISOLINEINTERCEPT_HPP
#define AATISOLINEINTERCEPT_HPP

#include "AATIsoline.hpp"

/**
 * Specialisation of AATIsoline to calculate intercepts between
 * a line extending from the aircraft to the isoline.
 *
 */
class AATIsolineIntercept: public AATIsoline
{
public:
    /** 
     * Constructor.
     * 
     * @param ap The AAT point for which the isoline is sought
     * 
     * @return Initialised object
     */
  AATIsolineIntercept(const AATPoint& ap);

/** 
 * Calculate intercept location.  Test line bearing is from previous
 * max/achieved point, through aircraft, adjusted by bearing_offset.
 * 
 * \todo
 * - adjust for bearing_offset (currently not implemented)
 *
 * @param ap AAT point associated with the isoline
 * @param state Aircraft state from which intercept line originates
 * @param bearing_offset Offset of desired bearing between cruise track from previous and intercept line
 * @param ip Set location of intercept point (if returned true)
 * 
 * @return True if intercept is found and within OZ 
 */
  bool intercept(const AATPoint& ap,
                 const AIRCRAFT_STATE &state,
                 const double bearing_offset,
                 GEOPOINT& ip) const;
};

#endif
