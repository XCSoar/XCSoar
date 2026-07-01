// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/FileType.hpp"

#include <initializer_list>

class Path;
class AllocatedPath;

/**
 * Return @p path if it exists; otherwise, if the basename matches a
 * typed data subdirectory layout, try that location.
 */
[[gnu::pure]]
AllocatedPath
ResolveLocalDataFile(AllocatedPath path,
                     FileType file_type = FileType::UNKNOWN) noexcept;

/**
 * Canonical save path for a typed data file.
 */
AllocatedPath
TypedDataSavePath(FileType file_type, const char *filename) noexcept;

/**
 * Resolve a typed data file for reading (typed subdir first, legacy
 * data-root fallback).
 */
[[gnu::pure]]
AllocatedPath
ResolveTypedDataFilePath(FileType file_type,
                         const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
ResolveTypedDataFilePath(FileType file_type, const char *filename,
                         std::initializer_list<const char *> legacy_names) noexcept;

AllocatedPath
RepositoryDataSavePath(const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
ResolveRepositoryDataPath(const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
RepositoryDownloadRelativePath(const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
ResolveDownloadDestinationPath(Path path) noexcept;

[[gnu::pure]]
AllocatedPath
GetFileTypeDownloadRelativePath(FileType file_type,
                                const char *filename) noexcept;

bool
EnsureFileTypeDownloadDirectory(FileType file_type) noexcept;

AllocatedPath
CacheDataSavePath(const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
ResolveCacheDataPath(const char *filename) noexcept;

AllocatedPath
LogsDataSavePath(const char *filename) noexcept;

[[gnu::pure]]
AllocatedPath
ResolveLogsDataPath(const char *filename) noexcept;
