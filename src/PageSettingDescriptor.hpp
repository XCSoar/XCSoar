// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSetting.hpp"
#include "Form/DataField/Enum.hpp"

/**
 * Value shape for a catalog setting.  Storage remains #int
 * (bool 0/1, enum choice id, or integer range value).
 */
enum class PageSettingType : uint8_t {
  ENUM,
  BOOL,
  INTEGER,
};

/**
 * One catalog entry: UI, profile keys, and live get/set.
 * Map Display → Terrain is the pilot group; extend by adding rows.
 */
struct PageSettingDescriptor {
  PageSettingId id;

  PageSettingType type;

  /** UI label (N_(); gettext when showing). */
  const char *label;

  /** Short help for the Pages editor (N_()). */
  const char *help;

  /**
   * Profile key suffix after "PageN" for per-page overrides
   * (e.g. "OverrideTerrainColors").
   */
  const char *override_key;

  /**
   * Choice list for ENUM/BOOL editors.  Pages UI prepends Global /
   * Remove.  nullptr for INTEGER (range filled from int_*).
   */
  const StaticEnumChoice *choices;

  /** Inclusive range for INTEGER; unused (0) for ENUM/BOOL. */
  int int_min;
  int int_max;
  int int_step;

  /** Read live MapSettings (or equivalent). */
  int (*GetLive)() noexcept;

  /**
   * Write live MapSettings only — no profile, no SendMapSettings.
   * Callers batch a single notify after apply.
   */
  void (*SetLive)(int value) noexcept;

  /** Persist the global profile value. */
  void (*SaveGlobalProfile)(int value) noexcept;

  /** Load the global profile value (or a safe default). */
  int (*LoadGlobalProfile)() noexcept;
};
