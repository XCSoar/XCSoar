// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "File.hpp"
#include "ComboList.hpp"
#include "LocalPath.hpp"
#include "util/StringAPI.hxx"
#include "system/FileUtil.hpp"

#include <algorithm>

#include <windef.h> /* for MAX_PATH */
#include <cassert>
#include <stdlib.h>

/**
 * Checks whether the given string str equals a xcsoar internal file's filename
 * @param str The string to check
 * @return True if string equals a xcsoar internal file's filename
 */
[[gnu::pure]]
static bool
IsInternalFile(const char *str) noexcept
{
  static const char *const ifiles[] = {
    "xcsoar-checklist.txt",
    "xcsoar-checklist.xcc",
    "xcsoar-flarm.txt",
    "xcsoar-marks.txt",
    "xcsoar-persist.log",
    "xcsoar-startup.log",
    "xcsoar.log",
    "xcsoar-rasp.dat",
    "user.cup",
    nullptr
  };

  for (unsigned i = 0; ifiles[i] != nullptr; i++)
    if (StringIsEqual(str, ifiles[i]))
      return true;

  return false;
}

class FileVisitor: public File::Visitor
{
private:
  FileDataField &datafield;

public:
  explicit FileVisitor(FileDataField &_datafield) noexcept
    : datafield(_datafield) {}

  void Visit(Path path, Path filename) override {
    bool skip = IsInternalFile(filename.c_str());
    if (skip && datafield.GetFileType() == FileType::CHECKLIST &&
        StringIsEqual(filename.c_str(), "xcsoar-checklist.txt"))
      skip = false;
    if (!skip)
      datafield.AddFile(path);
  }
};

inline void
FileDataField::Item::Set(Path _path) noexcept
{
  path = _path;
  filename = path.GetBase();
  if (filename == nullptr)
    filename = path;
}

FileDataField::FileDataField(DataFieldListener *listener) noexcept
  :DataField(Type::FILE, true, listener),
   // Set selection to zero
   current_index(0),
   loaded(false), postponed_sort(false),
   postponed_value(nullptr) {}

void
FileDataField::ScanDirectoryTop(const char *filter) noexcept
{
  if (!loaded) {
    if (!postponed_patterns.full() &&
        strlen(filter) < PatternList::value_type().capacity()) {
      postponed_patterns.append() = filter;
      return;
    } else
      EnsureLoaded();
  }

  FileVisitor fv(*this);
  VisitDataFiles(filter, fv);

  Sort();
}

void
FileDataField::ScanMultiplePatterns(const char *patterns) noexcept
{
  size_t length;
  while ((length = strlen(patterns)) > 0) {
    ScanDirectoryTop(patterns);
    patterns += length + 1;
  }
}

int
FileDataField::Find(Path path) const noexcept
{
  for (unsigned i = 0, n = files.size(); i < n; i++)
    if (files[i].path == path)
      return i;

  return -1;
}

void
FileDataField::SetValue(Path text) noexcept
{
  if (!loaded) {
    postponed_value = text;
    return;
  }

  auto i = Find(text);
  if (i >= 0)
    current_index = i;
}

void
FileDataField::ModifyValue(Path new_value) noexcept
{
  if (new_value == GetValue())
    return;

  if (!loaded) {
    postponed_value = new_value;
    Modified();
    return;
  }

  auto i = Find(new_value);
  if (i >= 0) {
    current_index = i;
    Modified();
  }
}

void
FileDataField::ForceModify(Path path) noexcept
{
  EnsureLoaded();

  auto i = Find(path);
  if (i >= 0) {
    if (unsigned(i) == current_index)
      return;
  } else {
    auto &item = files.full() ? files.back() : files.append();
    item.Set(path);
    i = files.size() - 1;
  }

  current_index = i;
  Modified();
}

unsigned
FileDataField::GetNumFiles() const noexcept
{
  EnsureLoadedDeconst();

  return files.size();
}

Path
FileDataField::GetValue() const noexcept
{
  if (!loaded && postponed_value != nullptr)
    return postponed_value;

  if (current_index >= files.size())
    // TODO: return nullptr instead of empty string?
    return Path("");

  const Path path = files[current_index].path;
  assert(path != nullptr);
  return path;
}

