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

#ifndef XCSOAR_FORM_UTIL_HPP
#define XCSOAR_FORM_UTIL_HPP

#include <tchar.h>

class WndForm;

/**
 * Loads the specified value into the form.
 *
 * @param form the form
 * @param control_name the name of the control in the form
 * @param value the new value
 */
void
LoadFormProperty(WndForm &form, const TCHAR *control_name, bool value);
void
LoadFormProperty(WndForm &form, const TCHAR *control_name, int value);
void
LoadFormProperty(WndForm &form, const TCHAR *control_name, unsigned int value);
void
LoadFormProperty(WndForm &form, const TCHAR *control_name, double value);

/**
 * Saves a form value into a variable and into the registry.
 *
 * @param form the form
 * @param control_name the name of the control in the form
 * @param value the new value
 * @param registry_name the name of the registry key
 * @return true if the value has been modified
 */
bool
SaveFormProperty(const WndForm &form, const TCHAR *control_name,
                 bool &value, const TCHAR *registry_name);

#endif
