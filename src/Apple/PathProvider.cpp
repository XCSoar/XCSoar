// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Apple/PathProvider.hpp"
#include "ProductName.hpp"
#include "Asset.hpp"
#include "system/FileUtil.hpp"

#import <Foundation/Foundation.h>

namespace Apple {

/**
* Get the data path within the home directory.
* On iOS, returns "Documents/PRODUCT_DATA_DIR", otherwise returns "PRODUCT_DATA_DIR".
* @return Path object pointing to the data directory
*/
Path
GetDataPathInHome() noexcept
{
  return IsIOS()
    ? Path("Documents/" PRODUCT_DATA_DIR)
    : Path(PRODUCT_DATA_DIR);
}

/**
 * Ensure the specified data path exists by creating the directory
 * and any intermediate directories if they don't already exist.
 *
 * @param path The directory path to ensure exists.
 * @return true if the directory exists or was created successfully.
 */
bool
EnsureDataPathExists(Path path) noexcept
{
  if (path == nullptr || path.empty())
    return false;

  if (Directory::Exists(path))
    return true;

  const std::string utf8_path = path.ToUTF8();
  if (utf8_path.empty())
    return false;

  NSString *path_str =
    [NSString stringWithCString:utf8_path.c_str()
                       encoding:NSUTF8StringEncoding];
  if (path_str == nil)
    return false;

  NSFileManager *file_manager = [NSFileManager defaultManager];
  NSError *error = nil;
  const BOOL ok =
    [file_manager createDirectoryAtPath:path_str
            withIntermediateDirectories:YES
                             attributes:nil
                                  error:&error];
  return ok;
}

} // namespace Apple
