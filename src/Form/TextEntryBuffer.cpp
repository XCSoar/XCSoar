// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextEntryBuffer.hpp"
#include "util/CharUtil.hxx"
#include "util/TruncateString.hpp"
#include "util/UTF8.hpp"

#include <algorithm>
#include <cstring>

TextEntryBuffer::TextEntryBuffer(const char *value, std::size_t width) noexcept
  :capacity(width == 0 ? CAPACITY : std::min(width, CAPACITY))
{
  CopyTruncateString(text, capacity, value != nullptr ? value : "");
  End();
}

std::size_t
TextEntryBuffer::GetSequenceLength() const noexcept
{
  return text[cursor] == 0 ? 0
    : std::max(std::size_t{1}, SequenceLengthUTF8(text + cursor));
}

std::size_t
TextEntryBuffer::GetPreviousCursor() const noexcept
{
  std::size_t previous = 0;
  for (std::size_t i = 0; i < cursor;) {
    previous = i;
    i += std::max(std::size_t{1}, SequenceLengthUTF8(text + i));
  }
  return previous;
}

void
TextEntryBuffer::Clear() noexcept
{
  cursor = 0;
  text[0] = 0;
}

void
TextEntryBuffer::End() noexcept
{
  cursor = strlen(text);
}

bool
TextEntryBuffer::MoveLeft() noexcept
{
  if (cursor == 0)
    return false;
  cursor = GetPreviousCursor();
  return true;
}

bool
TextEntryBuffer::MoveRight() noexcept
{
  if (text[cursor] == 0)
    return false;
  cursor += GetSequenceLength();
  return true;
}

bool
TextEntryBuffer::Insert(char ch) noexcept
{
  const auto length = strlen(text);
  if (!IsPrintableASCII(ch) || length + 1 >= capacity)
    return false;

  memmove(text + cursor + 1, text + cursor, length - cursor + 1);
  text[cursor++] = ch;
  return true;
}

bool
TextEntryBuffer::Replace(char ch) noexcept
{
  if (!IsPrintableASCII(ch))
    return false;

  if (text[cursor] == 0) {
    if (!Insert(ch))
      return false;
    --cursor;
    return true;
  }

  const auto sequence = GetSequenceLength();
  memmove(text + cursor + 1, text + cursor + sequence,
          strlen(text) - cursor - sequence + 1);
  text[cursor] = ch;
  return true;
}

bool
TextEntryBuffer::Backspace() noexcept
{
  if (!MoveLeft())
    return false;
  return Delete();
}

bool
TextEntryBuffer::Delete() noexcept
{
  const auto sequence = GetSequenceLength();
  if (sequence == 0)
    return false;
  memmove(text + cursor, text + cursor + sequence,
          strlen(text) - cursor - sequence + 1);
  return true;
}

char
TextEntryBuffer::Filter(unsigned ch, const char *allowed) noexcept
{
  if (ch < 0x20 || ch > 0x7e)
    return 0;
  if (allowed == nullptr || strchr(allowed, ch) != nullptr)
    return ch;

  const char upper = ToUpperASCII(ch);
  return strchr(allowed, upper) != nullptr ? upper : 0;
}
