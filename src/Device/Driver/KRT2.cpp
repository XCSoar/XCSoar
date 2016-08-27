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

#include "Device/Driver/KRT2.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "Thread/Cond.hxx"
#include "Thread/Mutex.hpp"
#include "Util/CharUtil.hpp"
#include "Util/StaticFifoBuffer.hxx"

#include <stdint.h>
#include <stdio.h>

/**
 * KRT2 device class.
 *
 * This class provides the interface to communicate with the KRT2 radio.
 * The driver retransmits messages in case of a failure.
 * See
 * http://bugs.xcsoar.org/raw-attachment/ticket/2727/Remote_control_Interface_V12.pdf
 * for the protocol specification.
 */
class KRT2Device final : public AbstractDevice {
  static constexpr unsigned CMD_TIMEOUT = 250; //!< Command timeout in ms.
  static constexpr unsigned NR_RETRIES = 3; //!< Number of tries to send a command.

  static constexpr char STX = 0x02; //!< Command start character.
  static constexpr char ACK = 0x06; //!< Command acknowledged character.
  static constexpr char NAK = 0x15; //!< Command not acknowledged character.
  static constexpr char NO_RSP = 0; //!< No response received yet.

  static constexpr size_t MAX_NAME_LENGTH = 8; //!< Max. radio station name length.

  //! Port the radio is connected to.
  Port &port;
  //! Expected length of the message just receiving.
  size_t expected_msg_length{};
  //! Buffer which receives the messages send from the radio.
  StaticFifoBuffer<uint8_t, 256u> rx_buf;
  //! Last response received from the radio.
  uint8_t response;
  //! Condition to signal that a response was received from the radio.
  Cond rx_cond;
  //! Mutex to be locked to access response.
  Mutex response_mutex;

public:
  /**
   * Constructor of the radio device class.
   *
   * @param _port Port the radio is connected to.
   */
  KRT2Device(Port &_port);

private:
  /**
   * Sends a message to the radio.
   *
   * @param msg Message to be send to the radio.
   */
  bool Send(const uint8_t *msg, unsigned msg_size, OperationEnvironment &env);
  /**
   * Calculates the length of the message just receiving.
   *
   * @param data Pointer to the first character of the message.
   * @param length Number of characters received.
   * @return Expected message length.
   */
  static size_t ExpectedMsgLength(const uint8_t *data, size_t length);
  /**
   * Calculates the length of the command message just receiving.
   *
   * @param code Command code received after the STX character.
   * @return Expected message length after the code character.
   */
  static size_t ExpectedMsgLengthSTX(uint8_t code);
  /**
   * Gets the displayable station name.
   *
   * @param name Name of the radio station.
   * @return Name of the radio station (printable ASCII, MAX_NAME_LENGTH characters).
   */
  static void GetStationName(char *station_name, const TCHAR *name);
  /**
   * Sends the frequency to the radio.
   *
   * Puts the frequency and the name of the station at the active or
   * passive location on the radio.
   * @param cmd Command char to set the location of the frequency.
   * @param frequency Frequency of the radio station.
   * @param name Name of the radio station.
   * @param env Operation environment.
   * @return true if the frequency is defined.
   */
  bool PutFrequency(char cmd,
                    RadioFrequency frequency,
                    const TCHAR *name,
                    OperationEnvironment &env);

public:
  /**
   * Sets the active frequency on the radio.
   */
  virtual bool PutActiveFrequency(RadioFrequency frequency,
                                  const TCHAR *name,
                                  OperationEnvironment &env) override;
  /**
   * Sets the standby frequency on the radio.
   */
  virtual bool PutStandbyFrequency(RadioFrequency frequency,
                                   const TCHAR *name,
                                   OperationEnvironment &env) override;
  /**
   * Receives and handles data from the radio.
   *
   * The function parses messages send by the radio.
   * Because all control characters (e.g. STX, ACK, NAK, ...)
   * can be part of the payload of the messages, it is important
   * to separate the messages to distinguish control characters
   * from payload characters.
   *
   * If a response to a command is received, the function notifies
   * the sender. This could trigger a retransmission in case of a
   * failure.
   */
  virtual bool DataReceived(const void *data, size_t length,
                            struct NMEAInfo &info) override;
};

KRT2Device::KRT2Device(Port &_port)
 : port(_port)
{
}

bool
KRT2Device::Send(const uint8_t *msg, unsigned msg_size,
                 OperationEnvironment &env)
{
  //! Number of tries to send a message
  unsigned retries = NR_RETRIES;

  assert(msg_size > 0);

  do {
    response_mutex.Lock();
    response = NO_RSP;
    response_mutex.Unlock();
    // Send the message
    if (!port.FullWrite(msg, msg_size, env, CMD_TIMEOUT))
      return false;
    // Wait for the response
    response_mutex.Lock();
    rx_cond.timed_wait(response_mutex, CMD_TIMEOUT);
    auto _response = response;
    response_mutex.Unlock();

    if (_response == ACK)
      // ACK received, finish
      return true;

    // No ACK received, retry
    retries--;
  } while (retries);

  return false;
}

bool
KRT2Device::DataReceived(const void *_data, size_t length,
                         struct NMEAInfo &info)
{
  assert(_data != nullptr);
  assert(length > 0);

  const uint8_t *data = (const uint8_t *)_data;
  const uint8_t *end = data + length;

