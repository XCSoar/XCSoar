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
#include "Profile/Map.hpp"
#include "Profile/File.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"

bool
ProfileFileHasPassword(const TCHAR *path)
{
  ProfileMap map;
  return Profile::LoadFile(map, path) && map.Exists(ProfileKeys::Password);
}

ProfilePasswordResult
CheckProfilePassword(const ProfileMap &map)
{
  /* oh no, profile passwords are not truly secure! */

  StaticString<80> profile_password;
  if (!map.Get(ProfileKeys::Password, profile_password))
      /* not password protected */
      return ProfilePasswordResult::UNPROTECTED;

  StaticString<80> user_password;
  user_password.clear();
  if (!TextEntryDialog(user_password, _("Enter your password")))
    return ProfilePasswordResult::CANCEL;

  return StringIsEqualIgnoreCase(profile_password, user_password)
    ? ProfilePasswordResult::MATCH
    : ProfilePasswordResult::MISMATCH;
}

ProfilePasswordResult
CheckProfileFilePassword(const TCHAR *path)
{
  ProfileMap map;
  if (!Profile::LoadFile(map, path))
    return ProfilePasswordResult::UNPROTECTED;

  return CheckProfilePassword(map);
}

bool
CheckProfilePasswordResult(ProfilePasswordResult result)
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
  }

  gcc_unreachable();
}

bool
SetProfilePasswordDialog(ProfileMap &map)
{
  StaticString<80> new_password;
  new_password.clear();
  if (!TextEntryDialog(new_password, _("Enter a new password")))
    return false;

  if (new_password.empty())
    map.erase(ProfileKeys::Password);
  else
    map.Set(ProfileKeys::Password, new_password);

  return true;
}
