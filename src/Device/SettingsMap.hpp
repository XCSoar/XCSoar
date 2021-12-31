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

#ifndef XCSOAR_DEVICE_SETTINGS_MAP_HPP
#define XCSOAR_DEVICE_SETTINGS_MAP_HPP

#include "util/Serial.hpp"
#include "time/TimeoutClock.hpp"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"
#include "Operation/Operation.hpp"
#include "Operation/Cancelled.hpp"

#include <map>
#include <string>
#include <algorithm>

/**
 * A thread-safe store for named values received from a device.  The
 * driver's NMEA parser fills values, and another thread may wait for
 * this.
 *
 * The caller is responsible for locking and unlocking an instance for
 * each method call.
 */
template<typename V>
class DeviceSettingsMap {
  Mutex mutex;

  Cond cond;

  struct Item {
    V value;

    bool old;

    explicit constexpr Item(const V &_value) noexcept:value(_value) {}
  };

  typedef std::map<std::string, Item> Map;

  Map map;

public:
  class const_iterator {
    typename Map::const_iterator i;

  public:
    explicit constexpr const_iterator(typename Map::const_iterator _i) noexcept
      :i(_i) {}

    constexpr const V &operator*() const noexcept {
      return i->second.value;
    }

    constexpr const V *operator->() const noexcept {
      return &i->second.value;
    }

    constexpr bool operator==(const const_iterator &other) const noexcept {
      return i == other.i;
    }

    constexpr bool operator!=(const const_iterator &other) const noexcept {
      return i != other.i;
    }
  };

  operator Mutex &() const noexcept {
    return const_cast<Mutex &>(mutex);
  }

  template<typename K>
  void MarkOld(const K &key) noexcept {
    if (auto i = map.find(key); i != map.end())
      i->second.old = true;
  }

  template<typename K>
  const_iterator Wait(std::unique_lock<Mutex> &lock,
                      const K &key, OperationEnvironment &env,
                      TimeoutClock timeout) {
    while (true) {
      if (auto i = map.find(key); i != map.end() && !i->second.old)
        return const_iterator(i);

      if (env.IsCancelled())
        throw OperationCancelled{};

      const auto remaining = timeout.GetRemainingSigned();
      if (remaining.count() <= 0)
        return end();

      cond.wait_for(lock, remaining);
    }
  }

  template<typename K, class Rep, class Period>
  const_iterator Wait(std::unique_lock<Mutex> &lock,
                      const K &key, OperationEnvironment &env,
                      const std::chrono::duration<Rep,Period> &_timeout) {
    TimeoutClock timeout(_timeout);
    return Wait(lock, key, env, timeout);
  }

  template<typename K>
  [[gnu::pure]]
  const_iterator find(const K &key) const noexcept {
    return const_iterator(map.find(key));
  }

  [[gnu::pure]]
  const_iterator end() const noexcept {
    return const_iterator(map.end());
  }

  template<typename K>
  void Set(const K &key, const V &value) {
    auto [it, _] = map.insert_or_assign(key, Item(value));
    Item &item = it->second;
    item.old = false;

    cond.notify_all();
  }

  template<typename K>
  void erase(const K &key) noexcept {
    map.erase(key);
  }

  void clear() noexcept {
    map.clear();
  }
};

#endif