  do {
    // Append new data to the buffer, as much as fits in there
    auto range = rx_buf.Write();
    if (rx_buf.IsFull()) {
      // Overflow: reset buffer to recover quickly
      rx_buf.Clear();
      expected_msg_length = 0;
      continue;
    }
    size_t nbytes = std::min(range.size, size_t(end - data));
    memcpy(range.data, data, nbytes);
    data += nbytes;
    rx_buf.Append(nbytes);

    for (;;) {
      // Read data from buffer to handle the messages
      range = rx_buf.Read();
      if (range.IsEmpty())
        break;

      if (range.size < expected_msg_length)
        break;

      expected_msg_length = ExpectedMsgLength(range.data, range.size);

      if (range.size >= expected_msg_length) {
        switch (*(const uint8_t *) range.data) {
          case ACK:
          case NAK:
            // Received a response to a normal command (STX)
            response_mutex.Lock();
            response = *(const uint8_t *) range.data;
            // Signal the response to the TX thread
            rx_cond.signal();
            response_mutex.Unlock();
            break;
          default:
            // Received a command from the radio -> ignore it
            break;
        }
        // Message handled -> remove message
        rx_buf.Consume(expected_msg_length);
        expected_msg_length = 0;
        // Received something from the radio -> the connection is alive
        info.alive.Update(info.clock);
      }
    }
  } while (data < end);

  return true;
}

/**
  The expected length of a received message may change,
  when the first character is STX and the second character
  is not received yet.
*/
size_t
KRT2Device::ExpectedMsgLength(const uint8_t *data, size_t length)
{
  size_t expected_length;

  assert(data != nullptr);
  assert(length > 0);

  if (data[0] == STX) {
    if (length > 1) {
      expected_length = 2 + ExpectedMsgLengthSTX(data[1]);
    } else {
      // minimum 2 chars
      expected_length = 2;
    }
  } else
    expected_length = 1;

  return expected_length;
}

size_t
KRT2Device::ExpectedMsgLengthSTX(uint8_t code)
{
  size_t expected_length;

  switch (code) {
  case 'U':
    // Active frequency
  case 'R':
    // Standby frequency
    expected_length = 11;
    break;
  case 'Z':
    // Set frequency
    expected_length = 12;
    break;
  case 'A':
    // Set volume
    expected_length = 4;
    break;
  case 'C':
    // Exchange frequencies
  case '8':
    // Unknown code, received once after power up, STX '8'
  case 'B':
    // Low batt
  case 'D':
    // !Low batt
  case 'E':
    // PLL error
  case 'W':
    // PLL error
  case 'F':
    // !PLL error
  case 'J':
    // RX
  case 'V':
    // !RX
  case 'K':
    // TX
  case 'L':
    // Te
  case 'Y':
    // !TX || !RX
  case 'O':
    // Dual on
  case 'o':
    // Dual off
  case 'M':
    // RX on active frequency on (DUAL^)
  case 'm':
    // RX on active frequency off (DUAL)
    expected_length = 0;
    break;
  default:
    // Received unknown STX code
    expected_length = 0;
    break;
  }

  return expected_length;
}

void
KRT2Device::GetStationName(char *station_name, const TCHAR *name)
{
  size_t s_idx = 0; //!< Source name index
  size_t d_idx = 0; //!< Destination name index
  TCHAR c; //!< Character at source name index

  while ((c = name[s_idx++])) {
    // KRT2 supports printable ASCII only
    if (IsPrintableASCII(c)) {
      station_name[d_idx++] = (char) c;
      if (d_idx == MAX_NAME_LENGTH)
        break;
    }
  }
  // Fill up the rest of the string with spaces
  for (; d_idx < MAX_NAME_LENGTH; d_idx++) {
    station_name[d_idx] = ' ';
  }
}

bool
KRT2Device::PutFrequency(char cmd,
                         RadioFrequency frequency,
                         const TCHAR *name,
                         OperationEnvironment &env)
{
  if (frequency.IsDefined()) {
    struct {
      uint8_t start = STX;
      uint8_t command;
      uint8_t mhz;
      uint8_t khz;
      char station[MAX_NAME_LENGTH];
      uint8_t checksum;
    } msg;

    msg.command = cmd;
    msg.mhz = frequency.GetKiloHertz() / 1000;
    msg.khz = (frequency.GetKiloHertz() % 1000) / 5;
    GetStationName(msg.station, name);
    msg.checksum = msg.mhz ^ msg.khz;

    Send((uint8_t *) &msg, sizeof(msg), env);

    return true;
  }

  return false;
}

bool
KRT2Device::PutActiveFrequency(RadioFrequency frequency,
                               const TCHAR *name,
                               OperationEnvironment &env)
{
  return PutFrequency('U', frequency, name, env);
}

bool
KRT2Device::PutStandbyFrequency(RadioFrequency frequency,
                                const TCHAR *name,
                                OperationEnvironment &env)
{
  return PutFrequency('R', frequency, name, env);
}

static Device *
KRT2CreateOnPort(const DeviceConfig &config, Port &comPort)
{
  Device *dev = new KRT2Device(comPort);

  return dev;
}

const struct DeviceRegister krt2_driver = {
  _T("KRT2"),
  _T("KRT2"),
  DeviceRegister::NO_TIMEOUT
   | DeviceRegister::RAW_GPS_DATA,
  KRT2CreateOnPort,
};
