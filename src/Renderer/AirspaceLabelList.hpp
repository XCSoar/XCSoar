// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/NonCopyable.hpp"
#include "util/StaticArray.hxx"

#include <type_traits>

struct AirspaceWarningConfig;

class AirspaceLabelList : private NonCopyable {
public:
  struct Label {
    GeoPoint pos;
    AirspaceClass cls;
    AirspaceAltitude base;
    AirspaceAltitude top;
  };

  static_assert(std::is_trivial_v<Label>);

protected:
  StaticArray<Label, 512u> labels;

public:
  void Add(const GeoPoint &pos, AirspaceClass cls, const AirspaceAltitude &base,
           const AirspaceAltitude &top) noexcept;
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
