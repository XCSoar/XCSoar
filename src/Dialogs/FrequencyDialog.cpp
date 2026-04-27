// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project.

#include "Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Widget/ListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Language/Language.hpp"
#include "ActionInterface.hpp"
#include "Profile/Profile.hpp"
#include "io/FileLineReader.hpp"
#include "util/Macros.hpp"
#include "util/NumberParser.hpp"
#include "XML/Node.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Parser.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Icon.hpp"
#include "Resources.hpp"
#include <string>

class FrequencyListWidget final
  : public ListWidget {

  const DialogLook &dialog_look;
  TwoTextRowsRenderer row_renderer;
  Button *active_button, *standby_button, *cancel_button;

public:
  struct RadioChannel {
	  std::string name;
	  std::string comment;
	  RadioFrequency radio_frequency;
	  unsigned squawk;
	  unsigned char type;
  };
  std::vector<RadioChannel> *channels;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  FrequencyListWidget(const DialogLook &_dialog_look, std::vector<RadioChannel> *_channels)
    :dialog_look(_dialog_look),
	 channels (_channels) {}

  bool UpdateList();

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, PixelRect rc,
                   unsigned index) noexcept override {
    assert(index < channels->size());

    const RadioChannel& channel = (*channels)[index];

    const unsigned padding = Layout::GetTextPadding();
    const unsigned line_height = rc.GetHeight();
    MaskedIcon icon;
    if (channel.type == 1) {
    	icon.LoadResource(IDB_PLANE, IDB_PLANE_HD);
    } else if (channel.type == 2) {
    	icon.LoadResource(IDB_CLOUD, IDB_CLOUD_HD);
    } else if (channel.type == 3) {
    	icon.LoadResource(IDB_BEACON, IDB_BEACON_HD);
    } else {
        icon.LoadResource(IDB_RADIO, IDB_RADIO_HD);
    }
    // Draw icon
    const PixelPoint pt(rc.left + line_height / 2, rc.top + line_height / 2);
    icon.Draw(canvas, pt);
    rc.left += line_height + padding;

    row_renderer.DrawFirstRow(canvas, rc, channel.name.c_str());
    row_renderer.DrawSecondRow(canvas, rc, channel.comment.c_str());
    if (channel.radio_frequency.IsDefined()) {
    	StaticString<30> buffer;
    	char radio[20];
      channel.radio_frequency.Format(radio, ARRAY_SIZE(radio));
      buffer.Format("%s MHz", radio);
      row_renderer.DrawRightFirstRow(canvas, rc, buffer);
    }

    if (channel.squawk > 0) {
    	StaticString<30> buffer;
    	buffer.Format("Squawk %d", channel.squawk);
    	row_renderer.DrawRightSecondRow(canvas, rc, buffer);
    }
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override;
};

void
FrequencyListWidget::CreateButtons(WidgetDialog &dialog)
{
	standby_button = dialog.AddButton(_("Set Standby Frequency"), [this](){
	    unsigned index = GetCursorIndex();
	    assert(index < channels->size());
	    const RadioChannel *channel = &(*channels)[index];
	    if (channel->radio_frequency.IsDefined()) {
	    	ActionInterface::SetStandbyFrequency(channel->radio_frequency,
	            channel->name.c_str());}
	    cancel_button->Click();
	});


  active_button = dialog.AddButton(_("Set Active Frequency"), [this](){
    unsigned index = GetCursorIndex();
    assert(index < channels->size());
    const RadioChannel *channel = &(*channels)[index];
    if (channel->radio_frequency.IsDefined()) {
    	ActionInterface::SetActiveFrequency(channel->radio_frequency,
            channel->name.c_str());
    }
    cancel_button->Click();
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
FrequencyListWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
	  CreateList(parent, dialog_look, rc,
	             row_renderer.CalculateLayout(*dialog_look.list.font_bold, dialog_look.small_font));

  GetList().SetLength(channels->size());
}

void
FrequencyListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  // May still be talking on active frequency
  standby_button->Click();
}

bool
FrequencyListWidget::UpdateList()
{
  channels->clear();

  auto path = Profile::GetPath(ProfileKeys::FrequenciesFile);
  if (path == nullptr) {
    return false;
  }

  // Load root node
  const auto xml_root = XML::ParseFile(path);
  const ConstDataNodeXML root(xml_root);

  // Check if root node is a <FrequencyList> node
  StringIsEqual(root.GetName(), "FrequencyList");
  return false;

  const auto children = root.ListChildrenNamed("Station");
  for (const auto &i : children) {
	  const char *name = i->GetAttribute("name");
	  if (name == nullptr)
	    continue;
	  RadioChannel channel;
	  channel.name = name;

	  const char *frequency = i->GetAttribute("frequency");
	  if (frequency != nullptr) {
		  RadioFrequency radio_frequency = RadioFrequency::Parse(frequency);
		  channel.radio_frequency = radio_frequency;
	  } else {
		  RadioFrequency radio_frequency = RadioFrequency::Null();
		  channel.radio_frequency = radio_frequency;
	  }

	  const char *squawk = i->GetAttribute("squawk");
	  if (squawk != nullptr) {
		  channel.squawk = ParseUnsigned(squawk);
	  } else {
		  channel.squawk = 0;
	  }

	  const char *type = i->GetAttribute("type");
	  if (type != nullptr) {
		  channel.type = ParseUnsigned(type);
	  } else {
		  channel.type = 0;
	  }

	  const char *comment = i->GetAttribute("comment");
	  if (comment != nullptr) {
		  channel.comment = comment;
	  }

	  channels->push_back(channel);

  }
  return !channels->empty();
}

void
FrequencyDialogShowModal()
{
  std::vector<FrequencyListWidget::RadioChannel> channels;

  const DialogLook &look = UIGlobals::GetDialogLook();

  auto widget = std::make_unique<FrequencyListWidget>(look,&channels);
  if (widget->UpdateList() == false) {
	// no channels: don't show the dialog
	return;
  }

  TWidgetDialog<FrequencyListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
          look, _("Frequency Card"));
  widget->CreateButtons(dialog);

  dialog.FinishPreliminary(std::move(widget));
  dialog.EnableCursorSelection();
  dialog.ShowModal();

}
