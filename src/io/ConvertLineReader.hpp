// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "LineReader.hpp"
#include "StringConverter.hpp"

#include <memory>

/**
 * Adapter which converts data from LineReader<char> to
 * LineReader<TCHAR>.
 */
class ConvertLineReader : public TLineReader {
  std::unique_ptr<LineReader<char>> source;

  StringConverter converter;

public:
  ConvertLineReader(std::unique_ptr<LineReader<char>> &&_source,
                    Charset cs=Charset::UTF8);

protected:
  LineReader<char> &GetSource() {
    return *source;
  }

public:
  /* virtual methods from class LineReader */
  TCHAR *ReadLine() override;
  long GetSize() const override;
  long Tell() const override;
};
