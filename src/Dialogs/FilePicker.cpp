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

#include "FilePicker.hpp"
#include "ComboPicker.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Language/Language.hpp"
#include "Net/HTTP/Features.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Net/HTTP/DownloadManager.hpp"
#include "DownloadFilePicker.hpp"
#endif

bool
FilePicker(const TCHAR *caption, FileDataField &df,
           const TCHAR *help_text)
{
  ComboList combo_list = df.CreateComboList(nullptr);
  if (combo_list.size() == 0)
    return false;

  const TCHAR *extra_caption = nullptr;
#ifdef HAVE_DOWNLOAD_MANAGER
  if (df.GetFileType() != FileType::UNKNOWN &&
      Net::DownloadManager::IsAvailable())
    extra_caption = _("Download");
#endif

  int i = ComboPicker(caption, combo_list, help_text, false, extra_caption);

#ifdef HAVE_DOWNLOAD_MANAGER
  if (i == -2) {
    const auto path = DownloadFilePicker(df.GetFileType());
    if (path.IsNull())
      return false;

    df.ForceModify(path);
    return true;
  }
#endif

  if (i < 0)
    return false;

  const ComboList::Item &item = combo_list[i];
  df.SetFromCombo(item.int_value, item.string_value.c_str());
  return true;
}

AllocatedPath
FilePicker(const TCHAR *caption, const TCHAR *patterns)
{
  assert(patterns != nullptr);

  FileDataField df;
  df.ScanMultiplePatterns(patterns);
  return FilePicker(caption, df)
    ? df.GetPathFile()
    : nullptr;
}
