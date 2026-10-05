// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxGroupPicker.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/MultiSelectListWidget.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "ui/canvas/Canvas.hpp"
#include "util/StringBuilder.hxx"

using InfoBoxFactory::Group;
using InfoBoxFactory::GroupMask;

/**
 * One row per group, a checkbox in front of the name.
 */
class InfoBoxGroupListWidget final : public MultiSelectListWidget {
  TextRowRenderer row_renderer;

  /** the ticks to start with; applied in Prepare(), because the
      selection state only exists once the list does */
  const GroupMask initial;

public:
  explicit InfoBoxGroupListWidget(const GroupMask &_initial) noexcept
    :initial(_initial) {}

  void LoadMask(const GroupMask &mask) noexcept {
    for (unsigned i = 0; i < unsigned(Group::COUNT); ++i)
      SetSelected(i, mask.Contains(Group(i)));
  }

  GroupMask GetMask() const noexcept {
    GroupMask mask;
    mask.Clear();
    for (unsigned i = 0; i < unsigned(Group::COUNT); ++i)
      mask.Set(Group(i), IsSelected(i));
    return mask;
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    const DialogLook &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc,
               row_renderer.CalculateLayout(*look.list.font));
    SetLengthWithSelection(unsigned(Group::COUNT));
    LoadMask(initial);
    MultiSelectListWidget::Prepare(parent, rc);
  }

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= unsigned(Group::COUNT))
      return;

    DrawCheckboxText(canvas, rc,
                     gettext(InfoBoxFactory::GetGroupName(Group(idx))),
                     IsSelected(idx));
  }
};

bool
InfoBoxGroupPicker(GroupMask &mask) noexcept
{
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Groups"));

  auto widget = std::make_unique<InfoBoxGroupListWidget>(mask);
  InfoBoxGroupListWidget *const list = widget.get();

  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(C_("Button", "Select all"), [list](){ list->SelectAll(); });
  dialog.AddButton(C_("Button", "Select none"), [list](){ list->ClearSelection(); });
  dialog.AddButton(_("Cancel"), mrCancel);

  /* the widget is prepared by ShowModal(), so the ticks are set in
     Prepare() from the mask handed to the constructor */
  dialog.FinishPreliminary(std::move(widget));

  if (dialog.ShowModal() != mrOK)
    return false;

  mask = list->GetMask();
  return true;
}

const char *
FormatInfoBoxGroups(GroupMask mask, std::span<char> buffer) noexcept
{
  if (mask.IsAll())
    return _("All");

  if (mask.IsEmpty())
    return _("None");

  if (buffer.empty())
    return "";

  buffer.front() = '\0';
  BasicStringBuilder<char> builder{buffer};
  bool first = true;

  try {
    for (unsigned i = 0; i < unsigned(Group::COUNT); ++i) {
      if (!mask.Contains(Group(i)))
        continue;

      if (!first)
        builder.Append(", ");
      first = false;
      builder.Append(gettext(InfoBoxFactory::GetGroupName(Group(i))));
    }
  } catch (BasicStringBuilder<char>::Overflow) {
    /* the buffer is full: what fits is enough for a caption */
  }

  return buffer.data();
}
