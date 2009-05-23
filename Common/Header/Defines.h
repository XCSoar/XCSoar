/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}
 */

#ifndef XCSOAR_DEFINES_H
#define XCSOAR_DEFINES_H
/* VENTA3
 * General defines for XCSoar
 * Paolo Ventafridda
 *
 * We put here whatever can be fine tuned. Each .cpp using one of these values should
 * include this file at the beginning.
 *
 * Some of these values could be good candidates for system config menus
 */

#define INVALID_GR	999

/*
 * Over this GR value, alternates and Final GR are not showing numbers anymore and
 * the variable is set to INVALIDVALUE
 */
#define ALTERNATE_MAXVALIDGR 200

/*
 * Normally GlidePolar::bestld * 0.7 makes an efficiency 40 become 28 as a safety margin
 */
#define SAFELD_FACTOR	0.7
#define ALTERNATE_QUIETMARGIN 250 // meters - Quiet sound mode under safetyaltitude+QUIETMARGIN

/*
 * Additional height over arrival altitude needed for safety searches and green&blue Ok status
 * This is really needed since car GPS or non-baro GPS may be in error of several meters, and
 * we can't accept a +30m over safety if the gps altitude is wrong!!
 */
#define ALTERNATE_OVERSAFETY  100 // meters
#define ALTERNATE_MAXRANGE    100 // km - bestalternate search is within this maximum limit
#define DYNABOXTIME 3.0  // seconds between infobox dynamic flipping
#define DOUBLECLICKINTERVAL 350 // ms max interval between two clicks for getting a double click
#define BESTALTERNATEINTERVAL 60.0 // interval in seconds between BA search (slow)

/*
 * Global defines
 */
// Why MPSR? (historical, it was back when it was commercial!)
#define REGKEYNAME	"Software\\MPSR\\XCSoar"
#define XCSDATADIR	"XCSoarData"
#define XCSCHKLIST	"xcsoar-checklist.txt"
#define XCSPROFILE	"xcsoar-registry.prf"

#endif
