/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "MessageParser.hpp"
#include "Checksum.hpp"

#define IMICOMM_MAX_MSG_SIZE (sizeof(TMsg))

namespace IMI {
namespace MessageParser {
  /** Parser state */
  enum TState {
    /** Synchronization bits not found */
    STATE_NOT_SYNC,
    /** Parsing message body */
    STATE_COMM_MSG
  } state;

  static union {
  /** Parsed message buffer */
    IMIBYTE buffer[IMICOMM_MAX_MSG_SIZE];
    TMsg tmsg;
  };

  /** Current position in a message buffer */
  unsigned buffer_pos;
  /** Remaining number of bytes of the message to parse */
  unsigned bytes_left;

  /**
   * Cast the head of the buffer to a TMsg.
   */
  TMsg &GetMessage();

  /**
   * @brief Verifies received message
   *
   * @param msg Message to check
   * @param size Size of received message
   *
   * @return Verification status
   */
  bool Check(const TMsg *msg, IMIDWORD size);
}
}

IMI::TMsg &
IMI::MessageParser::GetMessage()
{
  return tmsg;
}

void
IMI::MessageParser::Reset()
{
  bytes_left = 0;
  buffer_pos = 0;
  state = STATE_NOT_SYNC;
}

bool
IMI::MessageParser::Check(const TMsg *msg, IMIDWORD size)
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

const IMI::TMsg *
IMI::MessageParser::Parse(const IMIBYTE _buffer[], int size)
{
  const IMIBYTE *ptr = _buffer;
  const TMsg *msg = 0;

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
            msg = &GetMessage();

          // prepare parser for the next message
          Reset();
        }
      }
    }
  }

  return msg;
}
