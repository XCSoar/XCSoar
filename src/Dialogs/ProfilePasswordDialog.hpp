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

#ifndef XCSOAR_PROFILE_PASSWORD_DIALOG_HPP
#define XCSOAR_PROFILE_PASSWORD_DIALOG_HPP

#include "Util/TriState.hpp"
#include "Compiler.h"

#ifdef WIN32
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

class Path;
class ProfileMap;

gcc_pure
TriState
ProfileFileHasPassword(Path path);

enum class ProfilePasswordResult {
  /**
   * The profile is not password-protected.
   */
  UNPROTECTED,

  /**
   * User has entered the correct password.
   */
  MATCH,

  /**
   * The user has entered an incorrect password.  Caller is
   * responsible for telling the user.
   */
  MISMATCH,

  /**
   * The user has cancelled the password dialog.
   */
  CANCEL,
};

ProfilePasswordResult
CheckProfilePassword(const ProfileMap &map);

/**
 * Throws std::runtime_errror on error.
 */
ProfilePasswordResult
CheckProfileFilePassword(Path path);

bool
CheckProfilePasswordResult(ProfilePasswordResult result);

bool
SetProfilePasswordDialog(ProfileMap &map);

#endif
