// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/UriUtil.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(9);

  /* basic %XX decoding */
  {
    std::string s = "hello%20world";
    PercentDecode(s);
    ok1(s == "hello world");
  }

  /* colon and slash (SAF URIs) */
  {
    std::string s = "primary%3ADocuments%2Fbackup";
    PercentDecode(s);
    ok1(s == "primary:Documents/backup");
  }

  /* no encoded sequences — pass through unchanged */
  {
    std::string s = "plain_text";
    PercentDecode(s);
    ok1(s == "plain_text");
  }

  /* empty string */
  {
    std::string s;
    PercentDecode(s);
    ok1(s.empty());
  }

  /* truncated sequence at end — left unchanged */
  {
    std::string s = "abc%2";
    PercentDecode(s);
    bool unchanged = (s == "abc%2");
    ok1(unchanged);
  }

  /* invalid hex digit — left unchanged */
  {
    std::string s = "abc%GGdef";
    PercentDecode(s);
    bool unchanged = (s == "abc%GGdef");
    ok1(unchanged);
  }

  /* lone percent at end */
  {
    std::string s = "abc%";
    PercentDecode(s);
    bool unchanged = (s == "abc%");
    ok1(unchanged);
  }

  /* multiple consecutive encodings */
  {
    std::string s = "%41%42%43";
    PercentDecode(s);
    ok1(s == "ABC");
  }

  /* lowercase hex digits */
  {
    std::string s = "%2f%3a";
    PercentDecode(s);
    ok1(s == "/:"); 
  }

  return exit_status();
}
