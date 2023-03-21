// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LineReader.hpp"
#include "BufferedReader.hxx"

/**
 * Adapter between #Reader and #NLineReader.
 */
class BufferedLineReader : public NLineReader {
  BufferedReader buffered;

public:
  explicit BufferedLineReader(Reader &reader) noexcept
    :buffered(reader) {}

public:
  /* virtual methods from class NLineReader */
  char *ReadLine() override;
};
