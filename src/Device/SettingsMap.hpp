// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

    template<typename VV>
    explicit constexpr Item(VV &&_value) noexcept
      :value(std::forward<VV>(_value)) {}
  };

  using Map = std::map<std::string, Item, std::less<>>;

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

  template<typename K, typename VV>
  void Set(K &&key, VV &&value) {
    auto [it, _] = map.insert_or_assign(std::forward<K>(key),
                                        Item{std::forward<VV>(value)});
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
