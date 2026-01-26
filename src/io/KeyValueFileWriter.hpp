// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class BufferedOutputStream;

class KeyValueFileWriter {
  BufferedOutputStream &os;

public:
  explicit KeyValueFileWriter(BufferedOutputStream &_os):os(_os) {}

  void Write(const char *key, const char *value);
};
