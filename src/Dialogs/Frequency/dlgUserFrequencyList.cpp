// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgUserFrequencyList.hpp"
#include "LogFile.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedCsvReader.hpp"
#include "io/FileReader.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/BufferedReader.hxx"
#include "LocalPath.hpp"
#include "io/DataFile.hpp"
#include "io/StringConverter.hpp"
#include "ActionInterface.hpp"
#include "UIGlobals.hpp"
#include "dlgFrequencyEdit.hpp"
#include "Dialogs/Message.hpp"

void UserFrequencyListWidget::ReadFile()
{
  std::unique_ptr<Reader> reader;
  list.clear();
  try
  {
    reader = OpenDataFile(file_name);
  }
  catch (...)
  {
    // file does not exist, list stays empty
    ListUpdated();
    return;
  }

  BufferedReader buffered_reader{*reader};
  StringConverter string_converter(Charset::UTF8);

  std::array<std::string_view, 4> params;

  int line = 0;
  bool has_invalids = false;
  while (1)
  {
    size_t fields_parsed = ReadCsvRecord(buffered_reader, params);
    if (fields_parsed == 0)
      break;
    line++;
    try
    {
      if (fields_parsed != 3)
        throw std::runtime_error("Invalid number of fields");

      RadioFrequency freq = RadioFrequency::Parse(params.at(2));
      if (freq.IsDefined() == false)
        throw std::runtime_error("Invalid frequency");

      tstring_view name = string_converter.Convert(params.at(0));
      tstring_view short_name = string_converter.Convert(params.at(1));
      list.emplace_back(name, short_name, freq);
    }
    catch (std::exception &e)
    {
      has_invalids = true;
      if (dialog_mode == DialogMode::MANAGE)
      {
        StaticString<256> error;
        error.Format(_("Error in frequencies file (%s) at line %d: %s"),
                     file_name, line, e.what());
        ShowMessageBox(error.buffer(), _("Error reading frequencies file"),
                       MB_OK | MB_ICONERROR);
      }
    }
  }

  if (has_invalids)
  {
    if (dialog_mode == DialogMode::MANAGE)
    {
      ShowMessageBox(_("Invalid entries will be deleted if you save changes."),
                     _("Error reading frequencies file"),
                     MB_OK | MB_ICONWARNING);
      modified = true;
    }
    else
    {
      StaticString<256> message;
      message.Format(_("Invalid entries were found in the %s file. Got to %s for details"),
                     file_name, _("Manage"));
      ShowMessageBox(message, _("Error reading frequencies file"), MB_OK | MB_ICONWARNING);
    }
  }

  ListUpdated();
}

void UserFrequencyListWidget::ListUpdated()
{
  if (dialog_mode == DialogMode::MANAGE)
  {
    edit_button->SetEnabled(list.size() > 0);
    delete_button->SetEnabled(list.size() > 0);
    duplicate_button->SetEnabled(list.size() > 0);
    up_button->SetEnabled(list.size() > 1);
    down_button->SetEnabled(list.size() > 1);
  }
  else
  {
    set_button->SetEnabled(list.size() > 0);
    set_standby_button->SetEnabled(list.size() > 0);
  }

  ListControl &list_control = GetList();
  list_control.SetLength(list.size());
  list_control.Invalidate();
}

void UserFrequencyListWidget::PromptSaveChanges() noexcept
{
  if (IsModified() &&
      ShowMessageBox(_("Save changes?"), _("Frequency list modified"),
                     MB_YESNO | MB_ICONQUESTION) == IDYES)
    UpdateFile();
}

void UserFrequencyListWidget::CloseClicked()
{
  PromptSaveChanges();
  modified = false;
  form->SetModalResult(mrOK);
}

void UserFrequencyListWidget::UpdateFile()
{
  const auto path = LocalPath(file_name);

  FileOutputStream file(path);
  BufferedOutputStream writer(file);

  for (const auto &item : list)
  {
    StaticString<8> freq_str;
    assert(item.freq.IsDefined());
    item.freq.Format(freq_str.buffer(), freq_str.capacity());

    writer.Write(item.name);
    writer.Write(_T(","));
    writer.Write(item.short_name);
    writer.Write(_T(","));
    writer.Write(freq_str);
    writer.Write(_T("\n"));
  }

  writer.Flush();
  file.Commit();

  LogFormat(_T("Frequencies file '%s' saved"), path.c_str());
}

