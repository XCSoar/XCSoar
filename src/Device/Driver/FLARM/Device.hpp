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

#ifndef XCSOAR_FLARM_DEVICE_HPP
#define XCSOAR_FLARM_DEVICE_HPP

#include "BinaryProtocol.hpp"
#include "Util/AllocatedArray.hxx"
#include "Compiler.h"
#include "tchar.h"
#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"

#include <string>

#include <stdint.h>

class Port;
struct Declaration;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;
class NMEAInputLine;

class FlarmDevice: public AbstractDevice
{
  enum class Mode : uint8_t {
    UNKNOWN,
    NMEA,
    TEXT,
    BINARY,
  };

  Port &port;

  Mode mode;

  uint16_t sequence_number;

  /**
   * Settings that were received in PDVSC sentences.
   */
  DeviceSettingsMap<std::string> settings;

public:
  FlarmDevice(Port &_port)
    :port(_port), mode(Mode::UNKNOWN), sequence_number(0) {}

  /**
   * Write a setting to the FLARM.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the FLARM has understood and processed it)
   */
  bool SendSetting(const char *name, const char *value,
                   OperationEnvironment &env);

  /**
   * Request a setting from the FLARM.  The FLARM will send the value,
   * but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the FLARM has understood and processed it)
   */
  bool RequestSetting(const char *name, OperationEnvironment &env);

  /**
   * Look up the given setting in the table of received values.  The
   * first element is a "found" flag, and if that is true, the second
   * element is the value.
   */
  gcc_pure
  std::pair<bool, std::string> GetSetting(const char *name) const;

protected:
  bool TextMode(OperationEnvironment &env);
  bool BinaryMode(OperationEnvironment &env);

  bool ParsePFLAC(NMEAInputLine &line);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment &env) override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;

  bool GetPilot(TCHAR *buffer, size_t length, OperationEnvironment &env);
  bool SetPilot(const TCHAR *pilot_name, OperationEnvironment &env);
  bool GetCoPilot(TCHAR *buffer, size_t length, OperationEnvironment &env);
  bool SetCoPilot(const TCHAR *copilot_name, OperationEnvironment &env);
  bool GetPlaneType(TCHAR *buffer, size_t length, OperationEnvironment &env);
  bool SetPlaneType(const TCHAR *plane_type, OperationEnvironment &env);
  bool GetPlaneRegistration(TCHAR *buffer, size_t length,
                            OperationEnvironment &env);
  bool SetPlaneRegistration(const TCHAR *registration,
                            OperationEnvironment &env);
  bool GetCompetitionId(TCHAR *buffer, size_t length,
                        OperationEnvironment &env);
  bool SetCompetitionId(const TCHAR *competition_id,
                        OperationEnvironment &env);
  bool GetCompetitionClass(TCHAR *buffer, size_t length,
                           OperationEnvironment &env);
  bool SetCompetitionClass(const TCHAR *competition_class,
                           OperationEnvironment &env);

  bool GetStealthMode(bool &enabled, OperationEnvironment &env);
  bool SetStealthMode(bool enabled, OperationEnvironment &env);
  bool GetRange(unsigned &range, OperationEnvironment &env);
  bool SetRange(unsigned range, OperationEnvironment &env);
  bool GetBaudRate(unsigned &baud_id, OperationEnvironment &env);
  bool SetBaudRate(unsigned baud_id, OperationEnvironment &env);

  void Restart(OperationEnvironment &env);

private:
  /**
   * Sends the supplied sentence with a $ prepended and a line break appended
   */
  bool Send(const char *sentence, OperationEnvironment &env);
  bool Receive(const char *prefix, char *buffer, size_t length,
               OperationEnvironment &env, unsigned timeout_ms);

  bool GetConfig(const char *setting, char *buffer, size_t length,
                 OperationEnvironment &env);
  bool SetConfig(const char *setting, const char *value,
                 OperationEnvironment &env);

#ifdef _UNICODE
  bool GetConfig(const char *setting, TCHAR *buffer, size_t length,
                 OperationEnvironment &env);
  bool SetConfig(const char *setting, const TCHAR *value,
                 OperationEnvironment &env);
