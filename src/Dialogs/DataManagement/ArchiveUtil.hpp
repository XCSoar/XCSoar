// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <string_view>

class OperationEnvironment;
class Path;

using ArchiveExcludePathFn = bool (*)(std::string_view) noexcept;

bool
CreateBackup(Path source_root, Path destination_tar,
             ArchiveExcludePathFn exclude,
             OperationEnvironment &env,
             unsigned &created_files,
             std::string &error_message) noexcept;

bool
RestoreBackup(Path tar_file, Path destination_root,
              ArchiveExcludePathFn exclude,
              OperationEnvironment &env,
              unsigned &restored_files,
              unsigned &failed_files,
              std::string &error_message) noexcept;
