// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct METAR;
struct TAF;
class CurlGlobal;
class ProgressListener;
namespace Co { template<typename T> class Task; }

namespace NOAADownloader {

/**
 * Downloads a METAR from the NOAA server
 *
 * Throws on error.
 *
 * @param code Four letter code of the airport (upper case)
 */
Co::Task<METAR>
DownloadMETAR(const char *code, CurlGlobal &curl, ProgressListener &progress);

/**
 * Downloads a METAR from the NOAA server
 *
 * Throws on error.
 *
 * @param code Four letter code of the airport (upper case)
 */
Co::Task<TAF>
DownloadTAF(const char *code, CurlGlobal &curl,
            ProgressListener &progress);

} // namespace NOAADownloader
