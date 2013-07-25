/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_OS_FILEUTIL_HPP
#define XCSOAR_OS_FILEUTIL_HPP

#include "Compiler.h"

#include <stdint.h>
#include <tchar.h>

#ifdef HAVE_POSIX
#include <unistd.h>
#include <stdio.h>
#else
#include <windows.h>
#endif

namespace File
{
  /**
   * Base class for a FileVisitor that is used by Directory::VisitFiles()
   */
  class Visitor
  {
  public:
    /**
     * The function that is called when the visitor is visiting a specific file
     * @param path Complete path of the file (e.g. "xyz/abc/bla.txt")
     * @param filename Base name of the file (e.g. "bla.txt")
     */
    virtual void Visit(const TCHAR* path, const TCHAR* filename) = 0;
  };
}

namespace Directory
{
  /**
   * Returns whether the given folder exists and is a folder (not a file)
   * @param path File system path to check
   * @return True if the folder exists
   */
  gcc_pure
  bool Exists(const TCHAR* path);

  /**
   * Creates a new folder at the given path
   * @param path Path to the folder that should be created
   */
  void Create(const TCHAR* path);

  /**
   * Visit all the files of a specific directory with the given visitor
   * @param path Path to visit
   * @param visitor Visitor that should be used
   * @param recursive If true all subfolders will be visited too
   */
  void VisitFiles(const TCHAR* path, File::Visitor &visitor,
                  bool recursive = false);
  /**
   * Visit all the files of a specific directory that match the given
   * filename pattern with the given visitor
   * @param path Path to visit
   * @param filter Filename pattern that should be searched for
   * @param visitor Visitor that should be used
   * @param recursive If true all subfolders will be visited too
   */
  void VisitSpecificFiles(const TCHAR* path, const TCHAR* filter,
                          File::Visitor &visitor, bool recursive = false);
}

namespace File
{
  /**
   * Returns whether a file or directory or any other directory entry
   * with the specified name exists.
   */
  gcc_pure
  bool ExistsAny(const TCHAR *path);

  /**
   * Returns whether the given file exists and is a file (not a folder)
   * @param path File system path to check
   * @return True if the file exists
   */
  gcc_pure
  bool Exists(const TCHAR* path);

#if defined(WIN32) && defined(UNICODE) && !defined(_WIN32_WCE)
  gcc_pure
  bool Exists(const char *path);
#endif

  /**
   * Deletes the given file
   * @param path Path to the file that should be deleted
   * @return True if the file was successfully deleted
   */
  static inline bool
  Delete(const TCHAR *path)
  {
#ifdef HAVE_POSIX
    return unlink(path) == 0;
#else
    return DeleteFile(path);
#endif
  }

  static inline bool
  Rename(const TCHAR *oldpath, const TCHAR *newpath)
  {
#ifdef HAVE_POSIX
    /* XXX handle EXDEV; cross-filesystem moves are not supported by
       rename() */
    return rename(oldpath, newpath) == 0;
#else
    return MoveFile(oldpath, newpath) != 0;
#endif
  }

  /**
   * Atomically rename a file, optionally replacing an existing file.
   *
   * Due to API limitations, this operation is not atomic on Windows
   * CE.
   */
  static inline bool
  Replace(const TCHAR *oldpath, const TCHAR *newpath)
  {
#ifdef HAVE_POSIX
    return rename(oldpath, newpath) == 0;
#elif defined(_WIN32_WCE)
    /* MoveFileEx() is not available on Windows CE, we need to fall
       back to non-atomic delete and rename */
    Delete(newpath);
    return MoveFile(oldpath, newpath) != 0;
#else
    return MoveFileEx(oldpath, newpath, MOVEFILE_REPLACE_EXISTING) != 0;
#endif
  }

  /**
   * Returns the size of a regular file in bytes.
   */
  gcc_pure
  uint64_t GetSize(const TCHAR *path);

  /**
   * Get a timestamp of last modification that can be used to compare
   * two files with each other
   * @param path Path to the file
   * @return 0 in case of failure or a timestamp for comparison
   */
  gcc_pure
  uint64_t GetLastModification(const TCHAR *path);

  /**
   * Sets the modification timestamp of the file to the current system time
   * @param path Path to the file
   * @return True in case of success, False otherwise
   */
  bool Touch(const TCHAR *path);

  /**
   * Read data from a file and null-terminate it.
   *
   * @param size the size of the buffer, including space for the null
   * terminator
   * @return false on error
   */
  bool ReadString(const TCHAR *path, char *buffer, size_t size);

  /**
   * Write a string to an existing file.  It will never create a new
   * file or truncate the existing file.  This function may be useful
   * for writing sysfs files.
   */
  bool WriteExisting(const TCHAR *path, const char *value);

  /**
   * Create a file with the given name, and leave it empty.
   *
   * @return true on success, false on error or if the file already
   * exists
   */
  bool CreateExclusive(const TCHAR *path);
}

#endif
