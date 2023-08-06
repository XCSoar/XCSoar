// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/KRT2.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "thread/Cond.hxx"
#include "thread/Mutex.hxx"
#include "util/CharUtil.hxx"
#include "util/StaticFifoBuffer.hxx"
#include "util/Compiler.h"

#include <cstdint>

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
  static constexpr auto CMD_TIMEOUT = std::chrono::milliseconds(250); //!< Command timeout
  static constexpr unsigned NR_RETRIES = 3; //!< Number of tries to send a command.

  static constexpr std::byte STX{0x02}; //!< Command start character.
  static constexpr std::byte ACK{0x06}; //!< Command acknowledged character.
  static constexpr std::byte NAK{0x15}; //!< Command not acknowledged character.
  static constexpr std::byte NO_RSP{0}; //!< No response received yet.
  static constexpr std::byte RCQ{'S'};  //!< Respond to connection query

  /** Unknown code, received once after power up, STX '8' */
  static constexpr std::byte UNKNOWN1{'8'};
  static constexpr std::byte SET_VOLUME{'A'};
  static constexpr std::byte LOW_BATTERY{'B'};
  static constexpr std::byte EXCHANGE_FREQUENCIES{'C'};
  static constexpr std::byte NO_LOW_BATTERY{'D'};
  static constexpr std::byte PLL_ERROR{'E'};
  static constexpr std::byte NO_PLL_ERROR{'F'};
  static constexpr std::byte RX{'J'};
  static constexpr std::byte TX{'K'};
  static constexpr std::byte TE{'L'};
  static constexpr std::byte RX_ON_ACTIVE_FREQUENCY{'M'};
  static constexpr std::byte DUAL_ON{'O'};
  static constexpr std::byte STANDBY_FREQUENCY{'R'};
  static constexpr std::byte ACTIVE_FREQUENCY{'U'};
  static constexpr std::byte NO_RX{'V'};
  static constexpr std::byte PLL_ERROR2{'W'};
  static constexpr std::byte NO_TX_RX{'Y'};
  static constexpr std::byte SET_FREQUENCY{'Z'};
  static constexpr std::byte DUAL_OFF{'o'};
  static constexpr std::byte NO_RX_ON_ACTIVE_FREQUENCY{'m'};

  static constexpr size_t MAX_NAME_LENGTH = 8; //!< Max. radio station name length.

  struct stx_msg {
    std::byte start = STX;
    std::byte command;
    uint8_t mhz;
    uint8_t khz;
    char station[MAX_NAME_LENGTH];
    uint8_t checksum;
  };

  //! Port the radio is connected to.
  Port &port;
  //! Buffer which receives the messages send from the radio.
  StaticFifoBuffer<std::byte, 256u> rx_buf;
  //! Last response received from the radio.
  std::byte response;
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
  [[nodiscard]]
  bool Send(std::span<const std::byte> msg, OperationEnvironment &env);

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
  bool PutFrequency(std::byte cmd,
                    RadioFrequency frequency,
                    const TCHAR *name,
                    OperationEnvironment &env);

  void LockSetResponse(std::byte _response) noexcept {
    const std::lock_guard lock{response_mutex};
    response = _response;
    // Signal the response to the TX thread
    rx_cond.notify_one();
  }

  std::byte LockWaitResponse() noexcept {
    std::unique_lock lock{response_mutex};
    rx_cond.wait_for(lock, CMD_TIMEOUT, [this]{ return response != NO_RSP; });
    return response;
  }

  /**
   * Handle an STX command from the radio.
   *
   * Handles STX commands from the radio, when these indicate a change in either
   * active of passive frequency.
   */
  static void HandleFrequency(const struct stx_msg &msg, NMEAInfo &info) noexcept;

  static std::size_t HandleSTX(std::span<const std::byte> src, NMEAInfo &info) noexcept;

  /**
   * Handle a raw message data received on the port.  It may be called
   * repeatedly until all data has been consumed.
   *
   * @return the number of bytes consumed or 0 if the message is
   * incomplete
   */
  std::size_t HandleMessage(std::span<const std::byte> src, NMEAInfo &info) noexcept;

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
  virtual bool DataReceived(std::span<const std::byte> s,
                            NMEAInfo &info) noexcept override;
};

KRT2Device::KRT2Device(Port &_port)
 : port(_port)
{
}

bool
KRT2Device::Send(std::span<const std::byte> msg,
                 OperationEnvironment &env)
{
  assert(!msg.empty());

  //! Number of tries to send a message
  unsigned retries = NR_RETRIES;

  do {
    {
      const std::lock_guard lock{response_mutex};
      response = NO_RSP;
    }

    // Send the message
    port.FullWrite(msg, env, CMD_TIMEOUT);

    // Wait for the response
    if (LockWaitResponse() == ACK)
      // ACK received, finish
      return true;

    // No ACK received, retry
    retries--;
  } while (retries);

  return false;
}

