// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"
#include "java/Class.hxx"
#include "java/Ref.hxx"
#include "Storage/StorageDevice.hpp"
#include "Storage/DirEntry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Context;

/**
 * JNI bridge to the Java SAFHelper class.
 *
 * Provides access to Android's Storage Access Framework for
 * enumerating volumes, opening documents, and managing persisted
 * tree-URI permissions.
 *
 * The UI never sees real file-system paths; only display-name paths
 * (e.g. "XCSoarData/default.prf") are used.
 */
class SAFHelper final : protected Java::GlobalObject {
public:
  struct VolumeInfo {
    std::string uuid;
    std::string description;
    bool removable;
    /** persisted tree URI for this volume (empty if none) */
    std::string persisted_uri;
  };

  static bool Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  SAFHelper(JNIEnv *env, Context &context);

  /**
   * Enumerate available storage volumes.
   */
  std::vector<VolumeInfo> GetVolumes(JNIEnv *env) const;

  /**
   * Persist a tree URI (call after the activity receives the result
   * from the ACTION_OPEN_DOCUMENT_TREE intent).
   */
  bool PersistTreePermission(JNIEnv *env, const char *tree_uri) const;

  /**
   * Open a file for reading inside a SAF tree.
   *
   * @return a Java InputStream local ref, or nullptr on failure.
   *         The caller must close it.
   */
  Java::LocalRef<jobject> OpenRead(JNIEnv *env,
                                   const char *tree_uri,
                                   const char *display_path) const;

  /**
   * Open (or create) a file for writing inside a SAF tree.
   *
   * @return a Java OutputStream local ref, or nullptr on failure.
   *         The caller must close it.
   */
  Java::LocalRef<jobject> OpenWrite(JNIEnv *env,
                                    const char *tree_uri,
                                    const char *display_path,
                                    bool truncate) const;

  /**
   * Query free/total space for the volume behind a tree URI.
   */
  std::optional<StorageDevice::Space> GetSpace(JNIEnv *env,
                                               const char *tree_uri) const;

  /**
   * List children with full metadata (name, type, size, mtime)
   * in a single cursor query, avoiding per-child round-trips.
   */
  std::vector<DirEntry> ListChildren(JNIEnv *env,
                                     const char *tree_uri,
                                     const char *display_path) const;

  /**
   * Check whether a path inside a SAF tree is a directory.
   */
  bool IsDirectory(JNIEnv *env,
                   const char *tree_uri,
                   const char *display_path) const;

  /**
   * Delete a document (file or directory) inside a SAF tree.
   *
   * @return true if the document was deleted successfully
   */
  bool DeleteDocument(JNIEnv *env,
                      const char *tree_uri,
                      const char *display_path) const;

  /**
   * Build an ACTION_OPEN_DOCUMENT_TREE intent string suitable for
   * launching via Activity.startActivityForResult().
   *
   * @return a Java Intent local ref, or nullptr on failure.
   */
  Java::LocalRef<jobject> BuildOpenTreeIntent(JNIEnv *env,
                                              const char *volume_uuid) const;
};
