// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

#ifdef ANDROID
#include <jni.h>  // Required for JNIEnv definition on Android
#endif

class Path;
class AllocatedPath;

namespace File {
  class Visitor;
}

/**
 * Determine the data path.
 *
 * Throws on error.
 */
void InitialiseDataPath();

/**
 * Release resources obtained by InitialiseDataPath().
 */
void DeinitialiseDataPath() noexcept;

/**
 * Create the primary data path;
 */
void CreateDataPath();

/**
 * Changes the primary data path. All other data paths found by
 * InitialiseDataPath() remain.
 */
void SetPrimaryDataPath(Path path) noexcept;

/**
 * Sets the data path, replacing all data paths found by
 * InitialiseDataPath().
 */
void SetSingleDataPath(Path path) noexcept;

/**
 * Returns the absolute path of the primary data directory.
 */
Path GetPrimaryDataPath() noexcept;

/**
 * Gives the position of an XCSoar data file within the particular file
 * system of this host.
 * @param file The name of the file in question. Should not be equal to 
 *             'nullptr'.
 * @return The fully qualified path of file.
 */
AllocatedPath LocalPath(Path file) noexcept;

AllocatedPath LocalPath(const TCHAR *file) noexcept;

/**
 * Create a subdirectory of XCSoarData and return its absolute path.
 */
AllocatedPath MakeLocalPath(const TCHAR *name);

/**
 * Return the portion of the specified path that is relative to the
 * primary data path. Returns nullptr on failure (if the path is not
 * inside the primary data path).
 */
[[gnu::pure]]
Path RelativePath(Path path) noexcept;

/**
 * Converts a file path by replacing %LOCAL_PATH% with the full pathname to
 * the XCSoarData folder.
 * @param src The path to convert.
 * @return The expanded path.
 */
[[gnu::pure]]
AllocatedPath ExpandLocalPath(Path src) noexcept;

/**
 * Converts a file path from full pathname to a shorter version with the
 * XCSoarData folder replaced by %LOCAL_PATH%.
 * @param src The path to convert.
 * @return The contracted path or nullptr if the given path cannot be contracted.
 */
[[gnu::pure]]
AllocatedPath ContractLocalPath(Path src) noexcept;

void VisitDataFiles(const TCHAR* filter, File::Visitor &visitor);

[[gnu::pure]]
Path GetCachePath() noexcept;

[[gnu::pure]]
AllocatedPath MakeCacheDirectory(const TCHAR *name) noexcept;

#ifdef ANDROID

/**
 * @brief Moves files from the old external files directories to the new external media directories.
 *
 * This function handles the migration of XCSoar data files on Android devices.
 * Initially, data was stored in directories returned by `Context.getExternalFilesDirs()`, 
 * but with newer Android versions and best practices, it is preferable to store these files 
 * in directories returned by `Context.getExternalMediaDirs()`. This function locates the 
 * old files, moves them to the appropriate new location, and ensures the app is compliant 
 * with modern Android storage guidelines.
 *
 * @param env The JNI environment pointer, typically obtained via `Java::GetEnv()`. 
 *            This is required to interact with Android's Java-based APIs.
 */
void MoveFilesToMediaDirs(JNIEnv* env);

#endif // ANDROID
