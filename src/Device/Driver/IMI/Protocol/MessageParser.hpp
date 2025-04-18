// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

#include <optional>

namespace IMI
{

class MessageParser {
  /** Parser state */
  enum TState {
    /** Synchronization bits not found */
    STATE_NOT_SYNC,
    /** Parsing message body */
    STATE_COMM_MSG
  } state;

  static constexpr std::size_t IMICOMM_MAX_MSG_SIZE = sizeof(TMsg);

  union {
  /** Parsed message buffer */
    IMIBYTE buffer[IMICOMM_MAX_MSG_SIZE];
    TMsg tmsg;
  };

  /** Current position in a message buffer */
  unsigned buffer_pos;
  /** Remaining number of bytes of the message to parse */
  unsigned bytes_left;

public:
  MessageParser() noexcept {
    Reset();
  }

  /**
   * @brief Resets the state of the parser
   */
  void Reset() noexcept {
    bytes_left = 0;
    buffer_pos = 0;
    state = STATE_NOT_SYNC;
  }

  /**
   * @brief Parses received message chunk
   *
   * @param buffer Buffer with received data
   * @param size The size of received data
   *
   * @return Received message or 0 if invalid on incomplete.
   */
  std::optional<TMsg> Parse(const IMIBYTE buffer[], int size) noexcept;

private:
  /**
   * Cast the head of the buffer to a TMsg.
   */
  TMsg &GetMessage() noexcept {
    return tmsg;
  }

  /**
   * @brief Verifies received message
   *
   * @param msg Message to check
   * @param size Size of received message
   *
   * @return Verification status
   */
  bool Check(const TMsg *msg, IMIDWORD size) noexcept;
};

} // namespace IMI
