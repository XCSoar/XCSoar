// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Error.hxx"

#include <cstddef>
#include <span>
#include <utility>

#include <wtypesbase.h> // for LPSECURITY_ATTRIBUTES, needed by winreg.h
#include <winreg.h>
#include <tchar.h>

/**
 * OO wrapper for a HKEY.
 */
class RegistryKey {
  HKEY h{};

public:
  RegistryKey() noexcept = default;

  RegistryKey(HKEY parent, const char *key) {
    const auto result = RegOpenKeyEx(parent, key, 0, KEY_READ, &h);
    if (result != ERROR_SUCCESS)
      throw MakeLastError("RegOpenKeyEx() failed");
  }

  ~RegistryKey() noexcept {
    if (h != HKEY{})
      RegCloseKey(h);
  }

  RegistryKey(RegistryKey &&src) noexcept
    :h(std::exchange(src.h, HKEY{})) {}

  RegistryKey &operator=(RegistryKey &&src) noexcept {
    using std::swap;
    swap(h, src.h);
    return *this;
  }

  operator HKEY() const noexcept {
    return h;
  }

  bool GetValue(const char *name, LPDWORD type_r,
                LPBYTE data, LPDWORD length_r) const noexcept {
    return RegQueryValueEx(h, name, nullptr, type_r,
                           data, length_r) == ERROR_SUCCESS;
  }

  /**
   * Reads a string value.  When this function fails, the value in the
   * buffer is undefined (may have been modified by this method).
   *
   * @return true on success
   */
  bool GetValue(const char *name, std::span<char> value) const noexcept {
    const auto s = std::as_writable_bytes(value);
    DWORD type, length = s.size();
    return GetValue(name, &type, (LPBYTE)s.data(), &length) && type == REG_SZ;
  }

  bool EnumKey(DWORD idx, std::span<char> name) const noexcept {
    DWORD name_max_size = (DWORD)name.size();
    return RegEnumKeyEx(h, idx, name.data(), &name_max_size,
                        nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
  }

  bool EnumValue(DWORD idx, std::span<char> name, LPDWORD type,
                 std::span<std::byte> value) const noexcept {
    DWORD name_max_size = (DWORD)name.size();
    DWORD value_max_size = (DWORD)name.size();

    return RegEnumValue(h, idx, name.data(), &name_max_size,
                        nullptr, type,
                        (LPBYTE)value.data(), &value_max_size) == ERROR_SUCCESS;
  }
};
