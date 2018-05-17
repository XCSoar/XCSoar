/* Copyright_License {

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

#ifndef XCSOAR_ACTIVE_AIRSPACE_PREDICATE_HPP
#define XCSOAR_ACTIVE_AIRSPACE_PREDICATE_HPP

#include "Compiler.h"

class ProtectedAirspaceWarningManager;
class AbstractAirspace;

/**
 * Match only airspaces that are "active", see
 * AirspaceWarningManager::IsActive() for a definition.
 *
 * The ProtectedAirspaceWarningManager attribute is optional.  It will
 * only query AbstractAirspace::IsActive() if the
 * ProtectedAirspaceWarningManager is nullptr.
 */
class ActiveAirspacePredicate {
  const ProtectedAirspaceWarningManager *warnings;

public:
  constexpr
  ActiveAirspacePredicate(const ProtectedAirspaceWarningManager *_warnings)
    :warnings(_warnings) {}

  gcc_pure
  bool operator()(const AbstractAirspace &airspace) const;
};

#endif
