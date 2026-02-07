// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowsStorageMonitor.hpp"
#include "WindowsStorageDevice.hpp"
#include "util/ConvertString.hpp"

#include <cctype>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

std::vector<std::shared_ptr<StorageDevice>>
WindowsStorageMonitor::enumerate()
{
  std::vector<std::shared_ptr<StorageDevice>> result;

  // Determine system drive (usually "C:\\", but don't assume)
  wchar_t windows_dir[MAX_PATH];
  char system_drive = 0;
  if (GetWindowsDirectoryW(windows_dir, MAX_PATH) > 0) {
    system_drive = static_cast<char>(std::towupper(windows_dir[0]));
  }

  const DWORD drives = GetLogicalDrives();
  for (char d = 'A'; d <= 'Z'; ++d) {
    if (!(drives & (1u << (d - 'A'))))
      continue;

    // Root path must include trailing backslash to be absolute
    char root_for_api[4] = { d, ':', '\\', '\0' };

    const UINT type = GetDriveTypeA(root_for_api);

    // Skip drives that are not usable: no root, remote/network, or CD-ROM
    if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_REMOTE || type == DRIVE_CDROM)
      continue;
    
    // Exclude system drive (case-insensitive)
    if (system_drive != 0 && system_drive == static_cast<char>(std::toupper(static_cast<unsigned char>(d))))
      continue;

    // Accept removable media and fixed drives (e.g. USB HDDs)
    if (type != DRIVE_REMOVABLE && type != DRIVE_FIXED)
      continue;

    UTF8ToWideConverter conv(root_for_api);
    if (!conv.IsValid())
      continue;

    result.push_back(std::make_shared<WindowsStorageDevice>(AllocatedPath(conv.c_str())));
  }

  return result;
}
