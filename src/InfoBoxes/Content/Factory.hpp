// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Type.hpp"

#include <memory>
class InfoBoxContent;

namespace InfoBoxFactory
{
  /** A functional group for selecting an InfoBox type. */
  enum class Category {
    FLIGHT,
    NAVIGATION,
    TASK,
    THERMAL,
    WEATHER,
    TRAFFIC,
    SYSTEM,
    CHARTS,
    OTHER,
    NUM_CATEGORIES,
  };

  /** Returns the functional group of the InfoBox type. */
  [[gnu::const]]
  Category
  GetCategory(Type type) noexcept;

  /** Returns the translated-at-use-site name of an InfoBox category. */
  [[gnu::const]]
  const char *
  GetCategoryName(Category category) noexcept;

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
