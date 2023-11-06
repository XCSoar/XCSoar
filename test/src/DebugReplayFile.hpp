// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DebugReplay.hpp"
#include "io/FileLineReader.hpp"

class DebugReplayFile : public DebugReplay {
protected:
  FileLineReaderA *reader;

public:
  DebugReplayFile(FileLineReaderA *_reader)
    : reader(_reader) {
  }

  ~DebugReplayFile() {
    delete reader;
  }
};
