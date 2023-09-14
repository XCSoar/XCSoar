// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "PageActions.hpp"
#include "util/StringAPI.hxx"

void
InputEvents::eventPage(const TCHAR *misc)
{
  if (StringIsEqual(misc, _T("restore")))
    PageActions::Restore();
}
