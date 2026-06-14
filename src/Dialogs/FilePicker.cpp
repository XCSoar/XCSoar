// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FilePicker.hpp"
#include "ComboPicker.hpp"
#include "Repository/Glue.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Form/DataField/File.hpp"
#include "Form/Form.hpp"
#include "Language/Language.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "DownloadFilePicker.hpp"
#endif

bool
FilePicker(const char *caption, FileDataField &df, const char *help_text,
           bool nullable)
{
  ComboList combo_list = df.CreateComboList(nullptr);

  const char *extra_caption = nullptr;
#ifdef HAVE_DOWNLOAD_MANAGER
  const auto file_type = df.GetFileType();
  if (FileTypeSupportsDownload(file_type))
    extra_caption = _("Download");
#endif

  if (combo_list.size() == 0 && nullable && extra_caption == nullptr)
    return false;

  const int i = ComboPicker(caption, combo_list, help_text, false,
                            extra_caption);

#ifdef HAVE_DOWNLOAD_MANAGER
  if (i == mrExtra) {
    const auto path = DownloadFilePicker(file_type);
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
