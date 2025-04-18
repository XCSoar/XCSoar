// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <span>

class MD5
{
public:
  static constexpr size_t DIGEST_LENGTH = 32;

  struct State {
    uint32_t a, b, c, d;
  };

private:
  alignas(uint64_t)
  std::array<std::byte, 64> buff512bits;
  State state;
  uint64_t message_length;

  void Process512() noexcept;

public:
  /**
   * Initialise with a custom key.
   */
  void Initialise(const State &_state) noexcept {
    state = _state;
    message_length = 0;
  }

  /**
   * Initialise with the default key.
   */
  void Initialise() noexcept;

  void Append(std::byte ch) noexcept;
  void Append(std::span<const std::byte> src) noexcept;

  void Finalize() noexcept;

  /**
   * @param buffer a buffer of at least #DIGEST_LENGTH+1 bytes
   * @return a pointer to the null terminator
   */
  char *GetDigest(char *buffer) const noexcept;
};
