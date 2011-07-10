/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_COMMUNICATION_HPP
#define XCSOAR_IMI_COMMUNICATION_HPP

#include "Types.hpp"

class Port;

namespace IMI
{
  /**
   * @brief Sends message buffer to a device
   *
   * @param port Device handle
   * @param msg IMI message to send
   *
   * @return Operation status
   */
  bool Send(Port &port, const TMsg &msg);

  /**
   * @brief Prepares and sends the message to a device
   *
   * @param port Device handle
   * @param msgID ID of the message to send
   * @param payload Payload buffer to use for the message
   * @param payloadSize The size of the payload buffer
   * @param parameter1 1st parameter for to put in the message
   * @param parameter2 2nd parameter for to put in the message
   * @param parameter3 3rd parameter for to put in the message
   *
   * @return Operation status
   */
  bool Send(Port &port, IMIBYTE msgID, const void *payload = 0,
            IMIWORD payloadSize = 0, IMIBYTE parameter1 = 0,
            IMIWORD parameter2 = 0, IMIWORD parameter3 = 0);
  /**
   * @brief Receives a message from the device
   *
   * @param port Device handle
   * @param extraTimeout Additional timeout to wait for the message
   * @param expectedPayloadSize Expected size of the message
   *
   * @return Pointer to a message structure if expected message was received or 0 otherwise
   */
  const TMsg *Receive(Port &port, unsigned extraTimeout,
                      unsigned expectedPayloadSize);
  /**
   * @brief Sends a message and waits for a confirmation from the device
   *
   * @param port Device handle
   * @param msgID ID of the message to send
   * @param payload Payload buffer to use for the message
   * @param payloadSize The size of the payload buffer
   * @param reMsgID Expected ID of the message to receive
   * @param retPayloadSize Expected size of the received message
   * @param parameter1 1st parameter for to put in the message
   * @param parameter2 2nd parameter for to put in the message
   * @param parameter3 3rd parameter for to put in the message
   * @param extraTimeout Additional timeout to wait for the message
   * @param retry Number of send retries
   *
   * @return Pointer to a message structure if expected message was received or 0 otherwise
   */
  const TMsg *SendRet(Port &port, IMIBYTE msgID, const void *payload,
                      IMIWORD payloadSize, IMIBYTE reMsgID,
                      IMIWORD retPayloadSize, IMIBYTE parameter1 = 0,
                      IMIWORD parameter2 = 0, IMIWORD parameter3 = 0,
                      unsigned extraTimeout = 300, int retry = 4);

  bool FlashRead(Port &port, void *buffer, unsigned address, unsigned size);
}

#endif
