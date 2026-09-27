// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace InfoBoxFactory {

/**
 * A coarse topic for every InfoBox type.  The content picker lets the
 * user narrow its long list down to a few of these; a dozen entries
 * one can scan at a glance, not a taxonomy.
 *
 * The order is the order in which the picker lists the groups.
 */
enum class Group : uint8_t {
  ALTITUDE,
  VARIO,
  GLIDE,
  SPEED,
  WIND,
  WAYPOINT,
  TASK,
  TIME,
  AIRSPACE_TEAM,
  SETTING,
  SYSTEM,
  OTHER,
  COUNT
};

/**
 * A set of groups, one bit per #Group, used as the filter of the
 * content picker.  A default-constructed mask contains every group,
 * i.e. it filters nothing.
 */
class GroupMask {
  unsigned bits;

  static constexpr unsigned ALL = (1u << unsigned(Group::COUNT)) - 1;

public:
  constexpr GroupMask() noexcept:bits(ALL) {}

  constexpr bool IsAll() const noexcept {
    return bits == ALL;
  }

  constexpr bool IsEmpty() const noexcept {
    return bits == 0;
  }

  constexpr bool Contains(Group group) const noexcept {
    return bits & (1u << unsigned(group));
  }

  constexpr void Set(Group group, bool value) noexcept {
    const unsigned bit = 1u << unsigned(group);
    if (value)
      bits |= bit;
    else
      bits &= ~bit;
  }

  constexpr void SetAll() noexcept {
    bits = ALL;
  }

  constexpr void Clear() noexcept {
    bits = 0;
  }
};

/**
 * Returns the untranslated name of a group; pass it through
 * gettext() for display.
 */
[[gnu::const]]
const char *
GetGroupName(Group group) noexcept;

} // namespace InfoBoxFactory
