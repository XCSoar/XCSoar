// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GroupedListPicker.hpp"
#include "WidgetDialog.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/File.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Repository/Glue.hpp"
#include "UIGlobals.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "net/http/Features.hpp"
#include "system/Path.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "DownloadFilePicker.hpp"
#endif

#include <algorithm>
#include <memory>
#include <vector>

int
PickChoice(const char *caption, const char *help,
           std::span<const PickerChoice> choices, int current) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, caption);

  /* a choice which explains itself must be readable before it is
     taken: the first tap only moves the cursor there, which shows the
     explanation, the second tap or the OK button takes it */
  const bool explained =
    std::any_of(choices.begin(), choices.end(),
                [](const PickerChoice &choice){
                  return choice.help != nullptr;
                });

  auto _list = std::make_unique<GroupedListWidget>(look);
  GroupedListWidget &list = *_list;

  /* the explanation of the setting introduces the page, above the
     choices: it is read before one of them is taken */
  if (help != nullptr)
    list.AddHero(caption, help);

  list.AddGroup(nullptr,
                {.selection_mode = GroupedListWidget::SelectionMode::SINGLE,
                 .enter_action = explained
                 ? GroupedListWidget::EnterAction::ACTION_BAR
                 : GroupedListWidget::EnterAction::ITEM});

  int picked = -1;

  for (std::size_t i = 0; i < choices.size(); ++i)
    list.AddItem(choices[i].caption, [&dialog, &picked, i](){
      picked = i;
      dialog.SetModalResult(mrOK);
    }, {.checked = (int)i == current, .help = choices[i].help});

  if (current >= 0)
    list.SetCursorIndex(current);

  dialog.FinishPreliminary(std::move(_list));

  if (explained) {
    dialog.AddButton(_("OK"), [&dialog, &picked, &list](){
      picked = list.GetCursorIndex();
      dialog.SetModalResult(mrOK);
    });

    dialog.AddButton(_("Cancel"), mrCancel);

    list.SetActionBar(dialog.GetButtonPanel());
    dialog.EnableCursorSelection();
  } else
    dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  return picked;
}

const char *
GetEnumCaption(const StaticEnumChoice *list, unsigned value) noexcept
{
  for (auto i = list; i->display_string != nullptr; ++i)
    if (i->id == value)
      return gettext(i->display_string);

  return "";
}

bool
PickEnum(const char *caption, const char *help,
         const StaticEnumChoice *list, unsigned &value) noexcept
{
  std::vector<PickerChoice> choices;
  int current = -1;

  for (auto i = list; i->display_string != nullptr; ++i) {
    if (i->id == value)
      current = choices.size();

    choices.push_back({gettext(i->display_string),
                       i->help != nullptr ? gettext(i->help) : nullptr});
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || picked == current)
    return false;

  value = list[picked].id;
  return true;
}

void
PickFile(const char *caption, const char *help, FileDataField &df) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, caption);

  auto list = std::make_unique<GroupedListWidget>(look);
  list->AddHero(caption, help);
  list->AddGroup(nullptr,
                 {.selection_mode = GroupedListWidget::SelectionMode::SINGLE});

  const Path value = df.GetValue();

  for (unsigned i = 0, n = df.size(); i < n; ++i) {
    const auto &item = df.GetItem(i);
    const bool checked = item.path == value;
    if (checked)
      list->SetCursorIndex(i);

    /* the first item is the empty one, which stands for no file */
    list->AddItem(item.path.empty() ? _("(none)") : item.filename.c_str(),
                  [&df, &dialog, i](){
      df.SetIndex(i);
      dialog.SetModalResult(mrOK);
    }, {.checked = checked});
  }

#ifdef HAVE_DOWNLOAD_MANAGER
  if (FileTypeSupportsDownload(df.GetFileType()))
    list->AddButton(_("Download"), [&df, &dialog](){
      const auto path = DownloadFilePicker(df.GetFileType());
      if (path == nullptr)
        return;

      df.ForceModify(path);
      dialog.SetModalResult(mrOK);
    });
#endif

  dialog.FinishPreliminary(std::move(list));
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
