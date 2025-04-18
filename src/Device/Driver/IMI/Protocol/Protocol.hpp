// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
namespace IMI {

/**
 * @brief Connects to the device
 *
 * @param port Device handle
 *
 * @return Operation status
 */
bool
Connect(Port &port, OperationEnvironment &env);

/**
 * @brief Sends task declaration
 *
 * @param port Device handle
 * @param decl Task declaration data
 */
void
DeclarationWrite(Port &port, const Declaration &decl,
                 OperationEnvironment &env);

bool
ReadFlightList(Port &port, RecordedFlightList &flight_list,
               OperationEnvironment &env);

bool
FlightDownload(Port &port, const RecordedFlightInfo &flight,
               Path path, OperationEnvironment &env);

/**
 * @brief Disconnects from the device
 *
 * Throws on error or cancellation.
 *
 * @param port Device handle
 */
void
Disconnect(Port &port, OperationEnvironment &env);

} // namespace IMI
