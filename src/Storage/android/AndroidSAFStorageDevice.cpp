// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidSAFStorageDevice.hpp"
#include "SAFReader.hpp"
#include "SAFOutputStream.hpp"
#include "Android/SAFHelper.hpp"
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#include "io/FileReader.hxx"
#include "io/CopyStream.hxx"
#include "java/Global.hxx"
#include "java/Ref.hxx"
#include "system/Path.hpp"

#include <stdexcept>

AndroidSAFStorageDevice::AndroidSAFStorageDevice(
    SAFHelper &saf,
    std::string uuid,
    std::string description,
    bool removable,
    std::string tree_uri) noexcept
  : saf_(saf),
    uuid_(std::move(uuid)),
    description_(std::move(description)),
    removable_(removable),
    tree_uri_(std::move(tree_uri)) {}

JNIEnv *
AndroidSAFStorageDevice::RequireEnv() const
{
  if (tree_uri_.empty())
    throw std::runtime_error("No SAF permission for volume " + uuid_);

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    throw std::runtime_error("No JNI environment");

  return env;
}

JNIEnv *
AndroidSAFStorageDevice::GetEnvIfReady() const noexcept
{
  if (tree_uri_.empty())
    return nullptr;
  return Java::GetEnv();
}

std::string
AndroidSAFStorageDevice::Id() const
{
  return "saf:" + uuid_;
}

std::string
AndroidSAFStorageDevice::Name() const
{
  /* When we have a persisted tree URI, return it as the "name" —
     the dialog uses Name() as the mount/path identifier. */
  if (!tree_uri_.empty())
    return tree_uri_;
  return description_;
}

std::string
AndroidSAFStorageDevice::Label() const
{
  return description_;
}

StorageDevice::Kind
AndroidSAFStorageDevice::GetKind() const
{
  return removable_ ? Kind::Removable : Kind::Internal;
}

bool
AndroidSAFStorageDevice::IsWritable() const
{
  // Writable only if we have a persisted tree URI.
  return !tree_uri_.empty();
}

bool
AndroidSAFStorageDevice::NeedsPermission() const noexcept
{
  return tree_uri_.empty();
}

void
AndroidSAFStorageDevice::RequestPermission() noexcept
{
  if (native_view == nullptr || uuid_.empty())
    return;

  JNIEnv *env = Java::GetEnv();
  if (env != nullptr)
    native_view->LaunchSAFTreePicker(env, uuid_.c_str());
}

std::unique_ptr<Reader>
AndroidSAFStorageDevice::OpenRead(Path path) const
{
  JNIEnv *env = RequireEnv();

  auto is = saf_.OpenRead(env, tree_uri_.c_str(), path.c_str());
  if (!is)
    throw std::runtime_error(std::string("SAF: cannot open for reading: ") +
                             path.c_str());

  return std::make_unique<SAFReader>(env, is.Get());
}

std::unique_ptr<OutputStream>
AndroidSAFStorageDevice::OpenWrite(Path path, bool truncate)
{
  JNIEnv *env = RequireEnv();

  auto os = saf_.OpenWrite(env, tree_uri_.c_str(), path.c_str(), truncate);
  if (!os)
    throw std::runtime_error(std::string("SAF: cannot open for writing: ") +
                             path.c_str());

  return std::make_unique<SAFOutputStream>(env, os.Get());
}

void
AndroidSAFStorageDevice::CopyFromLocal(Path local_src, Path remote_path,
                                       OperationEnvironment *env)
{
  FileReader reader(local_src);
  auto writer = OpenWrite(remote_path, true);
  CopyStream(reader, *writer, env, reader.GetSize());
  writer->Commit();
}

bool
AndroidSAFStorageDevice::DeleteEntry(Path relative_path)
{
  JNIEnv *env = GetEnvIfReady();
  if (env == nullptr)
    return false;

  return saf_.DeleteDocument(env, tree_uri_.c_str(), relative_path.c_str());
}

std::vector<DirEntry>
AndroidSAFStorageDevice::ListEntries(Path relative_dir) const
{
  JNIEnv *env = GetEnvIfReady();
  if (env == nullptr)
    return {};

  return saf_.ListChildren(env, tree_uri_.c_str(), relative_dir.c_str());
}

bool
AndroidSAFStorageDevice::IsDirectoryEntry(Path relative_path) const
{
  JNIEnv *env = GetEnvIfReady();
  if (env == nullptr)
    return false;

  return saf_.IsDirectory(env, tree_uri_.c_str(), relative_path.c_str());
}

std::optional<StorageDevice::Space>
AndroidSAFStorageDevice::GetSpace() const noexcept
{
  JNIEnv *env = GetEnvIfReady();
  if (env == nullptr)
    return std::nullopt;

  return saf_.GetSpace(env, tree_uri_.c_str());
}
