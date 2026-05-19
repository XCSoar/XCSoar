// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DirEntry.hpp"
#include "system/Path.hpp"

#include <memory>
#include <string>
#include <vector>

class Reader;
class OutputStream;
class OperationEnvironment;

class StorageDevice {
public:
  enum class Kind {
    Internal,
    Removable,
    COUNT
  };

  virtual ~StorageDevice() = default;

  /* stable identifier */
  virtual std::string Id() const = 0;
  virtual std::string Name() const = 0;
  virtual std::string Label() const { 
    return Name(); 
  }
  virtual Kind GetKind() const = 0;

  virtual bool IsWritable() const = 0;

  /**
   * Open a file inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<Reader>
  OpenRead(Path path) const = 0;

  /**
   * Open a file for writing inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<OutputStream>
  OpenWrite(Path path, bool truncate = true) = 0;

  /**
   * List entries (files and directories) in a directory relative
   * to the device root.
   *
   * The default implementation treats Name() as a local filesystem
   * path.  Subclasses where Name() is not a filesystem path (e.g.
   * content URIs) must override this method.
   *
   * @param relative_dir relative directory path ("" for root)
   */
  virtual std::vector<DirEntry>
  ListEntries(Path relative_dir) const;

  /**
   * Check whether a relative path inside the device is a directory.
   *
   * The default implementation treats Name() as a local filesystem
   * path.  Subclasses where Name() is not a filesystem path (e.g.
   * content URIs) must override this method.
   */
  virtual bool IsDirectoryEntry(Path relative_path) const;

  /**
   * Copy a file from this storage device to a local filesystem path.
   *
   * @param remote_path path relative to the device root
   * @param local_dest absolute local filesystem path
   */
  virtual void CopyToLocal(Path remote_path, Path local_dest,
                           OperationEnvironment *env = nullptr) const;

  /**
   * Copy a local file to this storage device.
   *
   * @param local_src absolute local filesystem path
   * @param remote_path path relative to the device root
   * @param env optional environment for cancellation support
   */
  virtual void CopyFromLocal(Path local_src, Path remote_path,
                             OperationEnvironment *env = nullptr);

  /**
   * Filesystem/device space information (total and free bytes).
   * Returns empty optional if the information is not available for this
   * device (e.g. device removed or not mounted).
   */
  struct Space {
    uint64_t total_bytes;
    uint64_t free_bytes; /* available to unprivileged users */
  };

  /**
   * Delete a file or directory inside the storage device.
   *
   * @param relative_path path relative to the device root
   * @return true if successfully deleted
   */
  virtual bool DeleteEntry(Path relative_path);

  virtual std::optional<Space> GetSpace() const noexcept {
    return std::nullopt;
  }

  /**
   * Does this device require an explicit permission grant before
   * it can be written to?  (e.g. Android SAF volumes without a
   * persisted tree URI.)
   */
  virtual bool NeedsPermission() const noexcept {
    return false;
  }

protected:
  /**
   * Return the device root as a filesystem path.
   * Used by the default ListEntries()/IsDirectoryEntry() implementations.
   * Subclasses that store the root as an AllocatedPath should override
   * to avoid the Name()→string→Path round-trip.
   */
  virtual AllocatedPath RootPath() const {
    return AllocatedPath(Name().c_str());
  }

public:
  /**
   * Request the OS-level permission needed to access this volume.
   * This may block the calling thread while a system dialog is shown.
   *
   * The default implementation does nothing.  Platform-specific
   * subclasses override this to trigger platform-specific flows
   * (e.g. Android SAF tree picker).
   */
  virtual void RequestPermission() noexcept {}
};