bool
KRT2Device::DataReceived(std::span<const std::byte> s,
                         NMEAInfo &info) noexcept
{
  assert(!s.empty());

  do {
    // Append new data to the buffer, as much as fits in there
    const auto nbytes = rx_buf.MoveFrom(s);
    if (nbytes == 0) {
      // Overflow: reset buffer to recover quickly
      rx_buf.Clear();
      continue;
    }

    s = s.subspan(nbytes);

    for (;;) {
      // Read data from buffer to handle the messages
      const auto consumed = HandleMessage(rx_buf.Read(), info);
      if (consumed == 0)
        // need more data
        break;

      // Message handled -> remove message
      rx_buf.Consume(consumed);
      // Received something from the radio -> the connection is alive
      info.alive.Update(info.clock);
    }
  } while (!s.empty());

  return true;
}

inline void
KRT2Device::GetStationName(char *station_name, const TCHAR *name)
{
  if(name == nullptr)
      name = _T("");

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

inline void
KRT2Device::HandleFrequency(const struct stx_msg &msg, NMEAInfo &info) noexcept
{
  if (msg.checksum != (msg.mhz ^ msg.khz)) {
    return;
  }

  const auto freq = RadioFrequency::FromMegaKiloHertz(msg.mhz, msg.khz * 5);

  StaticString<MAX_NAME_LENGTH> freq_name;
  freq_name.SetASCII(msg.station);

  if (msg.command == ACTIVE_FREQUENCY) {
    info.settings.has_active_frequency.Update(info.clock);
    info.settings.active_frequency = freq;
    info.settings.active_freq_name = freq_name;
  } else if (msg.command == STANDBY_FREQUENCY) {
    info.settings.has_standby_frequency.Update(info.clock);
    info.settings.standby_frequency = freq;
    info.settings.standby_freq_name = freq_name;
  }
}

inline std::size_t
KRT2Device::HandleSTX(std::span<const std::byte> src, NMEAInfo &info) noexcept
{
  if (src.size() < 2)
    return 0;

  switch (src[1]) {
  case ACTIVE_FREQUENCY:
  case STANDBY_FREQUENCY:
    if (src.size() < sizeof(struct stx_msg))
      return 0;

    HandleFrequency(*(const struct stx_msg *)src.data(), info);
    return sizeof(struct stx_msg);

  case SET_FREQUENCY:
    return src.size() < 14 ? 0 : 14;

  case SET_VOLUME:
    return src.size() < 6 ? 0 : 6;

  case EXCHANGE_FREQUENCIES:
    info.settings.swap_frequencies.Update(info.clock);
    return 2;

  case UNKNOWN1:
  case LOW_BATTERY:
  case NO_LOW_BATTERY:
  case PLL_ERROR:
  case PLL_ERROR2:
  case NO_PLL_ERROR:
  case RX:
  case NO_RX:
  case TX:
  case TE:
  case NO_TX_RX:
  case DUAL_ON:
  case DUAL_OFF:
  case RX_ON_ACTIVE_FREQUENCY:
  case NO_RX_ON_ACTIVE_FREQUENCY:
    return 2;

  default:
    // Received unknown STX code
    return 2;
  }
}

inline std::size_t
KRT2Device::HandleMessage(std::span<const std::byte> src,
                          NMEAInfo &info) noexcept
{
  if (src.empty())
    return 0;

  switch (src.front()) {
  case RCQ:
    // Respond to connection query.
    port.Write(0x01);
    return 1;

  case ACK:
  case NAK:
    // Received a response to a normal command (STX)
    LockSetResponse(src.front());
    return 1;

  case STX:
    // Received a command from the radio (STX). Handle what we know.
    return HandleSTX(src, info);

  default:
    return 1;
  }
}

bool
KRT2Device::PutFrequency(std::byte cmd,
                         RadioFrequency frequency,
                         const TCHAR *name,
                         OperationEnvironment &env)
{
  stx_msg msg;

  msg.command = cmd;
  msg.mhz = frequency.GetKiloHertz() / 1000;
  msg.khz = (frequency.GetKiloHertz() % 1000) / 5;
  GetStationName(msg.station, name);
  msg.checksum = msg.mhz ^ msg.khz;

  return Send(std::as_bytes(std::span{&msg, 1}), env);
}

bool
KRT2Device::PutActiveFrequency(RadioFrequency frequency,
                               const TCHAR *name,
                               OperationEnvironment &env)
{
  return PutFrequency(ACTIVE_FREQUENCY, frequency, name, env);
}

bool
KRT2Device::PutStandbyFrequency(RadioFrequency frequency,
                                const TCHAR *name,
                                OperationEnvironment &env)
{
  return PutFrequency(STANDBY_FREQUENCY, frequency, name, env);
}

static Device *
KRT2CreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &comPort)
{
  return new KRT2Device(comPort);
}

const DeviceRegister krt2_driver = {
  _T("KRT2"),
  _T("KRT2"),
  DeviceRegister::NO_TIMEOUT
   | DeviceRegister::RAW_GPS_DATA,
  KRT2CreateOnPort,
};
