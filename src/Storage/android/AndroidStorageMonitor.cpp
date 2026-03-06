// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidStorageMonitor.hpp"
#include "AndroidSAFStorageDevice.hpp"
#include "Android/SAFHelper.hpp"
#include "java/Global.hxx"

AndroidStorageMonitor::AndroidStorageMonitor(SAFHelper &saf) noexcept
  : saf_(saf) {}

std::vector<std::shared_ptr<StorageDevice>>
AndroidStorageMonitor::Enumerate()
{
  std::vector<std::shared_ptr<StorageDevice>> result;

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return result;

  auto volumes = saf_.GetVolumes(env);

  for (auto &vol : volumes) {
    result.push_back(std::make_shared<AndroidSAFStorageDevice>(
        saf_,
        std::move(vol.uuid),
        std::move(vol.description),
        vol.removable,
        std::move(vol.persisted_uri)));
  }

  return result;
}
