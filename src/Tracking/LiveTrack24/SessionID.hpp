/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in thenv, that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef LIVETRACK24_SESSION_ID_HPP
#define LIVETRACK24_SESSION_ID_HPP

#include <cstdint>

namespace LiveTrack24 {

typedef uint32_t UserID;
typedef uint32_t SessionID;

/** Generates a random session id */
SessionID
GenerateSessionID() noexcept;

/** Generates a random session id containing the given user id */
SessionID
GenerateSessionID(UserID user_id) noexcept;

} // namespace Livetrack24

#endif
