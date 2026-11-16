// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Volkslogger/grecord.h"
#include "TestUtil.hpp"

#include <cstdio>
#include <cstring>

static void
Expect(const uint8_t *data, size_t length, const char *expected)
{
  FILE *fp = tmpfile();
  if (fp == nullptr) {
    ok1(false);
    return;
  }

  print_g_record(fp, data, length);
  rewind(fp);

  char buf[160];
  const size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
  buf[n] = '\0';
  fclose(fp);

  ok1(strcmp(buf, expected) == 0);
}

int
main()
{
  plan_tests(4);

  const uint8_t one[] = {0x00};
  Expect(one, 1, "G0Ezz\n");

  /* Three bytes are one group.  A second group would be the 0xff pad. */
  const uint8_t three[] = {0x00, 0x00, 0x00};
  Expect(three, 3, "G0000\n");

  uint8_t zeros[57];
  memset(zeros, 0, sizeof(zeros));

  char full[80];
  full[0] = 'G';
  memset(full + 1, '0', 72);
  full[73] = '\n';
  full[74] = '\0';
  Expect(zeros, 54, full);

  char next[90];
  memcpy(next, full, 74);
  memcpy(next + 74, "G0000\n", 7);
  Expect(zeros, 57, next);

  return exit_status();
}
