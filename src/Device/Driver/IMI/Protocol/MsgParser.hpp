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

namespace IMI
{
  namespace MessageParser {
    /**
     * @brief Resets the state of the parser
     */
    void Reset();

    /**
     * @brief Parses received message chunk
     *
     * @param buffer Buffer with received data
     * @param size The size of received data
     *
     * @return Received message or 0 if invalid on incomplete.
     */
    const TMsg *Parse(const IMIBYTE buffer[], int size);
  };
}

#endif
