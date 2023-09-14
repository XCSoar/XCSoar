// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

class MD5
{
public:
  static constexpr size_t DIGEST_LENGTH = 32;

  struct State {
    uint32_t a, b, c, d;
  };

private:
  std::array<uint8_t, 64> buff512bits;
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

  void Append(uint8_t ch) noexcept;
  void Append(const void *data, size_t length) noexcept;

  void Finalize() noexcept;

  /**
   * @param buffer a buffer of at least #DIGEST_LENGTH+1 bytes
   * @return a pointer to the null terminator
   */
  char *GetDigest(char *buffer) const noexcept;
};
