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
  /**
   * @brief Parser state
   */
  enum TState {
    STATE_NOT_SYNC,                               /**< @brief Synchronization bits not found */
    STATE_COMM_MSG                                /**< @brief Parsing message body */
  };

  TState state;                                  /**< @brief Parser state */
  IMIBYTE buffer[IMICOMM_MAX_MSG_SIZE];       /**< @brief Parsed message buffer */
  unsigned buffer_pos;                         /**< @brief Current position in a message buffer */
  unsigned bytes_left;                         /**< @brief Remaining number of bytes of the message to parse */

  /**
   * Cast the head of the buffer to a TMsg.
   */
  TMsg &GetMessage();
  bool Check(const TMsg *msg, IMIDWORD size);
}
}

IMI::TMsg &
IMI::MessageParser::GetMessage()
{
  return *(TMsg *)(void *)buffer;
}

/**
 * @brief Resets the state of the parser
 */
void
IMI::MessageParser::Reset()
{
  bytes_left = 0;
  buffer_pos = 0;
  state = STATE_NOT_SYNC;
}

/**
 * @brief Verifies received message
 *
 * @param msg Message to check
 * @param size Size of received message
 *
 * @return Verification status
 */
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

/**
 * @brief Parses received message chunk
 *
 * @param buffer Buffer with received data
 * @param size The size of received data
 *
 * @return Received message or 0 if invalid on incomplete.
 */
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
