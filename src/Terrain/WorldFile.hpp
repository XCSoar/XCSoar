// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct zzip_dir;
class GeoBounds;

GeoBounds
LoadWorldFile(struct zzip_dir *dir, const char *path,
              unsigned width, unsigned height);
