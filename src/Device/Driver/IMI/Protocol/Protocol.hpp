/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_DEVIMI_HPP
#define XCSOAR_IMI_DEVIMI_HPP

#include "Types.hpp"

#include <tchar.h>

class Port;
struct Declaration;
class RecordedFlightList;
struct RecordedFlightInfo;
class OperationEnvironment;

/**
 * @brief IMI-Gliding ERIXX device class
 *
 * Class provides support for IMI-Gliding ERIXX IGC certifed logger.
 *
 * @note IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */
namespace IMI
{
  /**
   * @brief Connects to the device
   *
   * @param port Device handle
   *
   * @return Operation status
   */
  bool Connect(Port &port, OperationEnvironment &env);

  /**
   * @brief Sends task declaration
   *
   * @param port Device handle
   * @param decl Task declaration data
   *
   * @return Operation status
   */
  bool DeclarationWrite(Port &port, const Declaration &decl);

  bool ReadFlightList(Port &port, RecordedFlightList &flight_list);
  bool FlightDownload(Port &port, const RecordedFlightInfo &flight,
                      const TCHAR *path, OperationEnvironment &env);
  /**
   * @brief Disconnects from the device
   *
   * @param port Device handle
   *
   * @return Operation status
   */
  bool Disconnect(Port &port);
};

#endif
