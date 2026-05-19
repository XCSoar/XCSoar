// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/DirEntry.hpp"

#include <string>
#include <string_view>
#include <vector>

class OperationEnvironment;
class Path;
class OutputStream;
class Reader;

using ArchiveExcludePathFn = bool (*)(std::string_view) noexcept;

/**
 * List .tar files on the device at the given path.
 */
[[nodiscard]]
std::vector<DirEntry>
EnumerateTarFiles(Path dir);

/**
 * Create a tar archive of all files under @p source_root,
 * writing to the given output stream.
 *
 * The caller is responsible for opening/committing/closing
 * the stream.
 */
bool
CreateBackup(Path source_root, OutputStream &output,
             ArchiveExcludePathFn exclude,
             OperationEnvironment &env,
             unsigned &created_files,
             std::string &error_message) noexcept;

/**
 * Restore files from a tar archive read from the given reader
 * into @p destination_root.
 *
 * The caller is responsible for opening the stream.
 */
bool
RestoreBackup(Reader &input, Path destination_root,
              ArchiveExcludePathFn exclude,
              OperationEnvironment &env,
              unsigned &restored_files,
              unsigned &failed_files,
              std::string &error_message) noexcept;
