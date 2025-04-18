// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessageParser.hpp"
#include "Checksum.hpp"

bool
IMI::MessageParser::Check(const TMsg *msg, IMIDWORD size) noexcept
{
  // minimal size of comm message
  if (size < IMICOMM_MSG_HEADER_SIZE + IMICOMM_CRC_LEN)
    return false;

  // check signature
  if(msg->syncChar1 != IMICOMM_SYNC_CHAR1 ||
     msg->syncChar2 != IMICOMM_SYNC_CHAR2)
    return false;

  // check size
  if(msg->payloadSize != size - IMICOMM_MSG_HEADER_SIZE - IMICOMM_CRC_LEN)
    return false;

  // check CRC
  IMIWORD crc1 = CRC16Checksum(((const IMIBYTE *)msg) + IMICOMM_SYNC_LEN,
                               IMICOMM_MSG_HEADER_SIZE + msg->payloadSize -
                               IMICOMM_SYNC_LEN);

  IMIWORD crc2 = (IMIWORD)(((const IMIBYTE *)msg)[size - 1]) |
                 ((IMIWORD)(((const IMIBYTE *)msg)[size - 2]) << 8);

  return crc1 == crc2;
}

std::optional<IMI::TMsg>
IMI::MessageParser::Parse(const IMIBYTE _buffer[], int size) noexcept
{
  const IMIBYTE *ptr = _buffer;
  std::optional<TMsg> msg;

  for (; size; size--) {
    IMIBYTE byte = *ptr++;

    if (state == STATE_NOT_SYNC) {
      // verify synchronization chars
      if (byte == IMICOMM_SYNC_CHAR1 && buffer_pos == 0) {
        buffer[buffer_pos++] = byte;
      } else if (byte == IMICOMM_SYNC_CHAR2 && buffer_pos == 1) {
        buffer[buffer_pos++] = byte;
        state = STATE_COMM_MSG;
      } else {
        buffer_pos = 0;
      }
    } else if (state == STATE_COMM_MSG) {
      if (buffer_pos < IMICOMM_MSG_HEADER_SIZE) {
        // copy header
        buffer[buffer_pos++] = byte;
      } else {
        if (buffer_pos == IMICOMM_MSG_HEADER_SIZE) {
          // verify payload size
          bytes_left = GetMessage().payloadSize + IMICOMM_CRC_LEN;
          if (bytes_left > COMM_MAX_PAYLOAD_SIZE + IMICOMM_CRC_LEN) {
            // Invalid length
            Reset();
            continue;
          }
        }

        // copy payload
        bytes_left--;
        if (buffer_pos < sizeof(buffer)) // Just in case
          buffer[buffer_pos++] = byte;

        if (bytes_left == 0) {
          // end of message
          if (Check(&GetMessage(), buffer_pos))
            msg = GetMessage();

          // prepare parser for the next message
          Reset();
        }
      }
    }
  }

  return msg;
}
