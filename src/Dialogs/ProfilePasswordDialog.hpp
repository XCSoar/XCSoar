/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Compiler.h"

#include <tchar.h>

#ifdef WIN32
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

class ProfileMap;
class Error;

gcc_pure
bool
ProfileFileHasPassword(const TCHAR *path);

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

  /**
   * An error has occurred.
   */
  ERROR,
};

ProfilePasswordResult
CheckProfilePassword(const ProfileMap &map);

ProfilePasswordResult
CheckProfileFilePassword(const TCHAR *path, Error &error);

bool
CheckProfilePasswordResult(ProfilePasswordResult result, const Error &error);

bool
SetProfilePasswordDialog(ProfileMap &map);

#endif
