// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"

#include <cstddef>
#include <cstdint>
#include <string>

class CurlGlobal;

namespace Net {

/**
 * Download a byte range of a URL into memory.
 *
 * This exists for files that are far larger than the part we need --
 * a cloud-optimized GeoTIFF, say, where the tile under the aircraft
 * is a few kilobytes of a multi-megabyte composite.  Nothing is
 * written to disk, because a range is small by construction and the
 * caller wants it in memory anyway.
 *
 * Throws on error, including when the server ignores the range and
 * offers the whole file instead.
 *
 * @param offset the first byte to fetch
 * @param length how many bytes, which must not be zero
 */
Co::Task<std::string>
CoGetRange(CurlGlobal &curl, const char *url,
           uint_least64_t offset, std::size_t length);

} // namespace Net
