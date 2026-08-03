// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

class Path;

/**
 * CUPX archive reader (Naviter SeeYou waypoint-with-images format).
 *
 * There is no public specification; the layout below was
 * reverse-engineered from files produced by SeeYou (2022+).
 *
 * File layout
 * -----------
 *   [0x000]  Optional 256-byte header.
 *            Bytes 0-3 are ASCII "CUPX" (LE32 0x58505543).
 *            The remaining 252 bytes are unknown / zero-padded.
 *            Some older files omit this header entirely.
 *
 *   [0x100]  pics.zip  -- standard ZIP archive containing images.
 *            Entries are stored under a "Pics/" folder prefix
 *            (e.g. "Pics/photo1.jpg").  The CUP "pics" column
 *            references them by bare filename ("photo1.jpg").
 *            Matching is case-insensitive.
 *
 *   [???]    points.zip -- standard ZIP archive appended directly
 *            after pics.zip.  Contains a single entry "POINTS.CUP"
 *            in SeeYou CSV format (with the 2022+ "pics" column).
 *
 * Offset handling
 * ---------------
 * Because two ZIP archives are concatenated (and optionally prefixed
 * by the 256-byte header), central-directory offsets stored inside
 * each ZIP are relative to that ZIP's own start, not to the
 * beginning of the .cupx file.  Each archive is located via its
 * End-of-Central-Directory record: the SFX "concat" delta (EOCD
 * position minus CD size minus CD offset) is the absolute start of
 * that ZIP.  points.zip is the last EOCD near EOF; pics.zip is the
 * EOCD whose concat equals the post-header offset, searched backward
 * from the start of points.zip (not from EOF) so a large points.zip
 * cannot hide pics.zip beyond the EOCD search window.
 *
 * Some SeeYou exports set ZIP general-purpose bit 3 (data descriptor)
 * so local-header sizes are zero; real sizes are taken from the
 * central directory for both archives.
 */
namespace CupxArchive {

/**
 * Extract an image from the CUPX archive by its bare filename
 * (as listed in the CUP "pics" column, e.g. "image1.jpg").
 * Handles the internal "Pics/" folder prefix and case-insensitive
 * matching automatically.
 */
std::vector<std::byte>
ExtractImage(Path cupx_path, std::string_view image_name);

/**
 * Extract and return the POINTS.CUP content from a CUPX archive.
 */
std::vector<std::byte>
ExtractPointsCup(Path cupx_path);

} // namespace CupxArchive
