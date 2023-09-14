// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

/**
 * The identifier for a resource to be passed to
 * ResourceLoader::Load() or other resource-loading functions.
 */
class ResourceId {
#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
  unsigned id;
#else
  const std::byte *begin;
  const size_t *size_ptr;
#endif

public:
  ResourceId() = default;

#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
  constexpr explicit ResourceId(unsigned _id) noexcept
    :id(_id) {}
#else
  constexpr explicit ResourceId(const std::byte *_begin,
                                const size_t *_size_ptr) noexcept
    :begin(_begin), size_ptr(_size_ptr) {}
#endif

  static constexpr ResourceId Null() noexcept {
#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
    return ResourceId(0);
#else
    return ResourceId(nullptr, nullptr);
#endif
  }

  constexpr bool IsDefined() const noexcept {
#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
    return id != 0;
#else
    return begin != nullptr;
#endif
  }

#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
  constexpr explicit operator unsigned() const noexcept {
    return id;
  }
#else
  [[gnu::pure]]
  operator std::span<const std::byte>() const noexcept {
    return {begin, *size_ptr};
  }
#endif

  constexpr bool operator==(ResourceId other) const noexcept {
#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
    return id == other.id;
#else
    return begin == other.begin;
#endif
  }

  constexpr bool operator!=(ResourceId other) const noexcept {
#if defined(USE_WIN32_RESOURCES) || defined(ANDROID)
    return id != other.id;
#else
    return begin != other.begin;
#endif
  }
};
