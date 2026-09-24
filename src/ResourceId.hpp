// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

/**
 * The identifier for a resource to be passed to
 * ResourceLoader::Load() or other resource-loading functions.
 */
class ResourceId {
#ifdef ANDROID
  const char *name;
#else
  const std::byte *begin;
  const size_t *size_ptr;
#endif

public:
  ResourceId() = default;

#ifdef ANDROID
  constexpr explicit ResourceId(const char *_name) noexcept
    :name(_name) {}
#else
  constexpr explicit ResourceId(const std::byte *_begin,
                                const size_t *_size_ptr) noexcept
    :begin(_begin), size_ptr(_size_ptr) {}
#endif

  static constexpr ResourceId Null() noexcept {
#ifdef ANDROID
    return ResourceId(0);
#else
    return ResourceId(nullptr, nullptr);
#endif
  }

  constexpr bool IsDefined() const noexcept {
#ifdef ANDROID
    return name != nullptr;
#else
    return begin != nullptr;
#endif
  }

#ifdef ANDROID
  constexpr explicit operator const char *() const noexcept {
    return name;
  }
#else
  [[gnu::pure]]
  operator std::span<const std::byte>() const noexcept {
    return {begin, *size_ptr};
  }
#endif

#ifndef ANDROID

  constexpr bool operator==(ResourceId other) const noexcept {
    return begin == other.begin;
  }

  constexpr bool operator!=(ResourceId other) const noexcept {
    return begin != other.begin;
  }

#endif // !ANDROID
};
