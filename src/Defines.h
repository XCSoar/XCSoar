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
 * Normally GlidePolar::bestld * 0.7 makes an efficiency 40 become 28 as a safety margin
 */
#define SAFELD_FACTOR	0.7

/**
 * Quiet sound mode under safetyaltitude + QUIETMARGIN
 * (in meters)
 */
#define ALTERNATE_QUIETMARGIN 250

/**
 * Additional height over arrival altitude needed for safety searches and green
 * and blue Ok status. This is really needed since car GPS or non-baro GPS
 * may be in error of several meters, and we can't accept a +30m over safety
 * if the gps altitude is wrong!!
 * (in meters)
 */
#define ALTERNATE_OVERSAFETY  100
/**
 * bestalternate search is within this maximum limit
 * (in kilometers)
 */
#define ALTERNATE_MAXRANGE    100

/** seconds between infobox dynamic flipping */
#define DYNABOXTIME 3.0

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
/** must be < than DCI/2 to have a chance to make airspace click recon!!! */
#define VKSHORTCLICK 120
/** triggers circling/cruis switch on aircraft icon */
#define VKLONGCLICK 1500
/** interval to look only for airspace and not WP (IF NOT USING VK) */
#define AIRSPACECLICK 1000


/** interval in seconds between BA search (slow) */
#define BESTALTERNATEINTERVAL 60.0

#define XCSDATADIR	"XCSoarData"

/** Max items in any rotary buffer (used in filters) */
#define RASIZE 180

/**
 * Max number of iterations during a filter convergence search
 * just for safety, it should stop at 3-4 automatically
 */
#define MAXITERFILTER 10

/** max size of rotary buffer for LD calculation */
#define MAXLDROTARYSIZE 180

/** over this, show INVALID_GR */
#define MAXEFFICIENCYSHOW 200

#endif
