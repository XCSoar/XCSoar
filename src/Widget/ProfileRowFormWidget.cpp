// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RowFormWidget.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/Date.hpp"
#include "Profile/Profile.hpp"
#include "LocalPath.hpp"
#include "util/ConvertString.hpp"
#include "Formatter/TimeFormatter.hpp"

WndProperty *
RowFormWidget::AddFile(const TCHAR *label, const TCHAR *help,
                       std::string_view profile_key, const TCHAR *filters,
                       FileType file_type,
                       bool nullable) noexcept
{
  WndProperty *edit = Add(label, help);
  auto *df = new FileDataField();
  df->SetFileType(file_type);
  edit->SetDataField(df);

  if (nullable)
    df->AddNull();

  df->ScanMultiplePatterns(filters);

  if (profile_key.data() != nullptr) {
    const auto path = Profile::GetPath(profile_key);
    if (path != nullptr)
      df->SetValue(path);
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
                         TCHAR *string, size_t max_size) const noexcept
{
  if (!SaveValue(i, string, max_size))
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
  Path new_value = GetValueFile(i);
  const auto contracted = ContractLocalPath(new_value);
  if (contracted != nullptr)
    new_value = contracted;

  const WideToUTF8Converter new_value2(new_value.c_str());
  if (!new_value2.IsValid())
    return false;

  const char *old_value = Profile::Get(profile_key, "");
  if (StringIsEqual(old_value, new_value2))
    return false;

  Profile::Set(profile_key, new_value2);
  return true;
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

  TCHAR buffer[0x10];
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
