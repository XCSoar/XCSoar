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

#ifndef XCSOAR_DEVICE_DRIVER_VOLKSLOGGER_PROTOCOL_HPP
#define XCSOAR_DEVICE_DRIVER_VOLKSLOGGER_PROTOCOL_HPP

#include <stdint.h>
#include <stddef.h>

class Port;
class OperationEnvironment;

namespace Volkslogger {
  // flow- and format- control
  enum {
    STX = 0x02,
    ETX = 0x03,
    ENQ = 0x05,
    ACK = 0x06,
    DLE = 0x10,
    NAK = 0x15,
    CAN = 0x18
  };

  // Kommandos PC -> Logger
  enum Command {
    /**
     * Information.
     */
    cmd_INF = 0x00,

    /**
     * Read directory.
     */
    cmd_DIR = 0x01,

    /**
     * Read flight with MD4.
     */
    cmd_GFL = 0x02,

    /**
     * Read flight with signature.
     */
    cmd_GFS = 0x03,

    /**
     * Read database.
     */
    cmd_RDB = 0x04,

    /**
     * Write parameter.
     */
    cmd_WPR = 0x05,

    /**
     * Clear flight memory.
     */
    cmd_CFL = 0x06,

    /**
     * Write database.
     */
    cmd_PDB = 0x07,

    /**
     * Calculate and print signature.
     */
    cmd_SIG = 0x08,

    /**
     * Emergency readout.
     */
    cmd_ERO = 0x09,

    /**
     * Restart logger.
     */
    cmd_RST = 0x0c,
  };

  bool Reset(Port &port, OperationEnvironment &env, unsigned n);

  bool Handshake(Port &port, OperationEnvironment &env, unsigned timeout_ms);

  bool Connect(Port &port, OperationEnvironment &env, unsigned timeout_ms);

  bool ConnectAndFlush(Port &port, OperationEnvironment &env,
                       unsigned timeout_ms);

  bool SendCommand(Port &port, OperationEnvironment &env,
                   Command cmd, uint8_t param1=0, uint8_t param2=0);

  static inline bool Reset(Port &port, OperationEnvironment &env) {
    return SendCommand(port, env, cmd_RST, 0, 0) == 0;
  }

  bool SendCommandSwitchBaudRate(Port &port, OperationEnvironment &env,
                                 Command cmd, uint8_t param1,
                                 unsigned baud_rate);

  static inline bool SendCommandSwitchBaudRate(Port &port,
                                               OperationEnvironment &env,
                                               Command cmd,
                                               unsigned baud_rate) {
    return SendCommandSwitchBaudRate(port, env, cmd, 0, baud_rate);
  }

  bool WaitForACK(Port &port, OperationEnvironment &env);

  /**
   * Read data from the Logger
   * @param buffer Pointer to the buffer containing the reply received from the
   *        logger
   * @param max_length Maximum buffer size
   * @param timeout_firstchar_ms Optional parameter. Prolonged timeout to wait
   *        for the first reply. If left out or set to 0(Zero) the standard
   *        timeout as for the other chars will be applied.
   */
  int ReadBulk(Port &port, OperationEnvironment &env,
               void *buffer, size_t max_length,
               unsigned timeout_firstchar_ms=0);

  bool WriteBulk(Port &port, OperationEnvironment &env,
                 const void *buffer, unsigned length);

  /**
   * Send command to Volkslogger and after that wait to read
   * the reply using ReadBulk(). This function uses standard IO
   * baudrate 9600
   * @param cmd Volkslogger command sent to logger
   * @param buffer Pointer to the buffer containing the reply received from the
   *        logger
   * @param max_length Maximum buffer size
   * @param timeout_firstchar_ms Optional parameter. Prolonged timeout to wait
   *        for the first reply. If left out or set to 0(Zero) the standard
   *        timeout as for the other chars will be applied.
   */
  int SendCommandReadBulk(Port &port, OperationEnvironment &env,
                          Command cmd, void *buffer, size_t max_length,
                          unsigned timeout_firstchar_ms=0);

  /**
    * Send command to Volkslogger and after that wait to read
    * the reply using ReadBulk(). This function uses bulk
    * baudrate (baud_rate).
    * @param baud_rate Baudrate to switch to for the data reception
    * @param cmd Volkslogger command sent to logger
    * @param param1 Extension of the Volkslogger command (e.g. log number)
    * @param buffer Pointer to the buffer containing the reply received from the
    *        logger
    * @param max_length Maximum buffer size
    * @param timeout_firstchar_ms Optional parameter. Prolonged timeout to wait
    *        for the first reply. If left out or set to 0(Zero) the standard
    *        timeout as for the other chars will be applied.
    */

  int SendCommandReadBulk(Port &port, unsigned baud_rate,
                          OperationEnvironment &env,
                          Command cmd, uint8_t param1,
                          void *buffer, size_t max_length,
                          unsigned timeout_firstchar_ms=0);

  /**
   * Same Function as the one above. Only without param1.
   */
  static inline int SendCommandReadBulk(Port &port, unsigned baud_rate,
                                        OperationEnvironment &env,
                                        Command cmd,
                                        void *buffer, size_t max_length,
                                        unsigned timeout_firstchar_ms=0)
  {
    return SendCommandReadBulk(port, baud_rate, env, cmd, 0, buffer,
                               max_length, timeout_firstchar_ms);
  }

  bool SendCommandWriteBulk(Port &port, OperationEnvironment &env,
                            Command cmd, const void *data, size_t size);

  static inline int
  ReadInfo(Port &port, OperationEnvironment &env,
           void *buffer, size_t max_length)
  {
    return SendCommandReadBulk(port, env, Volkslogger::cmd_INF,
                               buffer, max_length);
  }

  static inline int
  ReadDatabase(Port &port, unsigned baud_rate, OperationEnvironment &env,
               void *buffer, size_t max_length)
  {
    return SendCommandReadBulk(port, baud_rate, env, Volkslogger::cmd_RDB,
                               buffer, max_length);
  }

  static inline bool
  WriteDatabase(Port &port, OperationEnvironment &env,
                const void *buffer, size_t size)
  {
    return SendCommandWriteBulk(port, env, Volkslogger::cmd_PDB,
                                buffer, size);
  }

  /**
   * read Flightlist(Directory) in binary format from VL to buffer,
   * return the length of the read data in bytes
   * or -1 if a problem was encountered and the data read failed.
   */
  static inline int
  ReadFlightList(Port &port, OperationEnvironment &env,
                 void *buffer, size_t max_length)
  {
    return SendCommandReadBulk(port, env, Volkslogger::cmd_DIR,
                               buffer, max_length);
  }

  /**
   * Read one binary flight log from VL.
   *
   * @param secmode true for DSA-signature (valid for FAI
   * documentation), false for MD-signature only
   * @return the number of bytes written to the buffer or 0 on error
   */
  size_t
  ReadFlight(Port &port, unsigned baud_rate, OperationEnvironment &env,
             unsigned flight_number, bool secmode,
             void *buffer, size_t max_length);

  /**
   * Read all binary flight logs from VL.
   */
  static inline int
  ReadAllFlights(Port &port, unsigned baud_rate, OperationEnvironment &env,
                 void *buffer, size_t max_length)
  {
    return SendCommandReadBulk(port, baud_rate, env, Volkslogger::cmd_ERO,
                               buffer, max_length);
  }
}

#endif
