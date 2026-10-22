// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LoadWaypointImages.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"
#include "io/CupxArchive.hpp"
#include "io/FileOutputStream.hxx"
#include "system/ConvertPathName.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"

#include <exception>
#include <span>

unsigned
LoadWaypointEmbeddedImages(StaticArray<Bitmap, 5> &images,
                           const Waypoint &waypoint,
                           Path source_path) noexcept
{
  const unsigned before = images.size();
  const bool is_cupx = source_path != nullptr &&
    source_path.EndsWithIgnoreCase(".cupx");

  for (const auto &i : waypoint.files_embed) {
    if (images.full())
      break;

    try {
      if (is_cupx) {
        auto data = CupxArchive::ExtractImage(source_path, i);
        if (!data.empty()) {
#ifndef ANDROID
          if (!images.append().Load(std::span<const std::byte>(data)))
            images.shrink(images.size() - 1);
#else
          const auto tmp_dir = MakeCacheDirectory("cupx");
          const auto tmp_file = AllocatedPath::Build(tmp_dir, i.c_str());

          FileOutputStream fos(tmp_file,
                               FileOutputStream::Mode::CREATE_VISIBLE);
          fos.Write(std::as_bytes(std::span{data}));
          fos.Commit();

          if (!images.append().LoadFile(tmp_file))
            images.shrink(images.size() - 1);

          File::Delete(tmp_file);
#endif
          continue;
        }

        /* Not in the archive.  A waypoint details file may name its
           own pictures ("image=AIP/ETSI_1.png") for a waypoint that
           came from a .cupx, and those live in the data directory,
           not in the archive's pics.zip. */
      }

      if (!images.append().LoadFile(LocalPath(i.c_str())))
        images.shrink(images.size() - 1);
    } catch (const std::exception &e) {
      LogFormat("Failed to load %s: %s",
                (const char *)NarrowPathName(Path(i.c_str())),
                e.what());
      images.shrink(images.size() - 1);
    }
  }

  return images.size() - before;
}
