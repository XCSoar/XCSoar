// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Print a GNU-style `--version` block to standard output (program name,
 * \c PRODUCT_NAME, version string, copyright year from \c CompileDateYear()
 * in Version.hpp, GPLv2+ notice).
 */
void
PrintStandardVersion(const char *canonical_program_name,
                     const char *version_string) noexcept;
