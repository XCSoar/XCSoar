// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "jasper/jas_stream.h"

struct zzip_dir;

/**
 * Throws on error.
 */
jas_stream_t *
OpenJasperZzipStream(struct zzip_dir *dir, const char *path);
