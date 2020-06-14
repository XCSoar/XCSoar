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

#include "Repository/RepoFileManager.hpp"
#include "IO/BufferedOutputStream.hxx"
#include "IO/FileOutputStream.hxx"
#include "IO/KeyValueFileReader.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "LocalPath.hpp"
#include "IO/FileLineReader.hpp"
#include "Net/HTTP/DownloadManager.hpp"
#include "OS/FileUtil.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Parser.hpp"
#include "Time/BrokenDate.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Util/ConvertString.hpp"
#include "Util/HexString.hpp"

void
RepoFileManager::Load(Path repofilepath)
{
  repository.Clear();

  try {
    FileLineReaderA reader(repofilepath);
    bool success = ParseFileRepository(repository, reader);

    if (!success) {
      repository.Clear();
    }
  } catch (...) {
    repository.Clear();
    return;
  }

  LoadLocalHashes();
}

void
RepoFileManager::Download(const char *name)
{
  const AvailableFile *remote_file = repository.FindByName(name);
  if (remote_file == nullptr)
    return;

  const UTF8ToWideConverter base(remote_file->GetName());
  if (!base.IsValid())
    return;

  Net::DownloadManager::Enqueue(remote_file->GetURI(), Path(base));

  if (remote_file->HasHash()) {
    local_hashes[remote_file->name] = remote_file->sha256_hash;
  }
}

bool
RepoFileManager::IsOutOfDate(const char *name)
{
  const AvailableFile *remote_file = repository.FindByName(name);

  if (remote_file == nullptr)
    return false;

  if (remote_file->HasHash()) {
    try {
      std::array<std::byte, 32> local_hash = local_hashes.at(name);

      return local_hash != remote_file->sha256_hash;
    } catch (std::exception &e) {
      // No hash of the local version was found so mark as out of date.
      return true;
    }
  }

#ifdef HAVE_POSIX
  BrokenDate remote_changed = remote_file->update_date;

  const UTF8ToWideConverter base(remote_file->GetName());
  if (!base.IsValid())
    return false;

  BrokenDate local_changed = static_cast<BrokenDate>(BrokenDateTime::FromUnixTimeUTC(
        File::GetLastModification(LocalPath(base))));

  return local_changed < remote_changed;
#else
  return false;
#endif
}


/**
 * Reads the hashes of files from the repo from the disk.
 * Every line is an entry of the form `filename = hash`.
 * This is done so we don't have to calculate the
 * checksum of the local files on every start.
 */
void
RepoFileManager::LoadLocalHashes()
{
  try {
    FileLineReaderA linereader(LocalPath(LOCAL_HASHES_FILE));
    KeyValueFileReader kvreader(linereader);

    KeyValuePair kv_pair;
    while (kvreader.Read(kv_pair)) {
      std::string name = kv_pair.key;
      try {
        std::array<std::byte, 32> sha256 = ParseHexString<32>(kv_pair.value);

        local_hashes[name] = sha256;
      } catch (std::exception &e) {
        // invalid hash in the hash file don't add anything
      }
    }
  } catch (...) {
    local_hashes.clear();
  }
}

/**
 * Writes the hashes of the files from the repo to the disk.
 */
void
RepoFileManager::SaveLocalHashes()
{
  FileOutputStream file_os(LocalPath(LOCAL_HASHES_FILE),
                           FileOutputStream::Mode::CREATE);
  BufferedOutputStream buf_os(file_os);
  KeyValueFileWriter kvwriter(buf_os);

  for (const auto &pair : local_hashes) {
    const char *name = pair.first.c_str();
    const char *hash = RenderHexString(pair.second).c_str();
    kvwriter.Write(name, hash);
  }

  buf_os.Flush();
  file_os.Commit();
}

void
RepoFileManager::Close()
{
  SaveLocalHashes();
  local_hashes.clear();
}
