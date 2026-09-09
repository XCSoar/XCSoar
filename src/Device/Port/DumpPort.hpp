// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Port.hpp"

#include <chrono>
#include <memory>

/**
 * A port wrapper that dumps everything into the log file.
 */
class DumpPort final : public Port {
  std::unique_ptr<Port> port;

  /**
   * Dumping is enabled until this time stamp.
   */
  std::chrono::steady_clock::time_point until =
    std::chrono::steady_clock::time_point::max();

  /**
   * An optional handler which receives a copy of all data written to
   * the port; see SetWriteMonitor().
   */
  DataHandler *write_monitor = nullptr;

public:
  /**
   * Initialises the new instance.  Dumping is enabled forever by
   * default.
   */
  DumpPort(std::unique_ptr<Port> port) noexcept;

  /**
   * Disable dumping immediately.
   */
  void Disable() noexcept {
    until = std::chrono::steady_clock::time_point{};
  }

  /**
   * Enable dumping forever.
   */
  void EnableForever() noexcept {
    until = std::chrono::steady_clock::time_point::max();
  }

  /**
   * Enable dumping for a certain duration.
   */
  void EnableTemporarily(std::chrono::steady_clock::duration duration) noexcept {
    until = std::chrono::steady_clock::now() + duration;
  }

  bool IsEnabled() const noexcept {
    return until > std::chrono::steady_clock::time_point{};
  }

  /**
   * Install a handler which receives a copy of all data written to
   * the port, independently of the log file dump; used by the port
   * monitor dialog to display outgoing data.  Pass nullptr to
   * remove the handler.
   */
  void SetWriteMonitor(DataHandler *monitor) noexcept {
    write_monitor = monitor;
  }

  Port &GetInnerPort() noexcept {
    return *port;
  }

  const Port &GetInnerPort() const noexcept {
    return *port;
  }

  /**
   * Unwrap this DumpPort (and any inner wrappers) so a driver can
   * downcast to a concrete port type.  Do not pass this to
   * CreateOnPort; that skips Debug I/O logging (#2903).
   */
  Port &GetImplementationPort() noexcept override {
    return port->GetImplementationPort();
  }

private:
  /**
   * Determine whether dumping is currently enabled.
   */
  bool CheckEnabled() noexcept;

public:
  /* virtual methods from Port */
  PortState GetState() const noexcept override;
  bool WaitConnected(OperationEnvironment &env) override;
  std::size_t Write(std::span<const std::byte> src) override;
  bool Drain() override;
  void Flush() override;
  unsigned GetBaudrate() const noexcept override;
  void SetBaudrate(unsigned baud_rate) override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  std::size_t Read(std::span<std::byte> dest) override;
  void WaitRead(std::chrono::steady_clock::duration timeout) override;
};
