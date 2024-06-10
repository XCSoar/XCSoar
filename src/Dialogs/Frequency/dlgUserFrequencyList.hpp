// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/TextListWidget.hpp"
#include "RadioFrequency.hpp"
#include "util/StaticString.hxx"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "util/tstring_view.hxx"
#include <vector>

struct FrequencyListItem
{
  StaticString<64> name;
  StaticString<16> short_name;
  RadioFrequency freq;

  FrequencyListItem(const tstring_view &_name, const tstring_view &_short_name,
                    const RadioFrequency &_freq)
      : name(_name), short_name(_short_name), freq(_freq)
  {
  }

  StaticString<128> &getTitle() const
  {
    StaticString<8> freq_str;
    freq.Format(freq_str.buffer(), freq_str.capacity());
    bool has_short_name = short_name.length() > 0;
    bool has_name = name.length() > 0;
    if (has_name && has_short_name)
      title.Format(_T("%s (%s): %s"), name.c_str(), short_name.c_str(), freq_str.buffer());
    else if (has_name)
      title.Format(_T("%s: %s"), name.c_str(), freq_str.buffer());
    else if (has_short_name)
      title.Format(_T("%s: %s"), short_name.c_str(), freq_str.buffer());
    else
      title.Format(_T("%s"), freq_str.buffer());
    return title;
  }

private:
  mutable StaticString<128> title;
};

class UserFrequencyListWidget final
    : public TextListWidget
{

  const TCHAR *file_name = _T("user.freq");

  WndForm *form;

  Button *set_button, *set_standby_button, *manage_button;
  Button *edit_button, *delete_button, *duplicate_button;
  Button *up_button, *down_button;

  bool modified = false;

  std::vector<FrequencyListItem> list;

public:
  enum DialogMode
  {
    BROWSE,
    MANAGE,
    SELECT_ACTIVE,
    SELECT_STANDBY
  } const dialog_mode;

  explicit UserFrequencyListWidget(enum DialogMode _mode) : dialog_mode(_mode) {}

  void CreateButtons(WidgetDialog &dialog);

  /**
   * @brief Update the user.freq file with the current list of frequencies
   */
  void UpdateFile();

  /**
   * @brief Check if the list has been modified
   * @note Ignoring invalid entries also counts as modification
   */
  bool IsModified() const noexcept
  {
    if (dialog_mode != DialogMode::MANAGE)
    {
      assert(modified == false);
      return false;
    }
    return modified;
  }

private:
  void ReadFile();
  void ListUpdated();
  void SetClicked(bool as_active);
  void NewClicked();
  void EditClicked();
  void DeleteClicked();
  void UpClicked();
  void DownClicked();
  void DuplicateClicked();
  void CloseClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void PromptSaveChanges() noexcept;

protected:
  /* virtual methods from TextListWidget */
  const TCHAR *GetRowText(unsigned i) const noexcept override
  {
    return list[i].getTitle();
  }

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override
  {
    return dialog_mode != DialogMode::BROWSE;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override
  {
    if (dialog_mode == DialogMode::SELECT_ACTIVE)
      SetClicked(true);
    else if (dialog_mode == DialogMode::SELECT_STANDBY)
      SetClicked(false);
    else if (dialog_mode == DialogMode::MANAGE)
      EditClicked();
  }
};

void dlgUserFrequencyListWidgetShowModal(UserFrequencyListWidget::DialogMode mode);
