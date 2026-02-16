// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProfilePasswordDialog.hpp"
#include "TextEntry.hpp"
#include "Message.hpp"
#include "Error.hpp"
#include "Profile/Map.hpp"
#include "Profile/File.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "system/Path.hpp"
#include "util/StringAPI.hxx"

TriState
ProfileFileHasPassword(Path path)
{
  ProfileMap map;

  try {
    Profile::LoadFile(map, path);
  } catch (...) {
    return TriState::UNKNOWN;
  }

  return map.Exists(ProfileKeys::Password)
    ? TriState::TRUE
    : TriState::FALSE;
}

ProfilePasswordResult
CheckProfilePassword(const ProfileMap &map)
{
  /* oh no, profile passwords are not truly secure! */

  BasicStringBuffer<char, 80> profile_password;
  if (!map.Get(ProfileKeys::Password, profile_password))
      /* not password protected */
      return ProfilePasswordResult::UNPROTECTED;

  BasicStringBuffer<char, 80> user_password;
  user_password.clear();
  if (!TextEntryDialog(user_password, _("Enter your password")))
    return ProfilePasswordResult::CANCEL;

  return StringIsEqualIgnoreCase(profile_password, user_password)
    ? ProfilePasswordResult::MATCH
    : ProfilePasswordResult::MISMATCH;
}

ProfilePasswordResult
CheckProfileFilePassword(Path path)
{
  ProfileMap map;
  Profile::LoadFile(map, path);
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
  BasicStringBuffer<char, 80> new_password;
  new_password.clear();
  if (!TextEntryDialog(new_password, _("Enter a new password")))
    return false;

  if (new_password.empty())
    map.Remove(ProfileKeys::Password);
  else
    map.Set(ProfileKeys::Password, new_password);

  return true;
}
