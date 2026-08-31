// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TrivialArray.hxx"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace BoundedArray {

namespace Detail {

template<typename KeyAccessor, typename Value, typename Key>
inline constexpr bool is_nothrow_key_comparison_v =
  noexcept(std::declval<KeyAccessor &>()(std::declval<Value &>()) ==
           std::declval<const Key &>());

template<typename AgeAccessor, typename T>
inline constexpr bool is_nothrow_age_comparison_v =
  noexcept(std::declval<AgeAccessor &>()(std::declval<const T &>()) <
           std::declval<AgeAccessor &>()(std::declval<const T &>()));

} // namespace Detail

/** The slot selected by AppendOrReplaceOldest(). */
template<typename T>
struct AllocationResult {
  T &value;

  /** True if value previously held an entry from a full array. */
  bool replaced;
};

/**
 * Find an entry whose explicit key equals @p key.
 */
template<typename T, std::size_t max, typename Key, typename KeyAccessor>
[[nodiscard]] constexpr T *
FindByKey(TrivialArray<T, max> &array, const Key &key,
          KeyAccessor get_key) noexcept(
            Detail::is_nothrow_key_comparison_v<KeyAccessor, T, Key>)
{
  for (auto &value : array)
    if (get_key(value) == key)
      return &value;

  return nullptr;
}

template<typename T, std::size_t max, typename Key, typename KeyAccessor>
[[nodiscard]] constexpr const T *
FindByKey(const TrivialArray<T, max> &array, const Key &key,
          KeyAccessor get_key) noexcept(
            Detail::is_nothrow_key_comparison_v<KeyAccessor, const T, Key>)
{
  for (const auto &value : array)
    if (get_key(value) == key)
      return &value;

  return nullptr;
}

/**
 * Append an uninitialised slot, or select the oldest entry for replacement
 * when @p array is full.  Equal ages deterministically select the first
 * entry.  The caller remains responsible for cleaning up and initialising
 * the returned slot.
 */
template<typename T, std::size_t max, typename AgeAccessor>
[[nodiscard]] constexpr AllocationResult<T>
AppendOrReplaceOldest(TrivialArray<T, max> &array,
                      AgeAccessor get_age) noexcept(
                        Detail::is_nothrow_age_comparison_v<AgeAccessor, T>)
{
  static_assert(max > 0);

  if (!array.full())
    return {array.append(), false};

  const auto oldest = std::min_element(
    array.begin(), array.end(),
    [&get_age](const T &a, const T &b) noexcept(
      Detail::is_nothrow_age_comparison_v<AgeAccessor, T>) {
      return get_age(a) < get_age(b);
    });
  assert(oldest != array.end());
  return {*oldest, true};
}

} // namespace BoundedArray
