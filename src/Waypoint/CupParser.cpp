// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupParser.hpp"
#include "util/StringSplit.hxx"
#include "util/StringStrip.hxx"

std::string_view
CupNextColumn(std::string_view &line) noexcept
{
  line = StripLeft(line);

  if (line.starts_with('"')) {
    /* quoted: the value ends at the closing double quote */
    line.remove_prefix(1);

    auto [value, rest1] = Split(line, '"');

    auto [more, rest2] = Split(rest1, ',');

    /* ... unless there is another double quote ... */
    auto another_quote = more.rfind('"');
    if (another_quote != more.npos)
      /* ... which is a syntax error, but XCSoar supported this
         syntax, so let's emulate it */
      value = line.substr(0, &more[another_quote] - line.data());

    line = rest2;
    return value;
  } else {
    auto [value, rest] = Split(line, ',');
    line = rest;
    return StripRight(value);
  }
}

void
CupSplitColumns(std::string_view line,
                std::span<std::string_view> columns) noexcept
{
  for (auto &i : columns)
    i = CupNextColumn(line);
}
