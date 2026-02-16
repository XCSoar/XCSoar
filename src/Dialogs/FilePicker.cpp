// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FilePicker.hpp"
#include "ComboPicker.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/File.hpp"
#include "Language/Language.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "DownloadFilePicker.hpp"
#include "net/http/DownloadManager.hpp"
#endif

bool
FilePicker(const char *caption, FileDataField &df, const char *help_text,
           bool nullable)
{
  ComboList combo_list = df.CreateComboList(nullptr);
  // if nullable it's mean that combo_list must have at least a null element
  if (combo_list.size() == 0 && nullable) return false;

  const char *extra_caption = nullptr;
#ifdef HAVE_DOWNLOAD_MANAGER
  // with FileType::IGC don't show the 'Download'-Button!
  if (df.GetFileType() != FileType::IGC &&
    df.GetFileType() != FileType::UNKNOWN &&
    Net::DownloadManager::IsAvailable())
      extra_caption = _("Download");
#endif

  int i = ComboPicker(caption, combo_list, help_text, false, extra_caption);

#ifdef HAVE_DOWNLOAD_MANAGER
  if (i == -2) {
    const auto path = DownloadFilePicker(df.GetFileType());
    if (path == nullptr)
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
FilePicker(const char *caption, const char *patterns)
{
  assert(patterns != nullptr);

  FileDataField df;
  df.ScanMultiplePatterns(patterns);
  return FilePicker(caption, df)
    ? df.GetValue()
    : nullptr;
}
