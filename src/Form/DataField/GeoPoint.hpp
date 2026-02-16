// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/CoordinateFormat.hpp"

/**
 * This #DataField implementation stores a #GeoPoint.
 */
class GeoPointDataField final : public DataField {
  GeoPoint value;

  const CoordinateFormat format;

  /**
   * For GetAsString().  Must be mutable because the method is const.
   */
  mutable char string_buffer[64];

public:
  GeoPointDataField(GeoPoint _value, CoordinateFormat _format,
                    DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::GEOPOINT, false, listener),
     value(_value), format(_format) {}

  CoordinateFormat GetFormat() const noexcept {
    return format;
  }

  GeoPoint GetValue() const noexcept {
    return value;
  }

  void SetValue(GeoPoint _value) noexcept {
    value = _value;
  }

  void ModifyValue(GeoPoint _value) noexcept;

  /* virtual methods from class DataField */
  const char *GetAsString() const noexcept override;
};
