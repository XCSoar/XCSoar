// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticFifoBuffer.hxx"

#include <cstddef>

class Port;
class OperationEnvironment;
class TimeoutClock;

class PortNMEAReader {
  static constexpr std::size_t BUFFER_SIZE = 256;

  Port &port;
  OperationEnvironment &env;
  StaticFifoBuffer<char, BUFFER_SIZE> buffer;

public:
  PortNMEAReader(Port &_port, OperationEnvironment &_env)
    :port(_port), env(_env) {}

protected:
  bool Fill(TimeoutClock timeout);

  char *GetLine();

public:
  void Flush();

  char *ReadLine(TimeoutClock timeout);

  char *ExpectLine(const char *prefix, TimeoutClock timeout);
};
