/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "MsgParser.hpp"
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

  /** Parsed message buffer */
  IMIBYTE buffer[IMICOMM_MAX_MSG_SIZE];
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
  return *(TMsg *)buffer;
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
  IMIWORD crc1 = CRC16Checksum(((IMIBYTE*)msg) + IMICOMM_SYNC_LEN,
                               IMICOMM_MSG_HEADER_SIZE + msg->payloadSize -
                               IMICOMM_SYNC_LEN);

  IMIWORD crc2 = (IMIWORD)(((IMIBYTE*)msg)[size - 1]) |
                 ((IMIWORD)(((IMIBYTE*)msg)[size - 2]) << 8);

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
