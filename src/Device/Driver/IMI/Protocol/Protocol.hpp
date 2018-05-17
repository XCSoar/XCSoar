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

#ifndef XCSOAR_IMI_DEVIMI_HPP
#define XCSOAR_IMI_DEVIMI_HPP

class Path;
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
  bool DeclarationWrite(Port &port, const Declaration &decl,
                        OperationEnvironment &env);

  bool ReadFlightList(Port &port, RecordedFlightList &flight_list,
                      OperationEnvironment &env);

  bool FlightDownload(Port &port, const RecordedFlightInfo &flight,
                      Path path, OperationEnvironment &env);
  /**
   * @brief Disconnects from the device
   *
   * @param port Device handle
   *
   * @return Operation status
   */
  bool Disconnect(Port &port, OperationEnvironment &env);
};

#endif
