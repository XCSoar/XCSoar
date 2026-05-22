// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

/**
 * Stable Wi-Fi failure reasons carried by Wi-Fi backends and translated by
 * #Format at the UI boundary.
 */
namespace WifiError {
enum class Code : std::uint8_t {
  Gone,
  NeedKey,
  NoInterface,
  NoDbusConnection,
  ConnmanUnavailable,
  NoBackendAvailable,
  NetworkManagerUnavailable,
};

class Exception final : public std::runtime_error {
  Code code;

public:
  explicit Exception(Code _code);

  [[gnu::pure]]
  Code GetCode() const noexcept {
    return code;
  }
};

std::string
Format(const std::exception &e);

std::string
Format(std::exception_ptr e);
} // namespace WifiError
