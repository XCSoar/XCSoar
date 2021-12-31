/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_PORT_HPP
#define XCSOAR_DEVICE_PORT_HPP

#include "State.hpp"
#include "util/Compiler.h"

#include <chrono>
#include <cstddef>
#include <exception>
#include <cstddef>

class OperationEnvironment;
class PortListener;
class DataHandler;
class TimeoutClock;

/**
 * Generic Port thread handler class
 */
class Port {
protected:
  PortListener *const listener;

  DataHandler &handler;

public:
  Port(PortListener *_listener, DataHandler &_handler) noexcept;
  virtual ~Port() noexcept;

  /**
   * Returns the current state of this object.
   */
  gcc_pure
  virtual PortState GetState() const noexcept = 0;

  /**
   * Wait until the connection has been established.
   *
   * @return true on success, false on error or if the operation has
   * been cancelled
   */
  virtual bool WaitConnected(OperationEnvironment &env);

  /**
   * Writes a string to the serial port
   *
   * Throws on error.
   *
   * @param data Pointer to the first character
   * @param length Length of the string
   * @return the number of bytes written
   */
  gcc_nonnull_all
  virtual std::size_t Write(const void *data, std::size_t length) = 0;

  /**
   * Writes a null-terminated string to the serial port
   * @param s The string to write
   * @return the number of bytes written, or 0 on error
   */
  gcc_nonnull_all
  std::size_t Write(const char *s);

  /**
   * Writes a single byte to the serial port
   * @param ch Byte to write
   */
  void Write(char ch) {
    Write(&ch, sizeof(ch));
  }

  /**
   * Write data to the serial port, take care for partial writes.
   *
   * Note that this port's write timeout is still in effect for each
   * individual write operation.
   *
   * Throws on error.
   *
   * @param timeout give up after this duration
   */
  gcc_nonnull_all
  void FullWrite(const void *buffer, std::size_t length,
                 OperationEnvironment &env,
                 std::chrono::steady_clock::duration timeout);

  /**
   * Just like FullWrite(), but write a null-terminated string
   */
  gcc_nonnull_all
  void FullWriteString(const char *s,
                       OperationEnvironment &env,
                       std::chrono::steady_clock::duration timeout);

  /**
   * Wait until all data in the output buffer has been sent.
   *
   * @return false on error
   */
  virtual bool Drain() = 0;

  /**
   * Flushes the serial port buffers
   */
  virtual void Flush() = 0;

  /**
   * Sets the baud rate of the serial port to the given value
   *
   * Throws on error.
   *
   * @param BaudRate The desired baudrate
   */
  virtual void SetBaudrate(unsigned BaudRate) = 0;

  /**
   * Gets the current baud rate of the serial port
   *
   * @return the current baud rate, or 0 on error or if a baud rate is
   * not applicable to this #Port implementation
   */
  virtual unsigned GetBaudrate() const noexcept = 0;

  /**
   * Stops the receive thread
   * @return True on success, False on failure
   */
  virtual bool StopRxThread() = 0;

  /**
   * (Re)Starts the receive thread
   * @return True on success, False on failure
   */
  virtual bool StartRxThread() = 0;

  /**
   * Read a single byte from the serial port
   *
   * Throws on error.
   */
  std::byte ReadByte();

  /**
   * Read data from the serial port
   * @param Buffer Pointer to the buffer
   * @param Size Size of the buffer
   * @return Number of bytes read from the port (0 if no data is
   * available currently)
   */
  gcc_nonnull_all
  virtual std::size_t Read(void *Buffer, std::size_t Size) = 0;

  /**
   * Wait until data becomes available or the timeout expires.
   *
   * Throws on error.
   */
  virtual void WaitRead(std::chrono::steady_clock::duration timeout) = 0;

  /**
   * Force flushing the receive buffers, by trying to read from the
   * port until it times out.
   *
   * The configured read timeout not relevant for this method.
   *
   * Throws on error.
   *
   * @param total_timeout the timeout for each read call
   * @param total_timeout the maximum total duration of this method
   */
  void FullFlush(OperationEnvironment &env,
                 std::chrono::steady_clock::duration timeout,
                 std::chrono::steady_clock::duration total_timeout);

  /**
   * Read data from the serial port, take care for partial reads.
   *
   * Throws on error.
   *
   * @param env an OperationEnvironment that allows canceling the
   * operation
   * @param first_timeout timeout for the first read
   * @param subsequent_timeout timeout for the subsequent reads
   * @param total_timeout timeout for the whole operation
   */
  gcc_nonnull_all
  void FullRead(void *buffer, std::size_t length, OperationEnvironment &env,
                std::chrono::steady_clock::duration first_timeout,
                std::chrono::steady_clock::duration subsequent_timeout,
                std::chrono::steady_clock::duration total_timeout);

  /**
   * Read data from the serial port, take care for partial reads.
   *
   * Throws on error.
   *
   * @param env an OperationEnvironment that allows canceling the
   * operation
   * @param timeout give up after this duration
   * @return true on success
   */
  gcc_nonnull_all
  void FullRead(void *buffer, std::size_t length, OperationEnvironment &env,
                std::chrono::steady_clock::duration timeout);

  /**
   * Wait until data becomes available, the timeout expires or the
   * operation gets cancelled.
   *
   * Throws on error.
   *
   * @param timeout give up after this duration
   * @param env an OperationEnvironment that allows cancelling the
   * operation
   */
  void WaitRead(OperationEnvironment &env,
                std::chrono::steady_clock::duration timeout);

  /**
   * Combination of WaitRead() and Read().
   *
   * Throws on error.
   *
   * @return the number of bytes read (always positive)
   */
  std::size_t WaitAndRead(void *buffer, std::size_t length,
                          OperationEnvironment &env,
                          std::chrono::steady_clock::duration timeout);

  /**
   * Combination of WaitRead() and Read().
   *
   * Throws on error.
   *
   * @return the number of bytes read (always positive)
   */
  std::size_t WaitAndRead(void *buffer, std::size_t length,
                          OperationEnvironment &env, TimeoutClock timeout);

  /**
   * Throws on error.
   */
  gcc_nonnull_all
  void ExpectString(const char *token, OperationEnvironment &env,
                    std::chrono::steady_clock::duration timeout=std::chrono::seconds(2));

  /**
   * Wait until the expected character is received, the timeout expires
   * or the operation gets canceled.
   *
   * Throws on error.
   *
   * @param token The expected character
   * @param env An OperationEnvironment that allows canceling the
   * operation
   * @param timeout give up after this duration
   */
  void WaitForChar(const char token, OperationEnvironment &env,
                   std::chrono::steady_clock::duration timeout);

protected:
  /**
   * Implementations should call this method whenever the return value
   * of GetState() would change.
   */
  void StateChanged() noexcept;

  /**
   * Call PortListener::PortError().
   */
  void Error(const char *msg) noexcept;

  /**
   * Call PortListener::PortError().
   */
  void Error(std::exception_ptr e) noexcept;
};

#endif
