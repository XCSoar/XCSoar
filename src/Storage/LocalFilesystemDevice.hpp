// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StorageDevice.hpp"
#include "io/CopyFile.hxx"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"

#include <stdexcept>

/**
 * A StorageDevice backed by a regular local filesystem directory.
 *
 * This allows dialog code to treat local and remote (SAF) storage
 * uniformly through the StorageDevice interface, eliminating
 * if-device/else branches.
 */
class LocalFilesystemDevice final : public StorageDevice {
  AllocatedPath root_;

public:
  explicit LocalFilesystemDevice(Path root) noexcept
    : root_(root) {}

  std::string Id() const override { return root_.c_str(); }
  std::string Name() const override { return root_.c_str(); }
  Kind GetKind() const override { return Kind::Internal; }

protected:
  AllocatedPath RootPath() const override { return AllocatedPath{Path{root_}}; }

public:
  bool IsWritable() const override { return true; }

  std::unique_ptr<Reader>
  OpenRead(Path path) const override {
    return std::make_unique<FileReader>(
      AllocatedPath::Build(root_, path));
  }

  std::unique_ptr<OutputStream>
  OpenWrite(Path path, bool truncate) override {
    return std::make_unique<FileOutputStream>(
      AllocatedPath::Build(root_, path),
      truncate
        ? FileOutputStream::Mode::CREATE_VISIBLE
        : FileOutputStream::Mode::APPEND_OR_CREATE);
  }

  /**
   * Override: use efficient platform copy instead of stream copy.
   */
  void CopyToLocal(Path remote_path, Path local_dest,
                   OperationEnvironment *env = nullptr) const override {
    (void)env;
    CopyFile(AllocatedPath::Build(root_, remote_path), local_dest);
  }

  /**
   * Override: use efficient move-or-copy instead of stream copy.
   * Ensures the parent directory exists first.
   */
  void CopyFromLocal(Path local_src, Path remote_path,
                     OperationEnvironment *env = nullptr) override {
    (void)env;
    AllocatedPath dest = AllocatedPath::Build(root_, remote_path);
    AllocatedPath parent = dest.GetParent();
    if (parent != nullptr && !Directory::Exists(parent))
      Directory::Create(parent);
    MoveOrCopyFile(local_src, dest);
  }
};
