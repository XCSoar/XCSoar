// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/DataField/Date.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Form/Edit.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "LocalPath.hpp"
#include "Profile/Profile.hpp"
#include "RowFormWidget.hpp"

namespace {

static WndProperty *
FinishFileProperty(RowFormWidget &form, const char *label, const char *help,
                   std::string_view profile_key, const char *filters,
                   FileDataField &df, bool nullable) noexcept
{
  WndProperty *edit = form.Add(label, help);
  edit->SetDataField(&df);

  if (nullable)
    df.AddNull();

  df.ScanMultiplePatterns(filters);

  if (profile_key.data() != nullptr) {
    const auto path = Profile::GetPath(profile_key);
    if (path != nullptr)
      df.SetValue(path);
  }

  edit->RefreshDisplay();
  return edit;
}

static void
ScanFileTypePatterns(FileDataField &df,
                     std::initializer_list<FileType> file_types) noexcept
{
  for (const auto file_type : file_types)
    df.ScanMultiplePatterns(GetFileTypePatterns(file_type));
}

} // namespace

WndProperty *
RowFormWidget::AddFile(const char *label, const char *help,
                       std::string_view profile_key, const char *filters,
                       FileType file_type,
                       bool nullable) noexcept
{
  auto *df = new FileDataField();
  df->SetFileType(file_type);
  return FinishFileProperty(*this, label, help, profile_key, filters, *df,
                            nullable);
}

WndProperty *
RowFormWidget::AddFile(const char *label, const char *help,
                       std::string_view profile_key, const char * /*filters*/,
                       std::initializer_list<FileType> file_types,
                       bool nullable) noexcept
{
  return AddFile(label, help, profile_key, file_types, nullable);
}

WndProperty *
RowFormWidget::AddFile(const char *label, const char *help,
                       std::string_view profile_key,
                       std::initializer_list<FileType> file_types,
                       bool nullable) noexcept
{
  auto *df = new FileDataField();
  df->SetFileTypes(file_types);

  WndProperty *edit = Add(label, help);
  edit->SetDataField(df);

  if (nullable)
    df->AddNull();

  ScanFileTypePatterns(*df, file_types);

  if (profile_key.data() != nullptr) {
    const auto path = Profile::GetPath(profile_key);
    if (path != nullptr)
      df->SetValue(path);
  }

  edit->RefreshDisplay();
  return edit;
}

WndProperty *
RowFormWidget::AddMultipleFiles(const char *label, const char *help,
                                std::string_view registry_key,
                                const char *filters, FileType file_type)
{

  WndProperty *edit = Add(label, help);
  auto *df = new MultiFileDataField();
  df->SetFileType(file_type);
  edit->SetDataField(df);

  df->ScanMultiplePatterns(filters);

  if (registry_key.data() != nullptr) {
    auto paths = Profile::GetMultiplePaths(registry_key, filters);

    if (!paths.empty()) {
      for (auto const &p : paths) {
        df->AddInitialPath(p);
      }
    }
  }

  edit->RefreshDisplay();

  return edit;
}

void
RowFormWidget::SetProfile(std::string_view profile_key, unsigned value) noexcept
{
  Profile::Set(profile_key, value);
}

bool
RowFormWidget::SaveValue(unsigned i, std::string_view profile_key,
                         char *string, size_t max_size) const noexcept
{
  if (!SaveValue(i, string, max_size))
    return false;

  Profile::Set(profile_key, string);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, std::string_view profile_key,
                         std::string &string) const noexcept
{
  if (!SaveValue(i, string))
    return false;

  Profile::Set(profile_key, string);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, std::string_view profile_key,
                         bool &value, bool negated) const noexcept
{
  if (!SaveValue(i, value, negated))
    return false;

  Profile::Set(profile_key, value);
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i, std::string_view profile_key,
                         double &value) const noexcept
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(profile_key, value);
  return true;
}

bool
RowFormWidget::SaveValueFileReader(unsigned i,
                                   std::string_view profile_key) noexcept
{
  return Profile::SetPath(profile_key, GetValueFile(i));
}

bool
RowFormWidget::SaveValue(unsigned i,
                         std::string_view profile_key,
                         BrokenDate &value) const noexcept
{
  const auto &df = (const DataFieldDate &)GetDataField(i);
  assert(df.GetType() == DataField::Type::DATE);

  const auto new_value = df.GetValue();

  if (!new_value.IsPlausible())
    return false;

  if (new_value == value)
    return false;

  char buffer[0x10];
  FormatISO8601(buffer, new_value);
  Profile::Set(profile_key, buffer);
  value = new_value;
  return true;
}

bool
RowFormWidget::SaveValue(unsigned i,
                         std::string_view profile_key,
                         std::chrono::seconds &value) const noexcept
{
  if (!SaveValue(i, value))
    return false;

  Profile::Set(profile_key, value);
  return true;
}

bool
RowFormWidget::SaveValueMultiFileReader(unsigned i,
                                        std::string_view registry_key) noexcept
{
  const auto *dfe =
      static_cast<const MultiFileDataField *>(GetControl(i).GetDataField());

  return Profile::SetMultiplePaths(registry_key, dfe->GetPathFiles());
}
