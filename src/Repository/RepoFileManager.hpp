/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Repository/FileRepository.hpp"

#include <map>

class Path;

/**
 * Wraps a FileRepository to allow checking for updates.
 * Maintains a `local_hashes` file that holds the sha256
 * sum of downloaded files.
 */
class RepoFileManager {
  std::map<std::string, std::array<std::byte, 32>> local_hashes;

  static const char *LOCAL_HASHES_FILE = "local_hashes";

public:
  FileRepository repository;

  /**
   * Loads a repository file.
   * @param repofilepath The path to the repository file.
   */
  void Load(Path repofilepath);

  /**
   * Writes the updated hashes to the `local_hashes` file.
   */
  void Close();

  /**
   * Downloads a file from the repository.
   * @param name The filename of the file to download.
   */
  void Download(const char *name);

  /**
   * Checks if a local file has a newer version in the repository.
   * If hashes are available they are compared, if not the file
   * modification timestamp is older than the `update` field in
   * the repository file.
   */
  bool IsOutOfDate(const char *name);

private:
  void LoadLocalHashes();
  void SaveLocalHashes();
};