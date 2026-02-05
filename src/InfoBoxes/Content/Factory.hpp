// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Type.hpp"

#include <memory>
#include <tchar.h>

class InfoBoxContent;

namespace InfoBoxFactory
{
  /**
   * Returns the human-readable name of the info box type.
   */
  [[gnu::const]]
  const char *
  GetName(Type type) noexcept;

  /**
   * Returns the default caption of the info box type.  This is
   * usually a shorter version of the string returned by GetName(), to
   * fit in the small #InfoBoxWindow.
   */
  [[gnu::const]]
  const char *
  GetCaption(Type type) noexcept;

  /**
   * Returns the long description (help text) of the info box type.
   */
  [[gnu::const]]
  const char *
  GetDescription(Type type) noexcept;

  std::unique_ptr<InfoBoxContent> Create(Type infobox_type) noexcept;
};