void
FileDataField::AddFile(Path path) noexcept
{
  assert(loaded);

  // TODO enhancement: remove duplicates?

  // if too many files -> cancel
  if (files.full())
    return;

  Item &item = files.append();
  item.Set(path);
}

void
FileDataField::AddNull() noexcept
{
  assert(!files.full());

  Item &item = files.append();
  item.filename = Path("");
  item.path = Path("");
}

const char *
FileDataField::GetAsString() const noexcept
{
  if (!loaded && postponed_value != nullptr)
    return postponed_value.c_str();

  if (current_index < files.size())
    return files[current_index].path.c_str();
  else
    return "";
}

const char *
FileDataField::GetAsDisplayString() const noexcept
{
  if (!loaded && postponed_value != nullptr) {
    /* get basename from postponed_value */
    auto p = postponed_value.GetBase();
    if (p == nullptr)
      p = postponed_value;

    return p.c_str();
  }

  if (current_index < files.size())
    return files[current_index].filename.c_str();
  else
    return "";
}

void
FileDataField::SetIndex(unsigned new_value) noexcept
{
  if (new_value > 0)
    EnsureLoaded();
  else
    postponed_value = nullptr;

  if (new_value < files.size())
    current_index = new_value;
}

void
FileDataField::ModifyIndex(unsigned new_value) noexcept
{
  if (new_value > 0)
    EnsureLoaded();
  else
    postponed_value = nullptr;

  if (new_value == current_index)
    return;

  if (new_value < files.size()) {
    current_index = new_value;
    Modified();
  }
}

void
FileDataField::Inc() noexcept
{
  EnsureLoaded();

  if (current_index < files.size() - 1) {
    current_index++;
    Modified();
  }
}

void
FileDataField::Dec() noexcept
{
  if (current_index > 0) {
    current_index--;
    Modified();
  }
}

void
FileDataField::Sort() noexcept
{
  if (!loaded) {
    postponed_sort = true;
    return;
  }

  // Sort the filelist (except for the first (empty) element)
  std::sort(files.begin(), files.end(), [](const Item &a,
                                           const Item &b) {
              // Compare by filename
              return StringCollate(a.filename.c_str(), b.filename.c_str()) < 0;
            });
}

ComboList
FileDataField::CreateComboList([[maybe_unused]] const char *reference) const noexcept
{
  /* sorry for the const_cast .. this method keeps the promise of not
     modifying the object, given that one does not count filling the
     (cached) file list as "modification" */
  EnsureLoadedDeconst();

  ComboList combo_list;

  char buffer[MAX_PATH];

  for (unsigned i = 0; i < files.size(); i++) {
    const Path path = files[i].filename;
    assert(path != nullptr);

    /* is a file with the same base name present in another data
       directory? */

    bool found = false;
    for (unsigned j = 1; j < files.size(); j++) {
      if (j != i && files[j].filename == path) {
        found = true;
        break;
      }
    }

    const char *display_string = path.c_str();
    if (found) {
      /* yes - append the absolute path to allow the user to see the
         difference */
      strcpy(buffer, path.c_str());
      strcat(buffer, " (");
      strcat(buffer, files[i].path.c_str());
      strcat(buffer, ")");
      display_string = buffer;
    }

    combo_list.Append(display_string);
  }

  combo_list.current_index = current_index;

  return combo_list;
}

void
FileDataField::SetFromCombo(int i, const char *) noexcept
{
  ModifyIndex(i);
}

unsigned
FileDataField::size() const noexcept
{
  EnsureLoadedDeconst();

  return files.size();
}

const FileDataField::Item &
FileDataField::GetItem(unsigned index) const noexcept
{
  EnsureLoadedDeconst();

  return files[index];
}

void
FileDataField::EnsureLoaded() noexcept
{
  if (loaded)
    return;

  loaded = true;

  for (auto i = postponed_patterns.begin(), end = postponed_patterns.end();
       i != end; ++i)
    ScanDirectoryTop(*i);

  if (postponed_sort)
    Sort();

  if (postponed_value != nullptr)
    SetValue(postponed_value);
}
