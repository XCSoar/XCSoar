// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageDevice.hpp"
#include "DirEntry.hpp"
#include "io/CopyStream.hxx"
#include "io/FileReader.hxx"
#include "io/FileOutputStream.hxx"
#include "system/FileUtil.hpp"

std::vector<DirEntry>
StorageDevice::ListEntries(Path relative_dir) const
{
  AllocatedPath root = RootPath();
  AllocatedPath full = (relative_dir != nullptr && !relative_dir.empty())
    ? AllocatedPath::Build(root, relative_dir)
    : std::move(root);

  return ListDirEntries(full);
}

bool
StorageDevice::IsDirectoryEntry(Path relative_path) const
{
  return Directory::Exists(AllocatedPath::Build(RootPath(), relative_path));
}

void
StorageDevice::CopyToLocal(Path remote_path, Path local_dest,
                           OperationEnvironment *env) const
{
  auto reader = OpenRead(remote_path);
  FileOutputStream writer(local_dest,
                          FileOutputStream::Mode::CREATE_VISIBLE);
  CopyStream(*reader, writer, env, reader->GetSize());
  writer.Commit();
}

bool
StorageDevice::DeleteEntry(Path relative_path)
{
  AllocatedPath full = AllocatedPath::Build(RootPath(), relative_path);
  return File::Delete(full);
}

void
StorageDevice::CopyFromLocal(Path local_src, Path remote_path,
                             OperationEnvironment *env)
{
  FileReader reader(local_src);
  AllocatedPath dest = AllocatedPath::Build(RootPath(), remote_path);
  AllocatedPath parent = dest.GetParent();
  if (parent != nullptr && !Directory::Exists(parent))
    Directory::Create(parent);
  FileOutputStream writer(dest,
                          FileOutputStream::Mode::CREATE_VISIBLE);
  CopyStream(reader, writer, env, reader.GetSize());
  writer.Commit();
}
