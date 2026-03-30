// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Base.hpp"
#include "Engine/Waypoint/Ptr.hpp"

#include <cstddef>

enum class AlternateInfoBoxMode : uint8_t {
  AUTO,
  MANUAL,
};

/**
 * Logical slot shared by the Alternate 1 / Alternate 2 InfoBoxes and
 * all of their variants (name, glide ratio and altitude difference).
 */
enum class AlternateInfoBoxSlot : uint8_t {
  FIRST,
  SECOND,
};

[[gnu::const]]
constexpr unsigned
ToAlternateInfoBoxSlotIndex(AlternateInfoBoxSlot slot) noexcept
{
  return static_cast<unsigned>(slot);
}

[[gnu::const]]
constexpr unsigned
GetAlternateInfoBoxSlotDisplayNumber(AlternateInfoBoxSlot slot) noexcept
{
  return ToAlternateInfoBoxSlotIndex(slot) + 1;
}

static constexpr std::size_t alternate_info_box_slot_count = 2;

// Static asserts to help future maintainer to remember to modify the enum
// AND the slot count in case of an evolution
static_assert(ToAlternateInfoBoxSlotIndex(AlternateInfoBoxSlot::FIRST) == 0);
static_assert(ToAlternateInfoBoxSlotIndex(AlternateInfoBoxSlot::SECOND) == 1);
static_assert(alternate_info_box_slot_count == 2);

/**
 * Returns the current source mode for the specified alternate InfoBox
 * slot.  This state is runtime-only and defaults to AUTO on startup.
 */
AlternateInfoBoxMode
GetAlternateInfoBoxMode(AlternateInfoBoxSlot slot) noexcept;

/**
 * Sets the source mode for the specified alternate InfoBox slot.
 *
 * Switching to AUTO clears any stored manual waypoint for that slot.
 */
void
SetAlternateInfoBoxMode(AlternateInfoBoxSlot slot,
                        AlternateInfoBoxMode mode) noexcept;

/**
 * Returns the waypoint selected manually for the specified alternate
 * slot, or nullptr if none is selected.
 */
WaypointPtr
GetManualAlternateWaypoint(AlternateInfoBoxSlot slot) noexcept;

/**
 * Stores the manually selected waypoint for the specified alternate
 * slot.  The waypoint is used only while the slot is in MANUAL mode.
 */
void
SetManualAlternateWaypoint(AlternateInfoBoxSlot slot,
                           WaypointPtr waypoint) noexcept;

/**
 * Clears the manually selected waypoint for the specified alternate
 * slot.
 */
void
ClearManualAlternateWaypoint(AlternateInfoBoxSlot slot) noexcept;

/**
 * Base class for alternate-related InfoBox content that opens the
 * alternates list dialog on click.
 */
class InfoBoxContentAlternateBase : public InfoBoxContent
{
protected:
  AlternateInfoBoxSlot slot;

public:
  explicit InfoBoxContentAlternateBase(AlternateInfoBoxSlot _slot) noexcept
    :slot(_slot) {}

  bool HandleClick() noexcept override;
};

class InfoBoxContentAlternateName : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentAlternateGR : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};

class InfoBoxContentAlternateAltDiff : public InfoBoxContentAlternateBase
{
public:
  using InfoBoxContentAlternateBase::InfoBoxContentAlternateBase;

  void Update(InfoBoxData &data) noexcept override;
};
