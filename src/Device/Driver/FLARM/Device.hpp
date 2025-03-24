// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BinaryProtocol.hpp"
#include "util/AllocatedArray.hxx"
#include "tchar.h"
#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"

#include <cstdint>
#include <optional>
#include <string>

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

  Mode mode = Mode::UNKNOWN;

  uint16_t sequence_number = 0;

  /**
   * Settings that were received in PDVSC sentences.
   */
  DeviceSettingsMap<std::string> settings;

public:
  FlarmDevice(Port &_port)
    :port(_port) {}

  /**
   * Write a setting to the FLARM.
   */
  void SendSetting(const char *name, const char *value,
                   OperationEnvironment &env);

  /**
   * Request an array of settings from FLARM.
   * 
   * @return true if successful.
   */
  bool RequestAllSettings(const char* const* settings, OperationEnvironment &env);

  /**
   * Request a setting from the FLARM.  The FLARM will send the value,
   * but this method will not wait for that.
   *
   * @return true if sending the command has succeeded (it does not
   * indicate whether the FLARM has understood and processed it)
   */
  void RequestSetting(const char *name, OperationEnvironment &env);

  /**
   * Wait for FLARM to send a setting.
   * @timeout the timeout in milliseconds.
   *
   * @return true if the settings were received, false if a timeout occured.
   */
  bool WaitForSetting(const char *name, unsigned int timeout_ms);

  /**
   * Check if setting exists
   * 
   * @return true if setting exists
   */
  bool SettingExists(const char *name) noexcept;

  /**
   * Look up the given setting in the table of received values.  The
   * first element is a "found" flag, and if that is true, the second
   * element is the value.
   */
  [[gnu::pure]]
  std::optional<std::string> GetSetting(const char *name) const noexcept;

  /**
   * Get unsigned value from setting string.
   */
  unsigned GetUnsignedValue(const char *name, unsigned default_value);

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
  bool PutPilotEvent(OperationEnvironment &env) override;

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
  void Send(const char *sentence, OperationEnvironment &env);
  bool Receive(const char *prefix, char *buffer, size_t length,
               OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout);

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

  void SendEscaped(std::span<const std::byte> src,
                   OperationEnvironment &env,
                   std::chrono::steady_clock::duration timeout) {
    FLARM::SendEscaped(port, src, env, timeout);
  }

  bool ReceiveEscaped(std::span<std::byte> dest,
                      OperationEnvironment &env,
                      std::chrono::steady_clock::duration timeout) {
    return FLARM::ReceiveEscaped(port, dest, env, timeout);
  }

  /**
   * Send the byte that is used to signal that start of a new frame
   */
  void SendStartByte();

  /**
   * Waits for a certain amount of time until the next frame start signal byte
   * is received
   *
   * Throws on error.
   */
  void WaitForStartByte(OperationEnvironment &env,
                        std::chrono::steady_clock::duration timeout);

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
                                        std::span<const std::byte> payload={}) noexcept;

  /**
   * Sends a FrameHeader to the port. Remember that a StartByte should be
   * sent first!
   * @param header FrameHeader that should be sent.
   */
  void SendFrameHeader(const FLARM::FrameHeader &header,
                       OperationEnvironment &env,
                       std::chrono::steady_clock::duration timeout);

  /**
   * Reads a FrameHeader from the port. This should only be done directly
   * after receiving a StartByte!
   * @param header FrameHeader instance that should be filled
   * @return True if the header was received successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool ReceiveFrameHeader(FLARM::FrameHeader &header,
                          OperationEnvironment &env,
                          std::chrono::steady_clock::duration timeout);

  /**
   * Waits for an ACK or NACK message from the FLARM with the right
   * sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @param data An AllocatedArray where the received payload will be stored in
   * @param length The length of the received payload
   * @return Message type if N(ACK) was received properly, otherwise 0x00
   */
  FLARM::MessageType
  WaitForACKOrNACK(uint16_t sequence_number, AllocatedArray<std::byte> &data,
                   uint16_t &length,
                   OperationEnvironment &env,
                   std::chrono::steady_clock::duration timeout);

  /**
   * Waits for an ACK or NACK message from the FLARM with the right
   * sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @return Message type if N(ACK) was received properly, otherwise 0x00
   */
  FLARM::MessageType WaitForACKOrNACK(uint16_t sequence_number,
                                      OperationEnvironment &env,
                                      std::chrono::steady_clock::duration timeout);

  /**
   * Waits for an ACK message from the FLARM with the right sequence number
   * @param sequence_number Sequence Number that is supposed to be received
   * @return True if the ACK message was properly received, False otherwise
   */
  bool WaitForACK(uint16_t sequence_number,
                  OperationEnvironment &env,
                  std::chrono::steady_clock::duration timeout);

  /**
   * "Pings" the connected FLARM device in binary mode to see if the transfer
   * mode switched worked.
   * @return True if the FLARM responded properly to the ping, False otherwise
   */
  bool BinaryPing(OperationEnvironment &env,
                  std::chrono::steady_clock::duration timeout);

  /**
   * "Resets the device. The only way to resume normal operation."
   */
  void BinaryReset(OperationEnvironment &env,
                   std::chrono::steady_clock::duration timeout);

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
