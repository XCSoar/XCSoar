// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultiFile.hpp"
#include "ComboList.hpp"
#include "Language/Language.hpp"
#include "system/FileUtil.hpp"

#include <algorithm>

MultiFileDataField::MultiFileDataField(DataFieldListener *listener)
    : DataField(Type::MULTI_FILE, false, listener)
{
}

// Find the index of path in files
int
MultiFileDataField::Find(Path path) const
{
  return file_datafield.Find(path);
}

void
MultiFileDataField::AddInitialPath(Path text)
{
  // file_datafield must be loaded to perform a Find()
  if (file_datafield.GetNumFiles() == 0) return;

  current_selection.push_back(text);
  original_selection.push_back(text);

  UpdateDisplayString();
}

void
MultiFileDataField::Restore()
{
  current_selection.clear();

  for (const auto &path : original_selection) {
    current_selection.push_back(Path(path));
  }
}

std::vector<Path>
MultiFileDataField::GetPathFiles() const
{
  std::vector<Path> paths;

  if (current_selection.empty()) {
    paths.clear();
    return paths;
  }

  for (const auto &path : current_selection) {

    paths.push_back(path);
  }

  return paths;
}

void
MultiFileDataField::ScanMultiplePatterns(const TCHAR *patterns)
{
  file_datafield.ScanMultiplePatterns(patterns);
}

void
MultiFileDataField::AddValue(Path new_value)
{

  if (std::find(current_selection.begin(), current_selection.end(),
                new_value) != current_selection.end()) {
    return;
  }

  current_selection.emplace_back(new_value);
  UpdateDisplayString();
}

void
MultiFileDataField::UnSet(Path path)
{
  auto item =
      std::find(current_selection.begin(), current_selection.end(), path);
  if (item != current_selection.end()) {
    current_selection.erase(item); // 🚀 Removes the found item
  }

  UpdateDisplayString();
}

Path
MultiFileDataField::GetItem(unsigned index) const
{
  return file_datafield.GetItem(index).path;
}

ComboList
MultiFileDataField::CreateComboList(const TCHAR *reference) const noexcept
{
  return file_datafield.CreateComboList(reference);
}

void
MultiFileDataField::SetFromCombo(
    int datafield_index, [[maybe_unused]] const TCHAR *string_value) noexcept
{
  current_selection.emplace_back(Path(file_datafield.GetItem(datafield_index).path));
}

void
MultiFileDataField::ForceModify(Path path)
{
  file_datafield.ForceModify(path);

  current_selection.push_back(path);
}

const TCHAR *
MultiFileDataField::GetAsString() const noexcept
{
  return _T("");
}

void
MultiFileDataField::UpdateDisplayString()
{
  display_string = _T("");

  bool first = true;
  for (const auto &path : current_selection) {
    if (!first) {
      display_string += _T(" ");
    }
    first = false;

    auto index = file_datafield.Find(path);
    if (index >= 0)
      display_string += file_datafield.GetItem(index).filename.c_str();
    else
      display_string += path.c_str();
  }
}

const TCHAR *
MultiFileDataField::GetAsDisplayString() const noexcept
{
  return display_string.c_str();
}