void UserFrequencyListWidget::CreateButtons(WidgetDialog &dialog)
{
  form = &dialog;
  if (dialog_mode == DialogMode::MANAGE)
  {
    dialog.AddButton(_("New"), [this]()
                     { NewClicked(); });
    edit_button = dialog.AddButton(_("Edit"), [this]()
                                   { EditClicked(); });
    duplicate_button = dialog.AddButton(_("Duplicate"), [this]()
                                        { DuplicateClicked(); });
    delete_button = dialog.AddButton(_("Delete"), [this]()
                                     { DeleteClicked(); });
    up_button = dialog.AddSymbolButton(_T("^"), [this]()
                                       { UpClicked(); });
    down_button = dialog.AddSymbolButton(_T("v"), [this]()
                                         { DownClicked(); });
  }
  else
  {
    if (dialog_mode == DialogMode::SELECT_ACTIVE || dialog_mode == DialogMode::BROWSE)
    {
      set_button = dialog.AddButton(_("Set Active"), [this]()
                                    { SetClicked(true); });
      set_standby_button = dialog.AddButton(_("Set Standby"), [this]()
                                            { SetClicked(false); });
    }
    else if (dialog_mode == DialogMode::SELECT_STANDBY)
    {
      // Buttons are swapped if opened in SELECT_STANDBY mode
      set_standby_button = dialog.AddButton(_("Set Standby"), [this]()
                                            { SetClicked(false); });
      set_button = dialog.AddButton(_("Set Active"), [this]()
                                    { SetClicked(true); });
    }

    manage_button = dialog.AddButton(_("Manage"), [this]()
                                     { dlgUserFrequencyListWidgetShowModal(DialogMode::MANAGE); 
                                      this->ReadFile(); });
  }

  dialog.AddButton(_("Close"), [this]()
                   { CloseClicked(); });
}

void UserFrequencyListWidget::Prepare(ContainerWindow &parent,
                                      const PixelRect &rc) noexcept
{
  TextListWidget::Prepare(parent, rc);
  ReadFile();
}

void dlgUserFrequencyListWidgetShowModal(UserFrequencyListWidget::DialogMode mode)
{
  auto title = (mode == UserFrequencyListWidget::DialogMode::MANAGE) ? _("Manage Frequencies") : _("Frequencies");
  TWidgetDialog<UserFrequencyListWidget>
      dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
             UIGlobals::GetDialogLook(), title);
  dialog.SetWidget(mode);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.EnableCursorSelection();

  dialog.ShowModal();

  dialog.GetWidget().PromptSaveChanges();
}

inline void
UserFrequencyListWidget::SetClicked(bool as_active)
{
  assert(GetList().GetCursorIndex() < list.size());

  const auto &item = list[GetList().GetCursorIndex()];

  if (as_active)
    ActionInterface::SetActiveFrequency(item.freq, item.short_name);
  else
    ActionInterface::SetStandbyFrequency(item.freq, item.short_name);

  if (dialog_mode == DialogMode::SELECT_ACTIVE && as_active == true) 
    form->SetModalResult(mrOK);
  if(dialog_mode == DialogMode::SELECT_STANDBY && as_active == false)
    form->SetModalResult(mrOK);
}

void UserFrequencyListWidget::NewClicked()
{
  FrequencyListItem new_item(_T(""), _T(""), RadioFrequency::Null());

  if (dlgFrequencyEditShowModal(new_item) == FrequencyEditResult::MODIFIED)
  {
    list.emplace_back(new_item);
    ListUpdated();
    modified = true;
  }
}

void UserFrequencyListWidget::EditClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  FrequencyListItem item = list[GetList().GetCursorIndex()];

  if (dlgFrequencyEditShowModal(item) == FrequencyEditResult::MODIFIED)
  {
    list[GetList().GetCursorIndex()] = item;
    ListUpdated();
    modified = true;
  }
}

void UserFrequencyListWidget::DeleteClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  list.erase(list.begin() + GetList().GetCursorIndex());
  ListUpdated();
  modified = true;
}

void UserFrequencyListWidget::UpClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  if (GetList().GetCursorIndex() > 0)
  {
    std::swap(list[GetList().GetCursorIndex()],
              list[GetList().GetCursorIndex() - 1]);
    ListUpdated();
    modified = true;
    GetList().SetCursorIndex(GetList().GetCursorIndex() - 1);
  }
}

void UserFrequencyListWidget::DownClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  if (GetList().GetCursorIndex() < list.size() - 1)
  {
    std::swap(list[GetList().GetCursorIndex()],
              list[GetList().GetCursorIndex() + 1]);
    ListUpdated();
    modified = true;
    GetList().SetCursorIndex(GetList().GetCursorIndex() + 1);
  }
}

void UserFrequencyListWidget::DuplicateClicked()
{
  assert(GetList().GetCursorIndex() < list.size());

  list.insert(list.begin() + GetList().GetCursorIndex(),
              list[GetList().GetCursorIndex()]);
  modified = true;
  ListUpdated();
}
