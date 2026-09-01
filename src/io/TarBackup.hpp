// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

class OperationEnvironment;
class Path;
class OutputStream;
class Reader;

/**
 * Shall this archive entry (a relative path with '/' separators) be
 * left out?  May be empty (nothing is excluded).
 */
using ArchiveExcludePathFn = std::function<bool(std::string_view)>;

/**
 * Create a tar archive of all files under @p source_root,
 * writing to the given output stream.
 *
 * A file that cannot be opened for reading - typically one another
 * part of the program is writing right now, which Windows refuses
 * to share - is left out and named in @p skipped_files; the backup
 * carries on.  An error while a file is being copied still fails
 * the whole backup, the archive would be broken otherwise.
 *
 * The caller is responsible for opening/committing/closing
 * the stream.
 */
bool
CreateBackup(Path source_root, OutputStream &output,
             const ArchiveExcludePathFn &exclude,
             OperationEnvironment &env,
             unsigned &created_files,
             std::vector<std::string> &skipped_files,
             std::string &error_message) noexcept;

/**
 * Restore files from a tar archive read from the given reader
 * into @p destination_root.
 *
 * The caller is responsible for opening the stream.
 */
bool
RestoreBackup(Reader &input, Path destination_root,
              const ArchiveExcludePathFn &exclude,
              OperationEnvironment &env,
              unsigned &restored_files,
              unsigned &failed_files,
              std::string &error_message) noexcept;
