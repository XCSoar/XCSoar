/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Util/Serial.hpp"
#include "Time/TimeoutClock.hpp"
#include "Thread/Mutex.hpp"
#include "Thread/Cond.hxx"
#include "Operation/Operation.hpp"

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

    explicit Item(const V &_value):value(_value) {}
  };

  typedef std::map<std::string, Item> Map;

  Map map;

public:
  class const_iterator {
    typename Map::const_iterator i;

  public:
    explicit const_iterator(typename Map::const_iterator _i):i(_i) {}

    const V &operator*() const {
      return i->second.value;
    }

    const V *operator->() const {
      return &i->second.value;
    }

    bool operator==(const const_iterator &other) const {
      return i == other.i;
    }

    bool operator!=(const const_iterator &other) const {
      return i != other.i;
    }
  };

  void Lock() {
    mutex.Lock();
  }

  void Unlock() {
    mutex.Unlock();
  }

  operator Mutex &() const {
    return const_cast<Mutex &>(mutex);
  }

  template<typename K>
  void MarkOld(const K &key) {
    auto i = map.find(key);
    if (i != map.end())
      i->second.old = true;
  }

  template<typename K>
  const_iterator Wait(const K &key, OperationEnvironment &env,
                      TimeoutClock timeout) {
    while (true) {
      auto i = map.find(key);
      if (i != map.end() && !i->second.old)
        return const_iterator(i);

      if (env.IsCancelled())
        return end();

      int remaining = timeout.GetRemainingSigned();
      if (remaining <= 0)
        return end();

      cond.timed_wait(*this, remaining);
    }
  }

  template<typename K>
  const_iterator Wait(const K &key, OperationEnvironment &env,
                      unsigned timeout_ms) {
    TimeoutClock timeout(timeout_ms);
    return Wait(key, env, timeout);
  }

  template<typename K>
  gcc_pure
  const_iterator find(const K &key) const {
    return const_iterator(map.find(key));
  }

  gcc_pure
  const_iterator end() const {
    return const_iterator(map.end());
  }

  template<typename K>
  void Set(const K &key, const V &value) {
    auto i = map.insert(std::make_pair(key, Item(value)));
    Item &item = i.first->second;
    item.old = false;
    if (!i.second)
      item.value = value;

    cond.broadcast();
  }

  template<typename K>
  void erase(const K &key) {
    map.erase(key);
  }

  void clear() {
    map.clear();
  }
};

#endif
