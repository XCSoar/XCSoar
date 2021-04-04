/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

/**
 * Basic container class for storage of X-Y data pairs
 */

#ifndef _XYDATASTORE_H
#define _XYDATASTORE_H

#include "util/TrivialArray.hxx"

#include <type_traits>
#define LEASTSQS_WEIGHT_STORE

class XYDataStore
{
protected:
  double sum_xw, sum_yw;
  double sum_weights;

  double y_max;
  double y_min;
  double x_min;
  double x_max;

  unsigned sum_n;

  struct Slot {
    double x, y;

#ifdef LEASTSQS_WEIGHT_STORE
    double weight;
#endif

    Slot() = default;

    constexpr Slot(double _x, double _y, double _weight) noexcept
      :x(_x), y(_y)
#ifdef LEASTSQS_WEIGHT_STORE
      , weight(_weight)
#endif
    {}
  };

  TrivialArray<Slot, 1000> slots;

public:
  constexpr bool IsEmpty() const noexcept {
    return sum_n == 0;
  }

  constexpr bool HasResult() const noexcept {
    return sum_n >= 2;
  }

  constexpr unsigned GetCount() const noexcept {
    return sum_n;
  }

  /**
   * Reset the store.
   */
  void StoreReset() noexcept;

  constexpr double GetMinX() const noexcept {
    return x_min;
  }

  constexpr double GetMaxX() const noexcept {
    return x_max;
  }

  constexpr double GetMiddleX() const noexcept {
    return (x_min + x_max) / 2.;
  }

  constexpr double GetMinY() const noexcept {
    return y_min;
  }

  constexpr double GetMaxY() const noexcept {
    return y_max;
  }

  constexpr ConstBuffer<Slot> GetSlots() const noexcept {
    return slots;
  }

protected:

  /**
   * Add a new data point to the values.
   *
   * @param x x-Value of the new data point
   * @param y y-Value of the new data point
   * @param weight Weight of the new data point (optional)
   */
  void StoreAdd(double x, double y, double weight=1) noexcept;

  /**
   * Remove data point to the values.
   * If weights aren't stored, this assumes weight = 1
   */
  void StoreRemove(const unsigned i) noexcept;

};

static_assert(std::is_trivial<XYDataStore>::value, "type is not trivial");

#endif // _XYDATASTORE_H
