// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Path.hpp"

#include <chrono>
#include <cstdint>

#include <tchar.h>

#ifdef HAVE_POSIX
#include <unistd.h>
#include <stdio.h>
#else
#include <fileapi.h>
#include <windef.h> // for HWND (needed by winbase.h)
#include <winbase.h>
#endif

namespace File {

/**
 * Base class for a FileVisitor that is used by Directory::VisitFiles()
 */
class Visitor {
public:
  /**
   * The function that is called when the visitor is visiting a specific file
   * @param path Complete path of the file (e.g. "xyz/abc/bla.txt")
   * @param filename Base name of the file (e.g. "bla.txt")
   */
  virtual void Visit(Path path, Path filename) = 0;
};

} // namespace File

namespace Directory {

/**
 * Returns whether the given folder exists and is a folder (not a file)
 * @param path File system path to check
 * @return True if the folder exists
 */
[[gnu::pure]]
bool
Exists(Path path) noexcept;

/**
 * Creates a new folder at the given path
 * @param path Path to the folder that should be created
 */
void
Create(Path path) noexcept;

/**
 * Visit all the files of a specific directory with the given visitor
 * @param path Path to visit
 * @param visitor Visitor that should be used
 * @param recursive If true all subfolders will be visited too
 */
void
VisitFiles(Path path, File::Visitor &visitor,
           bool recursive = false);

/**
 * Visit all the files of a specific directory that match the given
 * filename pattern with the given visitor
 * @param path Path to visit
 * @param filter Filename pattern that should be searched for
 * @param visitor Visitor that should be used
 * @param recursive If true all subfolders will be visited too
 */
void
VisitSpecificFiles(Path path, const char *filter,
                   File::Visitor &visitor, bool recursive = false);

} // namespace Directory

namespace File {

/**
 * Returns whether a file or directory or any other directory entry
 * with the specified name exists.
 */
[[gnu::pure]]
bool
ExistsAny(Path path) noexcept;

/**
 * Returns whether the given file exists and is a file (not a folder)
 * @param path File system path to check
 * @return True if the file exists
 */
[[gnu::pure]]
bool
Exists(Path path) noexcept;

#ifdef HAVE_POSIX

/**
 * Returns whether the given file descriptor is a character device e.g. /dev/ttyS<n>
 * @param path File system path to check
 * @return True if the character device exists
 */
[[gnu::pure]]
bool
IsCharDev(Path path) noexcept;

#endif // HAVE_POSIX

/**
 * Deletes the given file
 * @param path Path to the file that should be deleted
 * @return True if the file was successfully deleted
 */
static inline bool
Delete(Path path) noexcept
{
#ifdef HAVE_POSIX
  return unlink(path.c_str()) == 0;
#else
  return DeleteFile(path.c_str());
#endif
}

static inline bool
Rename(Path oldpath, Path newpath) noexcept
{
#ifdef HAVE_POSIX
  /* XXX handle EXDEV; cross-filesystem moves are not supported by
     rename() */
  return rename(oldpath.c_str(), newpath.c_str()) == 0;
#else
  return MoveFile(oldpath.c_str(), newpath.c_str()) != 0;
#endif
}

/**
 * Atomically rename a file, optionally replacing an existing file.
 */
static inline bool
Replace(Path oldpath, Path newpath) noexcept
{
#ifdef HAVE_POSIX
  return rename(oldpath.c_str(), newpath.c_str()) == 0;
#else
  return MoveFileEx(oldpath.c_str(), newpath.c_str(),
                    MOVEFILE_REPLACE_EXISTING) != 0;
#endif
}

/**
 * Returns the size of a regular file in bytes.
 */
[[gnu::pure]]
uint64_t
GetSize(Path path) noexcept;

/**
 * Get a timestamp of last modification that can be used to compare
 * two files with each other
 * @param path Path to the file
 * @return 0 in case of failure or a timestamp for comparison
 */
[[gnu::pure]]
std::chrono::system_clock::time_point
GetLastModification(Path path) noexcept;

/**
 * Sets the modification timestamp of the file to the current system time
 * @param path Path to the file
 * @return True in case of success, False otherwise
 */
bool
Touch(Path path) noexcept;

/**
 * Read data from a file and null-terminate it.
 *
 * @param size the size of the buffer, including space for the null
 * terminator
 * @return false on error
 */
bool
ReadString(Path path, char *buffer, size_t size) noexcept;

/**
 * Write a string to an existing file.  It will never create a new
 * file or truncate the existing file.  This function may be useful
 * for writing sysfs files.
 */
bool
WriteExisting(Path path, const char *value) noexcept;

/**
 * Create a file with the given name, and leave it empty.
 *
 * @return true on success, false on error or if the file already
 * exists
 */
bool
CreateExclusive(Path path) noexcept;

} // namespace File
