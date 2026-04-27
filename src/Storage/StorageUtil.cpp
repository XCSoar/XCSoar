// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StorageUtil.hpp"
#include "StorageDevice.hpp"
#include "LocalFilesystemDevice.hpp"
#include "StorageManager.hpp"
#include "util/StringCompare.hxx"
#include "util/UriUtil.hpp"
#include "system/FileUtil.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

bool
IsContentUri(Path path) noexcept
{
  if (path == nullptr)
    return false;

  const char *s = path.c_str();
  return s != nullptr && StringStartsWith(s, "content://");
}

std::string
ExtractSafSubfolder(const std::string &uri)
{
  const std::size_t tree_pos = uri.find("/tree/");
  if (tree_pos == std::string::npos)
    return {};

  const std::size_t sep = uri.find("%3A", tree_pos + 6);
  if (sep == std::string::npos)
    return {};

  const std::size_t start = sep + 3;
  std::size_t end = uri.find('/', start);
  if (end == std::string::npos)
    end = uri.size();

  if (start >= end)
    return {};

  std::string result = uri.substr(start, end - start);

  PercentDecode(result);

  return result;
}

std::string
FormatStorageCaption(Path path)
{
  if (!IsContentUri(path))
    return path.c_str();

  const std::string s = path.c_str();
  const std::string subfolder = ExtractSafSubfolder(s);

  /* Try to find the matching device and use its human-readable label
     (e.g. "SD card") instead of the raw volume id. */
  if (backend_components != nullptr &&
      backend_components->storage_manager != nullptr) {
    for (const auto &dev :
         backend_components->storage_manager->GetDevices()) {
      if (dev == nullptr || !dev->IsWritable())
        continue;
      if (dev->Name() == s) {
        std::string label = dev->Label();
        if (!subfolder.empty()) {
          label += ':';
          label += subfolder;
        }
        return label;
      }
    }
  }

  /* Fallback: extract the volume id from the content URI. */
  std::string result{"disk://"};

  const std::size_t tree_pos = s.find("/tree/");
  if (tree_pos == std::string::npos)
    return result;

  std::size_t start = tree_pos + 6;
  std::size_t end = s.find("%3A", start);
  if (end == std::string::npos)
    end = s.find('/', start);
  if (end == std::string::npos)
    end = s.size();

  if (end > start)
    result.append(s, start, end - start);

  if (!subfolder.empty()) {
    result += ':';
    result += subfolder;
  }

  return result;
}

bool
IsSafDeviceId(const std::string &id) noexcept
{
  return StringStartsWith(id.c_str(), "saf:");
}

std::shared_ptr<StorageDevice>
FindDeviceByName(Path name) noexcept
{
  if (name == nullptr)
    return nullptr;

  if (backend_components != nullptr &&
      backend_components->storage_manager != nullptr) {
    const std::string needle = name.c_str();
    for (auto &dev :
         backend_components->storage_manager->GetDevices()) {
      if (dev == nullptr || !dev->IsWritable())
        continue;

      if (dev->Name() == needle)
        return dev;
    }
  }

  /* No platform device matched — wrap local filesystem path so
     callers can use the StorageDevice interface uniformly. */
  if (!IsContentUri(name))
    return std::make_shared<LocalFilesystemDevice>(name);

  return nullptr;
}
