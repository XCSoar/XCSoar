// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "io/NullDataHandler.hpp"

#include <string>
#include <string_view>
#include <vector>

/**
 * Port that records every Write() for driver unit tests.
 * PortWriteNMEA() writes '$', body, then "*CS\\r\\n" in separate
 * calls; this class reassembles complete lines.
 */
class DumpPort : public Port {
  NullDataHandler null_handler;
  std::string buffer;
  std::vector<std::string> lines;

public:
  DumpPort() noexcept
    :Port(nullptr, null_handler) {}

  void Clear() noexcept {
    buffer.clear();
    lines.clear();
  }

  const std::vector<std::string> &GetLines() const noexcept {
    return lines;
  }

  /**
   * Return the first recorded line that contains @p needle, or
   * nullptr if none.
   */
  [[gnu::pure]]
  const char *FindContaining(std::string_view needle) const noexcept {
    for (const auto &line : lines)
      if (line.find(needle) != std::string::npos)
        return line.c_str();
    return nullptr;
  }

  /* virtual methods from class Port */
  PortState GetState() const noexcept override {
    return PortState::READY;
  }

  std::size_t Write(std::span<const std::byte> src) override {
    buffer.append(reinterpret_cast<const char *>(src.data()), src.size());
    for (;;) {
      const auto pos = buffer.find('\n');
      if (pos == std::string::npos)
        break;
      std::string line = buffer.substr(0, pos);
      buffer.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      if (!line.empty())
        lines.push_back(std::move(line));
    }
    return src.size();
  }

  bool Drain() override {
    return true;
  }

  void Flush() override {}

  unsigned GetBaudrate() const noexcept override {
    return 0;
  }

  void SetBaudrate(unsigned) override {}

  bool StopRxThread() override {
    return true;
  }

  bool StartRxThread() override {
    return true;
  }

  std::size_t Read(std::span<std::byte>) override {
    return 0;
  }

  void WaitRead(std::chrono::steady_clock::duration) override {}
};
