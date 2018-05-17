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

#ifndef XCSOAR_UNITS_STORE_HPP
#define XCSOAR_UNITS_STORE_HPP

#include "Compiler.h"
#include <tchar.h>

struct UnitSetting;

/**
 * Namespace to manage units presets.
 */
namespace Units
{
  namespace Store
  {
    gcc_const
    const TCHAR* GetName(unsigned i);

    gcc_const
    const UnitSetting& Read(unsigned i);

    gcc_const
    unsigned Count();

    /**
     * Only the units part of the structure is addressed.
     * @return Index + 1 if an equivalent set is found, else 0.
     */
    gcc_pure
    unsigned EqualsPresetUnits(const UnitSetting &config);
  }
}

#endif
