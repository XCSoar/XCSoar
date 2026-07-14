// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/InternalLink.hpp"
#include "Dialogs/VhfLink.hpp"

bool
HandleInternalLink([[maybe_unused]] const char *uri)
{
  return false;
}

bool
HandleVhfLink([[maybe_unused]] const char *url,
              [[maybe_unused]] const char *station_name)
{
  return false;
}
