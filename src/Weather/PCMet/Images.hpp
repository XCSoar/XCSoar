// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PCMetSettings;
class CurlGlobal;
class ProgressListener;
class AllocatedPath;
namespace Co { template<typename T> class Task; }

namespace PCMet {

struct ImageArea {
  const char *name;
  const char *display_name;
};

struct ImageType {
  const char *uri;
  const char *display_name;

  const ImageArea *areas;
};

extern const ImageType image_types[];

/**
 * Throws on error.
 */
Co::Task<::AllocatedPath>
DownloadLatestImage(const char *type, const char *area,
                    const PCMetSettings &settings,
                    CurlGlobal &curl, ProgressListener &progress);

} // namespace PCMet
