// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "InfoBoxes/Content/Type.hpp"
#include "InfoBoxes/Content/Group.hpp"
#include "Form/DataField/String.hpp"

class DataField;

/**
 * The groups the InfoBox content list is narrowed down to.  One
 * filter for the whole session: whoever picks from the map or from
 * the InfoBox arrangement sees the same choice, and one usually fills
 * several boxes from the same corner.  All groups (no filter) at
 * start.
 */
[[gnu::const]]
InfoBoxFactory::GroupMask &
GetInfoBoxGroupFilter() noexcept;

/**
 * A row for the group filter: shows "All" or the ticked groups, and
 * opens the checkbox list (InfoBoxGroupPicker) when edited - give
 * the row EditInfoBoxGroups() as its edit callback.
 */
class InfoBoxGroupsDataField final : public DataFieldString {
  InfoBoxFactory::GroupMask &mask;

public:
  explicit InfoBoxGroupsDataField(InfoBoxFactory::GroupMask &_mask,
                                  DataFieldListener *listener=nullptr) noexcept;

  InfoBoxFactory::GroupMask &GetMask() noexcept {
    return mask;
  }

  /** the mask changed: refresh the text and tell the listener */
  void Update() noexcept;
};

/**
 * WndProperty::EditCallback for an InfoBoxGroupsDataField row.
 */
bool
EditInfoBoxGroups(const char *caption, DataField &df,
                  const char *help_text) noexcept;

/**
 * Choose an InfoBox: the group filter as a row on top, the matching
 * InfoBoxes as a list below.  The current type is always listed,
 * whatever the filter.
 *
 * @param type in: the current type; out: the chosen one
 * @return true if the user chose (the type may be unchanged)
 */
bool
InfoBoxPicker(const char *caption, InfoBoxFactory::Type &type) noexcept;
