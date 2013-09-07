/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Util/UTF8.hpp"
#include "Util/Macros.hpp"
#include "TestUtil.hpp"

#include <string.h>

static const char *const valid[] = {
  "",
  " ",
  "foo",
  "\x01",
  "\x7f",
  "\xc3\xbc",
  "\xd7\x9e",
  "\xe7\x9b\xae",
};

static const char *const invalid[] = {
  "\x80",
  "\xff",
  "\xc3",
  "\xd7",
  "\xe7",
  "\xe7\x9b",
};

static const struct {
  const char *value;
  size_t length;
} length[] = {
  { "", 0 },
  { " ", 1 },
  { "foo", 3 },
  { "\xc3\xbc", 1 },
  { "\xd7\x9e", 1 },
  { "\xe7\x9b\xae", 1 },
};

static const struct {
  char ch;
  const char *utf8;
} latin1_chars[] = {
  { 0, "" },
  { ' ', " " },
  { '\xfc', "\xc3\xbc", },
};

static const struct {
  const char *input, *output;
} crop[] = {
  { "", "" },
  { " ", " " },
  { "foo\xc3\xbc", "foo\xc3\xbc", },
  { "foo\xc3", "foo", },
  { "foo\xe7\x9b\xae", "foo\xe7\x9b\xae", },
  { "foo\xe7\x9b", "foo", },
  { "foo\xe7", "foo", },
};

#include <stdio.h>
int main(int argc, char **argv)
{
  plan_tests(ARRAY_SIZE(valid) + ARRAY_SIZE(invalid) +
             ARRAY_SIZE(length) +
             ARRAY_SIZE(crop) +
             ARRAY_SIZE(latin1_chars) +
             9 + 27);

  for (auto i : valid)
    ok1(ValidateUTF8(i));

  for (auto i : invalid)
    ok1(!ValidateUTF8(i));

  for (auto &l : length)
    ok1(l.length == LengthUTF8(l.value));

  char buffer[64];

  for (auto &l : latin1_chars) {
    *Latin1ToUTF8(l.ch, buffer) = 0;
    ok1(strcmp(l.utf8, buffer) == 0);
  }

  for (auto &c : crop) {
    strcpy(buffer, c.input);
    CropIncompleteUTF8(buffer);
    ok1(strcmp(c.output, buffer) == 0);
  }

  {
    const char *p = "foo\xe7\x9b\xae";
    auto n = NextUTF8(p);
    ok1(n.first == 'f');
    ok1(n.second == p + 1);

    n = NextUTF8(p + 1);
    ok1(n.first == 'o');
    ok1(n.second == p + 2);

    n = NextUTF8(p + 2);
    ok1(n.first == 'o');
    ok1(n.second == p + 3);

    n = NextUTF8(p + 3);
    ok1(n.first == 30446);
    ok1(n.second == p + 6);

    n = NextUTF8(p + 6);
    ok1(n.first == 0);
  }

  /* test UnicodeToUTF8() */

  buffer[0] = 1;
  ok1(UnicodeToUTF8(0, buffer) == buffer + 1);
  ok1(buffer[0] == 0);

  ok1(UnicodeToUTF8(' ', buffer) == buffer + 1);
  ok1(buffer[0] == ' ');

  ok1(UnicodeToUTF8(0x7f, buffer) == buffer + 1);
  ok1(buffer[0] == 0x7f);

  ok1(UnicodeToUTF8(0xa2, buffer) == buffer + 2);
  ok1(buffer[0] == char(0xc2));
  ok1(buffer[1] == char(0xa2));

  ok1(UnicodeToUTF8(0x6fb3, buffer) == buffer + 3);
  ok1(buffer[0] == char(0xe6));
  ok1(buffer[1] == char(0xbe));
  ok1(buffer[2] == char(0xb3));

  ok1(UnicodeToUTF8(0xffff, buffer) == buffer + 3);
  ok1(buffer[0] == char(0xef));
  ok1(buffer[1] == char(0xbf));
  ok1(buffer[2] == char(0xbf));

  ok1(UnicodeToUTF8(0x10000, buffer) == buffer + 4);
  ok1(buffer[0] == char(0xf0));
  ok1(buffer[1] == char(0x90));
  ok1(buffer[2] == char(0x80));
  ok1(buffer[3] == char(0x80));

  ok1(UnicodeToUTF8(0x10ffff, buffer) == buffer + 4);
  ok1(buffer[0] == char(0xf4));
  ok1(buffer[1] == char(0x8f));
  ok1(buffer[2] == char(0xbf));
  ok1(buffer[3] == char(0xbf));

  return exit_status();
}
