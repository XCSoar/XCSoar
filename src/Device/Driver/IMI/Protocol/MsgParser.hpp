/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_MSGPARSER_HPP
#define XCSOAR_IMI_MSGPARSER_HPP

#include "Types.hpp"

#define IMICOMM_MAX_MSG_SIZE (sizeof(TMsg))

namespace IMI
{
  /**
   * @brief Message parser class
   */
  class CMsgParser {
    /**
     * @brief Parser state
     */
    enum TState {
      STATE_NOT_SYNC,                               /**< @brief Synchronization bits not found */
      STATE_COMM_MSG                                /**< @brief Parsing message body */
    };

    TState _state;                                  /**< @brief Parser state */
    IMIBYTE _msgBuffer[IMICOMM_MAX_MSG_SIZE];       /**< @brief Parsed message buffer */
    unsigned _msgBufferPos;                         /**< @brief Current position in a message buffer */
    unsigned _msgBytesLeft;                         /**< @brief Remaining number of bytes of the message to parse */

    /**
     * Cast the head of the buffer to a TMsg.
     */
    TMsg &GetMessage() {
      return *(TMsg *)(void *)_msgBuffer;
    }

    bool Check(const TMsg *msg, IMIDWORD size) const;

  public:
    void Reset();
    const TMsg *Parse(const IMIBYTE buffer[], int size);
  };
}

#endif