#endif

  bool DeclareInternal(const Declaration &declaration,
                       OperationEnvironment &env);

  bool SendEscaped(const void *data, size_t length,
                   OperationEnvironment &env, unsigned timeout_ms) {
    return FLARM::SendEscaped(port, data, length, env, timeout_ms);
  }

  bool ReceiveEscaped(void *data, size_t length,
                      OperationEnvironment &env, unsigned timeout_ms) {
    return FLARM::ReceiveEscaped(port, data, length, env, timeout_ms);
  }

  /**
   * Send the byte that is used to signal that start of a new frame
   * @return True if the byte was sent successfully
   */
  bool SendStartByte();

  /**
   * Waits for a certain amount of time until the next frame start signal byte
   * is received
   * @param timeout_ms Timeout in milliseconds
   * @return True if the start byte was received, False if a timeout occurred
   */
  bool WaitForStartByte(OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Convenience function. Returns a pre-populated FrameHeader instance that is
   * ready to be sent by the SendFrameHeader() function.
   * @param message_type Message type of the FrameHeader
   * @param data Optional pointer to the first byte of the payload. Used for
   * CRC calculations.
   * @param length Optional length of the payload
   * @return An initialized FrameHeader instance
   */
  FLARM::FrameHeader PrepareFrameHeader(FLARM::MessageType message_type,
                                        const void *data = nullptr,
                                        size_t length = 0);

  /**
   * Sends a FrameHeader to the port. Remember that a StartByte should be
   * sent first!
   * @param header FrameHeader that should be sent.
   * @param timeout_ms Timeout in milliseconds
   * @return True if the header was sent successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool SendFrameHeader(const FLARM::FrameHeader &header,
                       OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Reads a FrameHeader from the port. This should only be done directly
   * after receiving a StartByte!
   * @param header FrameHeader instance that should be filled
   * @param timeout_ms Timeout in milliseconds
   * @return True if the header was received successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool ReceiveFrameHeader(FLARM::FrameHeader &header,
                          OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Waits for an ACK or NACK message from the FLARM with the right
   * sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @param data An AllocatedArray where the received payload will be stored in
   * @param length The length of the received payload
   * @param timeout_ms Timeout in milliseconds
   * @return Message type if N(ACK) was received properly, otherwise 0x00
   */
  FLARM::MessageType
  WaitForACKOrNACK(uint16_t sequence_number, AllocatedArray<uint8_t> &data,
                   uint16_t &length,
                   OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Waits for an ACK or NACK message from the FLARM with the right
   * sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @param timeout_ms Timeout in milliseconds
   * @return Message type if N(ACK) was received properly, otherwise 0x00
   */
  FLARM::MessageType WaitForACKOrNACK(uint16_t sequence_number,
                                      OperationEnvironment &env,
                                      unsigned timeout_ms);

  /**
   * Waits for an ACK message from the FLARM with the right sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @param timeout_ms Timeout in milliseconds
   * @return True if the ACK message was properly received, False otherwise
   */
  bool WaitForACK(uint16_t sequence_number,
                  OperationEnvironment &env, unsigned timeout_ms);

  /**
   * "Pings" the connected FLARM device in binary mode to see if the transfer
   * mode switched worked.
   * @param timeout_ms Timeout in milliseconds
   * @return True if the FLARM responded properly to the ping, False otherwise
   */
  bool BinaryPing(OperationEnvironment &env, unsigned timeout_ms);

  /**
   * "Resets the device. The only way to resume normal operation."
   * @param timeout_ms Timeout in milliseconds
   * @return True if the message was sent properly, False otherwise
   */
  bool BinaryReset(OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Sends a SelectRecord message to the Flarm
   * @param record_number Number of the record
   * @return ACK if record exists, NACK if record does not exist,
   * ERROR in case of timeout or transfer error
   */
  FLARM::MessageType SelectFlight(uint8_t record_number,
                                  OperationEnvironment &env);

  /**
   * Sends a GetRecordInfo message to the Flarm and parses the output
   * @param flight RecordedFlightInfo instance to parse into
   * @return True if received and parsed successfully, otherwise False
   */
  bool ReadFlightInfo(RecordedFlightInfo &flight, OperationEnvironment &env);

  /**
   * Sends GetIGCData messages to the Flarm and downloads the currently
   * selected flight
   * @param path Path to the IGC file to write into
   * @return True if received and written successfully, otherwise False
   */
  bool DownloadFlight(Path path, OperationEnvironment &env);

public:
  /**
   * Reads a RecordedFlightList from the Flarm
   * @param flight_list RecordedFlightList that should be filled
   * @return True if received and parsed successfully, otherwise False
   */
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;

  /**
   * Downloads a flight from the Flarm into an IGC file
   * @param flight A RecordedFlightInfo instance with internal.flarm set
   * @param path Path to the IGC file to write into
   * @return True if received and written successfully, otherwise False
   */
  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env) override;
};

#endif
