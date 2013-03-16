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

#include <stdio.h>
int main(int argc, char **argv)
{
  plan_tests(ARRAY_SIZE(valid) + ARRAY_SIZE(invalid) +
             ARRAY_SIZE(length) +
             ARRAY_SIZE(latin1_chars));

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

  return exit_status();
}
