// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/TriState.hpp"
#include "util/Compiler.h"

#ifdef _WIN32
#include <windef.h> // for POINT (needed by wingdi.h)
#include <wingdi.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

class Path;
class ProfileMap;

[[gnu::pure]]
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
