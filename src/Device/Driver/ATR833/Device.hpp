// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "time/PeriodClock.hpp"
#include "util/StaticFifoBuffer.hxx"

class ATR833Device final : public AbstractDevice {
  Port &port;

  /**
   * Buffer which receives the messages send from the radio.
   */
  StaticFifoBuffer<std::byte, 256u> rx_buf;

public:
  explicit ATR833Device(Port &_port):port(_port) {}

public:
  bool DataReceived(std::span<const std::byte> s,
                    NMEAInfo &info) noexcept override;
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;
  bool EnableNMEA(OperationEnvironment &env) override;
  void OnSysTicker() override;
  void LinkTimeout() override;

private:
  PeriodClock status_clock;

  /**
   * Handles responses from the radio.
   */
  void HandleResponse(const std::byte *data, struct NMEAInfo &info);

  static std::size_t HandleSTX(std::span<const std::byte> src, NMEAInfo &info) noexcept;

  /**
   * Handle a raw message data received on the port.  It may be called
   * repeatedly until all data has been consumed.
   *
   * @return the number of bytes consumed or 0 if the message is
   * incomplete
   */
  static std::size_t HandleMessage(std::span<const std::byte> src, NMEAInfo &info) noexcept;
};
