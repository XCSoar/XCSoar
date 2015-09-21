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

#include "ProfilePasswordDialog.hpp"
#include "TextEntry.hpp"
#include "Message.hpp"
#include "Error.hpp"
#include "Profile/Map.hpp"
#include "Profile/File.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Util/StringAPI.hxx"
#include "Util/Error.hxx"

bool
ProfileFileHasPassword(const TCHAR *path)
{
  ProfileMap map;
  return Profile::LoadFile(map, path, IgnoreError()) &&
    map.Exists(ProfileKeys::Password);
}

ProfilePasswordResult
CheckProfilePassword(const ProfileMap &map)
{
  /* oh no, profile passwords are not truly secure! */

  StringBuffer<TCHAR, 80> profile_password;
  if (!map.Get(ProfileKeys::Password, profile_password))
      /* not password protected */
      return ProfilePasswordResult::UNPROTECTED;

  StringBuffer<TCHAR, 80> user_password;
  user_password.clear();
  if (!TextEntryDialog(user_password, _("Enter your password")))
    return ProfilePasswordResult::CANCEL;

  return StringIsEqualIgnoreCase(profile_password, user_password)
    ? ProfilePasswordResult::MATCH
    : ProfilePasswordResult::MISMATCH;
}

ProfilePasswordResult
CheckProfileFilePassword(const TCHAR *path, Error &error)
{
  ProfileMap map;
  if (!Profile::LoadFile(map, path, error))
    return ProfilePasswordResult::ERROR;

  return CheckProfilePassword(map);
}

bool
CheckProfilePasswordResult(ProfilePasswordResult result, const Error &error)
{
  switch (result) {
  case ProfilePasswordResult::UNPROTECTED:
  case ProfilePasswordResult::MATCH:
    return true;

  case ProfilePasswordResult::MISMATCH:
    ShowMessageBox(_("Wrong password."), _("Password"), MB_OK);
    return false;

  case ProfilePasswordResult::CANCEL:
    return false;

  case ProfilePasswordResult::ERROR:
    ShowError(error, _("Password"));
    return false;
  }

  gcc_unreachable();
}

bool
SetProfilePasswordDialog(ProfileMap &map)
{
  StringBuffer<TCHAR, 80> new_password;
  new_password.clear();
  if (!TextEntryDialog(new_password, _("Enter a new password")))
    return false;

  if (new_password.empty())
    map.erase(ProfileKeys::Password);
  else
    map.Set(ProfileKeys::Password, new_password);

  return true;
}
