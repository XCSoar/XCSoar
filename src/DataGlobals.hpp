/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_DATA_GLOBALS_HPP
#define XCSOAR_DATA_GLOBALS_HPP

#include <memory>

class RaspStore;

/**
 * This namespace provides helper functions to access generic global
 * data objects.  Use them when you don't know where else to get them.
 * This is a last resort only, don't use it if you have a better way
 * to do it.
 *
 * This namespace exists to avoid direct access to #MainWindow and
 * others, because that would mean the code is not reusable in other
 * applications, while the functions in this namespace can easily be
 * replaced in another program.
 */
namespace DataGlobals {
std::shared_ptr<RaspStore> GetRasp();
void SetRasp(std::shared_ptr<RaspStore> rasp);
};

#endif
