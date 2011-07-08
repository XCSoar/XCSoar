/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_CHECKCUM_HPP
#define XCSOAR_IMI_CHECKCUM_HPP

#include "Types.hpp"

namespace IMI
{
  /**
   * @brief Calculates IMI CRC value
   *
   * @param message Message for which CRC should be provided
   * @param bytes The size of the message
   *
   * @return IMI CRC value
   */
  IMIWORD CRC16Checksum(const void *message, unsigned bytes);
}

#endif
