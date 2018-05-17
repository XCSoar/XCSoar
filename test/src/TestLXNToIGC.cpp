/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Device/Driver/LX/Convert.hpp"
#include "TestUtil.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const long MAX_LXN_SIZE = 1024 * 1024;

static const char *lxn_path = "test/data/lxn_to_igc/18BF14K1.FIL";
static const char *igc_in_path = "test/data/lxn_to_igc/18BF14K1.igc";
static const char *igc_out_path = "output/18BF14K1.igc";

static bool
RunConversion()
{
  FILE *lxn_file = fopen(lxn_path, "rb");
  if (lxn_file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", lxn_path);
    return false;
  }

  FILE *igc_file = fopen(igc_out_path, "wb");
  if (igc_file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", igc_out_path);
    return false;
  }

  long size;
  if (fseek(lxn_file, 0, SEEK_END) != 0 || (size = ftell(lxn_file)) <= 0 ||
      fseek(lxn_file, 0, SEEK_SET) != 0 || size > MAX_LXN_SIZE)  {
    fprintf(stderr, "Failed to seek file %s\n", lxn_path);
    fclose(lxn_file);
    fclose(igc_file);
    return false;
  }

  void *data = malloc(size);
  size_t n = fread(data, 1, size, lxn_file);
  fclose(lxn_file);
  if (n != (size_t)size) {
    free(data);
    fprintf(stderr, "Failed to read from file %s\n", lxn_path);
    fclose(igc_file);
    return false;
  }

  bool success = ok1(LX::ConvertLXNToIGC(data, n, igc_file));
  fclose(igc_file);
  free(data);

  return success;
}

static bool
CompareFiles()
{
  FILE *igc_in_file = fopen(igc_in_path, "rb");
  if (igc_in_file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", igc_in_path);
    return false;
  }

  FILE *igc_out_file = fopen(igc_out_path, "rb");
  if (igc_out_file == NULL) {
    fprintf(stderr, "Failed to open file %s\n", igc_out_path);
    fclose(igc_in_file);
    return false;
  }

  long in_size;
  if (fseek(igc_in_file, 0, SEEK_END) != 0 || (in_size = ftell(igc_in_file)) <= 0 ||
      fseek(igc_in_file, 0, SEEK_SET) != 0 || in_size > MAX_LXN_SIZE)  {
    fprintf(stderr, "Failed to seek file %s\n", igc_in_path);
    fclose(igc_in_file);
    fclose(igc_out_file);
    return false;
  }

  long out_size;
  if (fseek(igc_out_file, 0, SEEK_END) != 0 || (out_size = ftell(igc_out_file)) <= 0 ||
      fseek(igc_out_file, 0, SEEK_SET) != 0 || out_size > MAX_LXN_SIZE)  {
    fprintf(stderr, "Failed to seek file %s\n", igc_out_path);
    fclose(igc_in_file);
    fclose(igc_out_file);
    return false;
  }

  if (in_size != out_size) {
    fprintf(stderr, "File size doesn't match\n");
    fclose(igc_in_file);
    fclose(igc_out_file);
    return false;
  }

  void *in_data = malloc(in_size);
  size_t in_n = fread(in_data, 1, in_size, igc_in_file);
  fclose(igc_in_file);
  if (in_n != (size_t)in_size) {
    free(in_data);
    fprintf(stderr, "Failed to read from file %s\n", igc_in_path);
    fclose(igc_out_file);
    return false;
  }

  void *out_data = malloc(out_size);
  size_t out_n = fread(out_data, 1, out_size, igc_out_file);
  fclose(igc_out_file);
  if (out_n != (size_t)in_size) {
    free(out_data);
    fprintf(stderr, "Failed to read from file %s\n", igc_out_path);
    return false;
  }

  return memcmp(in_data, out_data, in_size) == 0;
}

int main(int argc, char **argv)
{
  plan_tests(2);

  if (!RunConversion())
    skip(1, 0, "conversion failed");

  ok1(CompareFiles());

  return exit_status();
}
