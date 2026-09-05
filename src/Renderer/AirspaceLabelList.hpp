// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/NonCopyable.hpp"
#include "util/StaticArray.hxx"

#include <cstdint>
#include <type_traits>

struct AirspaceWarningConfig;

class AirspaceLabelList : private NonCopyable {
public:
  /**
   * Identifies an airspace for the lifetime of its containing Airspaces
   * store.  The renderer uses the AbstractAirspace address; its placement
   * cache is discarded whenever that store's serial changes.
   */
  using Identity = std::uintptr_t;

  struct Label {
    GeoPoint pos;
    AirspaceClass cls;
    AirspaceClass border_class;
    AirspaceAltitude base;
    AirspaceAltitude top;
    Identity identity;
  };

  static_assert(std::is_trivial_v<Label>);

protected:
  StaticArray<Label, 512u> labels;

public:
  void Add(const GeoPoint &pos, AirspaceClass cls,
           const AirspaceAltitude &base,
           const AirspaceAltitude &top, Identity identity) noexcept;

  void Add(const GeoPoint &pos, AirspaceClass cls, AirspaceClass border_class,
           const AirspaceAltitude &base,
           const AirspaceAltitude &top, Identity identity) noexcept;

  /**
   * Sort labels from highest to lowest placement priority: classes with
   * enabled warnings first, then higher bases, then their stable airspace
   * identity.
   */
  void Sort(const AirspaceWarningConfig &config) noexcept;

  void Clear() noexcept {
    labels.clear();
  }

  auto begin() const noexcept {
    return labels.begin();
  }

  auto end() const noexcept {
    return labels.end();
  }

  const Label &operator[](unsigned i) const noexcept {
    return labels[i];
  }
};
