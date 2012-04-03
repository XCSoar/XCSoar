/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Util/AllocatedArray.hpp"
#include "OS/ByteOrder.hpp"
#include "Compiler.h"
#include "tchar.h"
#include "Device/Driver.hpp"

#include <stdint.h>

class Port;
struct Declaration;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;

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

public:
  FlarmDevice(Port &_port)
    :port(_port), mode(Mode::UNKNOWN), sequence_number(0) {}

protected:
  bool TextMode(OperationEnvironment &env);
  bool BinaryMode(OperationEnvironment &env);

public:
  void LinkTimeout();
  bool EnableNMEA(OperationEnvironment &env);

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env);

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

  void Restart(OperationEnvironment &env);

private:
  /**
   * Sends the supplied sentence with a $ prepended and a line break appended
   */
  bool Send(const char *sentence, OperationEnvironment &env);
  bool Receive(const char *prefix, char *buffer, size_t length,
               OperationEnvironment &env, unsigned timeout_ms);

  bool GetConfig(const char *setting, TCHAR *buffer, size_t length,
                 OperationEnvironment &env);
  bool SetConfig(const char *setting, const TCHAR *value,
                 OperationEnvironment &env);

  bool DeclareInternal(const Declaration &declaration,
                       OperationEnvironment &env);

  enum MessageType {
    MT_ERROR = 0x00,
    MT_ACK = 0xA0,
    MT_NACK = 0xB7,
    MT_PING = 0x01,
    MT_SETBAUDRATE = 0x02,
    MT_FLASHUPLOAD = 0x10,
    MT_EXIT = 0x12,
    MT_SELECTRECORD = 0x20,
    MT_GETRECORDINFO = 0x21,
    MT_GETIGCDATA = 0x22,
  };

#pragma pack(push, 1) // force 1-byte alignment
  /**
   * The binary transfer mode works with "frames". Each frame consists of a
   * start byte (0x73), an 8-byte frame header and an optional payload. The
   * length of the payload is transfered inside the frame header.
   */
  struct FrameHeader
  {
  private:
    /**
     * Length of the frame header (8) + length of the payload in bytes.
     * Use the Get/Set() functions to interact with this attribute!
     */
    uint16_t length;

  public:
    /**
     * Protocol version. Frames with higher version number than implemented
     * by software shall be discarded.
     */
    uint8_t version;

  private:
    /**
     * Sequence counter. Shall be increased by one for every frame sent.
     * Use the Get/Set() functions to interact with this attribute!
     */
    uint16_t sequence_number;

  public:
    /** Message type */
    uint8_t type;

  private:
    /**
     * CRC over the complete message, except CRC field.
     * Use the Get/Set() functions to interact with this attribute!
     */
    uint16_t crc;

  public:
    uint16_t GetLenght() const {
      return FromLE16(length);
    }

    void SetLength(uint16_t _length) {
      length = ToLE16(_length);
    }

    uint16_t GetSequenceNumber() const {
      return ReadUnalignedLE16(&sequence_number);
    }

    void SetSequenceNumber(uint16_t _sequence_number) {
      WriteUnalignedLE16(&sequence_number, _sequence_number);
    }

    uint16_t GetCRC() const {
      return FromLE16(crc);
    }

    void SetCRC(uint16_t _crc) {
      crc = ToLE16(_crc);
    }
  } gcc_packed;
#pragma pack(pop)

  static_assert(sizeof(FrameHeader) == 8,
                "The FrameHeader struct needs to have a size of 8 bytes");

  /**
   * Sends the specified data stream to the FLARM using the escaping algorithm
   * specified in the reference document.
   * @param data Pointer to the first byte
   * @param length Amount of bytes that should be send. Note that the actual
   * number of bytes can be larger due to the escaping.
   * @param timeout_ms Timeout in milliseconds
   * @return True if the data was sent successfully, False if a timeout
   * or some transfer problems occurred
   */
  bool SendEscaped(const void *data, size_t length,
                   OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Reads a specified number of bytes from the port while applying the
   * escaping algorithm. The algorithm will try to read bytes until the
   * specified number is reached or a timeout occurs.
   * @param data Pointer to the first byte of the writable buffer
   * @param length Length of the buffer that should be filled
   * @param timeout_ms Timeout in milliseconds
   * @return True if the data was received successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool ReceiveEscaped(void *data, size_t length,
                      OperationEnvironment &env, unsigned timeout_ms);

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
   * Calculates the CRC value of the FrameHeader and an optional payload
   * @param header FrameHeader to calculate the CRC for
   * @param data Optional pointer to the first byte of the payload
   * @param length Optional length of the payload
   * @return CRC value
   */
  uint16_t CalculateCRC(const FrameHeader &header, const void *data = NULL,
                        size_t length = 0);

  /**
   * Convenience function. Returns a pre-populated FrameHeader instance that is
   * ready to be sent by the SendFrameHeader() function.
   * @param message_type Message type of the FrameHeader
   * @param data Optional pointer to the first byte of the payload. Used for
   * CRC calculations.
   * @param length Optional length of the payload
   * @return An initialized FrameHeader instance
   */
  FrameHeader PrepareFrameHeader(MessageType message_type,
                                 const void *data = NULL, size_t length = 0);

  /**
   * Sends a FrameHeader to the port. Remember that a StartByte should be
   * sent first!
   * @param header FrameHeader that should be sent.
   * @param timeout_ms Timeout in milliseconds
   * @return True if the header was sent successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool SendFrameHeader(const FrameHeader &header,
                       OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Reads a FrameHeader from the port. This should only be done directly
   * after receiving a StartByte!
   * @param header FrameHeader instance that should be filled
   * @param timeout_ms Timeout in milliseconds
   * @return True if the header was received successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool ReceiveFrameHeader(FrameHeader &header,
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
  MessageType
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
  MessageType WaitForACKOrNACK(uint16_t sequence_number,
                               OperationEnvironment &env, unsigned timeout_ms);

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
  MessageType SelectFlight(uint8_t record_number, OperationEnvironment &env);

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
  bool DownloadFlight(const TCHAR *path, OperationEnvironment &env);

public:
  /**
   * Reads a RecordedFlightList from the Flarm
   * @param flight_list RecordedFlightList that should be filled
   * @return True if received and parsed successfully, otherwise False
   */
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env);

  /**
   * Downloads a flight from the Flarm into an IGC file
   * @param flight A RecordedFlightInfo instance with internal.flarm set
   * @param path Path to the IGC file to write into
   * @return True if received and written successfully, otherwise False
   */
  bool DownloadFlight(const RecordedFlightInfo &flight, const TCHAR *path,
                      OperationEnvironment &env);
};

#endif
