// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/StandardVersion.hpp"
#include "ProductName.hpp"
#include "Version.hpp"

#include <cstdio>

void
PrintStandardVersion(const char *canonical_program_name,
                     const char *version_string) noexcept
{
  std::printf("%s (%s) %s\n"
              "Copyright (C) %d The XCSoar Project\n"
              "License GPLv2+: GNU GPL version 2 or later "
              "<https://www.gnu.org/licenses/gpl.html>\n"
              "This is free software: you are free to change and "
              "redistribute it.\n"
              "There is NO WARRANTY, to the extent permitted by law.\n",
              canonical_program_name, PRODUCT_NAME, version_string,
              CompileDateYear());
}
