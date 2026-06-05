// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"

#include <memory>
#include <string>

struct _JNIEnv;
typedef _JNIEnv JNIEnv;

class SAFHelper;

/**
 * A #StorageDevice backed by Android's Storage Access Framework.
 *
 * The "path" provided to OpenRead/OpenWrite is a display name path
 * (e.g. "XCSoarData/default.prf") that is resolved via the SAF
 * document tree — the UI never sees a real file-system path.
 */
class AndroidSAFStorageDevice : public StorageDevice {
  /** Reference to the shared SAFHelper (not owned). */
  SAFHelper &saf_;

  /** Volume UUID (or "primary" for internal). */
  std::string uuid_;

  /** Human-visible label (e.g. "SD card", "USB drive"). */
  std::string description_;

  /** Whether this is a removable volume (SD, USB). */
  bool removable_;

  /** Persisted tree URI — empty until SAF permission is granted. */
  std::string tree_uri_;

public:
  AndroidSAFStorageDevice(SAFHelper &saf,
                          std::string uuid,
                          std::string description,
                          bool removable,
                          std::string tree_uri) noexcept;

  /** Update the tree URI (e.g. after the user grants a new permission). */
  void SetTreeUri(std::string uri) noexcept { tree_uri_ = std::move(uri); }

  /** Return the persisted tree URI (may be empty). */
  const std::string &GetTreeUri() const noexcept { return tree_uri_; }

  /** Return the volume UUID. */
  const std::string &GetUuid() const noexcept { return uuid_; }

  // StorageDevice overrides
  std::string Id() const override;
  std::string Name() const override;
  std::string Label() const override;
  Kind GetKind() const override;
  bool IsWritable() const override;
  bool NeedsPermission() const noexcept override;
  void RequestPermission() noexcept override;

  std::unique_ptr<Reader>
  OpenRead(Path path) const override;

  std::unique_ptr<OutputStream>
  OpenWrite(Path path, bool truncate = true) override;

  void CopyFromLocal(Path local_src, Path remote_path,
                     OperationEnvironment *env = nullptr) override;

  bool DeleteEntry(Path relative_path) override;

  std::vector<DirEntry>
  ListEntries(Path relative_dir) const override;

  bool IsDirectoryEntry(Path relative_path) const override;

  std::optional<Space> GetSpace() const noexcept override;

private:
  /** Check tree URI and JNI availability; throw on failure. */
  JNIEnv *RequireEnv() const;

  /** Return a JNI environment if tree URI is set, nullptr otherwise. */
  JNIEnv *GetEnvIfReady() const noexcept;
};
