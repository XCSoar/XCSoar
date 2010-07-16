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

#ifndef XCSOAR_DEFINES_H
#define XCSOAR_DEFINES_H

#define INVALID_GR	999

/**
 * Over this GR value, alternates and Final GR are not showing numbers anymore and
 * the variable is set to INVALIDVALUE
 */
#define ALTERNATE_MAXVALIDGR 200

/**
 * DOUBLECLICKINTERVAL is a general purpose timing, used by both VK and synthetic double click
 *
 * No miracles. Couldn't do it any better.
 *
 * Max interval in ms between two clicks for getting a double click
 * Very careful changing this value. It is used by virtual keys also!
 * This is the timing sequence for virtual keys:
 *
 *   0 - VKSHORTCLICK     single click on the waypoint
 *   0 - DCI / 2 + 30     double click detected and VK disabled
 *   DCI / 2 + 30 - DCI   airspace click and double click conflict?
 *   < DCI                possible double click
 *   > DCI                virtual key
 */
#define DOUBLECLICKINTERVAL 350

/** over this, show INVALID_GR */
#define MAXEFFICIENCYSHOW 200

#endif
