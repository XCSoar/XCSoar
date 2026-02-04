// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnescapeCString.hpp"

#include <cctype>

std::string UnescapeCString(const std::string &s) noexcept {
  std::string out;
  out.reserve(s.size());

  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c != '\\') {
      out.push_back(c);
      continue;
    }

    // backslash found
    if (i + 1 >= s.size()) {
      out.push_back('\\');
      break;
    }

    char n = s[i+1];
    auto emit_simple_escape = [&](char ch) noexcept {
      out.push_back(ch);
      i += 1;
      return true;
    };

    bool handled = false;
    switch (n) {
    case 'r':
      handled = emit_simple_escape('\r');
      break;
    case 'n':
      handled = emit_simple_escape('\n');
      break;
    case 't':
      handled = emit_simple_escape('\t');
      break;
    case 'a':
      handled = emit_simple_escape('\a');
      break;
    case 'b':
      handled = emit_simple_escape('\b');
      break;
    case 'f':
      handled = emit_simple_escape('\f');
      break;
    case 'v':
      handled = emit_simple_escape('\v');
      break;
    case '\'':
      handled = emit_simple_escape('\'');
      break;
    case '"':
      handled = emit_simple_escape('"');
      break;
    case '\\':
      handled = emit_simple_escape('\\');
      break;
    default:
      break;
    }

    if (handled)
      continue;

    if (n == 'x' || n == 'X') {
      // hex escape \xHH
      if (i + 3 < s.size() && 
          std::isxdigit(static_cast<unsigned char>(s[i+2])) && 
          std::isxdigit(static_cast<unsigned char>(s[i+3]))) {
        auto hexval = [](char c)->int{
          if (c >= '0' && c <= '9') return c - '0';
          if (c >= 'a' && c <= 'f') return c - 'a' + 10;
          return c - 'A' + 10;
        };
        int v = hexval(s[i+2]) * 16 + hexval(s[i+3]);
        out.push_back(static_cast<char>(v));
        i += 3;
        continue;
      }
      // fallback: emit 'x' and continue
      out.push_back('x');
      i += 1;
      continue;
    }

    if (n >= '0' && n <= '7') {
      // octal escape: consume up to 3 octal digits, then store lowest byte
      int val = 0;
      size_t j = 1;
      while (j <= 3 && i + j < s.size() && s[i+j] >= '0' && s[i+j] <= '7') {
        val = val * 8 + (s[i+j] - '0');
        ++j;
      }
      out.push_back(static_cast<char>(val & 0xFF));
      i += j - 1;
      continue;
    }

    // Unknown escape: emit the escaped character literally
    out.push_back(n);
    i += 1;
  }

  return out;
}
