// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogFile.hpp"
#include "tap.h"
#include "util/StringAPI.hxx"
#include <string>
#include <fmt/format.h>

// Capture the last log message
static std::string last_log_message;

// Mock LogString to capture output instead of printing
void
LogString(std::string_view s) noexcept
{
  last_log_message = s;
}

// Implement LogVFmt (normally in LogFile.cpp)
void
LogVFmt(fmt::string_view format_str, fmt::format_args args) noexcept
{
  fmt::memory_buffer buffer;
#if FMT_VERSION >= 80000
  fmt::vformat_to(std::back_inserter(buffer), format_str, args);
#else
  fmt::vformat_to(buffer, format_str, args);
#endif
  LogString({buffer.data(), buffer.size()});
}

int
main(int argc, char **argv)
{
  plan(5);

  // Test 1: Basic ASCII
  LogFmt("Hello World");
  ok(last_log_message == "Hello World", "Basic ASCII logging");

  // Test 2: UTF-8 string passing
  // "Euro \xE2\x82\xAC" is UTF-8 for Euro symbol
  LogFmt("Cost: {} EUR", "\xE2\x82\xAC");
  ok(last_log_message == "Cost: \xE2\x82\xAC EUR", "UTF-8 string passing");

  // Test 3: TCHAR handling
  // This simulates passing a TCHAR* (which is char* on Linux, wchar_t* on Windows)
  // LogFmt should convert it to UTF-8 output
#ifdef _UNICODE
  const wchar_t *tchar_str = L"\u20AC"; // Euro symbol
#else
  const char *tchar_str = "\xE2\x82\xAC"; // Euro symbol (UTF-8)
#endif

  LogFmt("Symbol: {}", tchar_str);
  
  // On both platforms, the output should be UTF-8
  ok(last_log_message == "Symbol: \xE2\x82\xAC", "TCHAR to UTF-8 conversion");

  // Test 4: Explicit Wide String (Cross-platform verification)
  // Even on Linux (where TCHAR is char), LogFmt should handle wchar_t* correctly
  // by converting it to UTF-8. This confirms safety for potential TCHAR mismatches or Windows behavior.
  // Note: fmt library should automatically convert wchar_t* to UTF-8 when formatting into narrow string buffer.
  const wchar_t *wide_str = L"\u20AC"; // Wide Euro (UTF-16 on Windows, UTF-32 on Linux)
  LogFmt("Wide: {}", wide_str);
  // The fmt library converts wchar_t* to UTF-8 automatically on all platforms
  ok(last_log_message == "Wide: \xE2\x82\xAC", "Explicit Wide string (wchar_t*) to UTF-8 conversion");

  // Test 5: Narrow String on Windows (Accidental char* pass-through)
  // On Windows (where TCHAR is wchar_t), if someone accidentally passes a char* (narrow string),
  // LogFmt should handle it correctly by treating it as UTF-8. This tests the opposite direction
  // of Test 4 and ensures fmt library handles mixed string types safely.
  const char *narrow_str = "\xE2\x82\xAC"; // UTF-8 Euro (narrow string)
  LogFmt("Narrow: {}", narrow_str);
  ok(last_log_message == "Narrow: \xE2\x82\xAC", "Narrow string (char*) to UTF-8 conversion on all platforms");

  return exit_status();
}
