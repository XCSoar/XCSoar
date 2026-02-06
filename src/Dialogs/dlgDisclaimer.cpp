// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgDisclaimer.hpp"
#include "WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/RichTextWindowWithInternalLinks.hpp"
#include "Widget/Widget.hpp"
#include "Form/VScrollPanel.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "Version.hpp"
#include "Asset.hpp"
#include "util/StringCompare.hxx"

#include <memory>

/**
 * Widget that displays the disclaimer text with a checkbox at the bottom.
 * The text area is scrollable and fills available space,
 * checkbox has fixed height at bottom.
 */
class DisclaimerWidget : public NullWidget, public VScrollPanelListener {
  const DialogLook &look;
  std::unique_ptr<VScrollPanel> scroll_panel;
  RichTextWindowWithInternalLinks text_window;
  CheckBoxControl checkbox;
  Button *accept_button = nullptr;
  const char *text;
  bool visible = false;

public:
  DisclaimerWidget(const DialogLook &_look, const char *_text) noexcept
    : look(_look), text(_text) {}

  void SetAcceptButton(Button *button) noexcept {
    accept_button = button;
  }

private:
  unsigned GetCheckboxHeight() const noexcept {
    return Layout::GetMinimumControlHeight();
  }

  PixelRect GetScrollRect(const PixelRect &rc) const noexcept {
    PixelRect scroll_rc = rc;
    scroll_rc.bottom -= GetCheckboxHeight() + Layout::Scale(4);
    return scroll_rc;
  }

  PixelRect GetCheckboxRect(const PixelRect &rc) const noexcept {
    PixelRect cb_rc = rc;
    cb_rc.top = rc.bottom - GetCheckboxHeight();
    return cb_rc;
  }

  static unsigned GetScrollbarWidth() noexcept {
    return HasPointer()
      ? Layout::GetMinimumControlHeight()
      : Layout::VptScale(10);
  }

  PixelRect ReserveScrollbar(PixelRect rc) const noexcept {
    const unsigned scrollbar_width = GetScrollbarWidth();
    if (scrollbar_width > 0 && rc.GetWidth() > scrollbar_width)
      rc.right -= scrollbar_width;
    return rc;
  }

  void UpdateVirtualHeight() noexcept {
    if (!scroll_panel)
      return;

    unsigned content_height = text_window.GetContentHeight();
    unsigned panel_height = scroll_panel->GetSize().height;
    scroll_panel->SetVirtualHeight(std::max(content_height, panel_height));
  }

  void OnVScrollPanelChange() noexcept override {
    if (visible && scroll_panel) {
      PixelRect rc = ReserveScrollbar(scroll_panel->GetVirtualRect());
      text_window.Move(rc);
    }
  }

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();
    style.ControlParent();
    style.TabStop();

    // Create scroll panel
    scroll_panel = std::make_unique<VScrollPanel>(
      parent, look, GetScrollRect(rc), style, *this);

    // Create text window inside scroll panel
    WindowStyle text_style;
    text_style.TabStop();
    PixelRect text_rc = ReserveScrollbar(scroll_panel->GetVirtualRect());
    text_window.Create(*scroll_panel, text_rc, text_style);
    text_window.SetFont(look.text_font, &look.bold_font, &look.bold_font);
    text_window.SetText(text);

    // Create checkbox
    WindowStyle cb_style;
    cb_style.Hide();
    cb_style.TabStop();
    checkbox.Create(parent, look,
      _("I have read and understand the above disclaimer"),
      GetCheckboxRect(rc), cb_style,
      [this](bool value) {
        if (accept_button != nullptr)
          accept_button->SetEnabled(value);
      }
    );
  }

  void Show(const PixelRect &rc) noexcept override {
    scroll_panel->Move(GetScrollRect(rc));
    UpdateVirtualHeight();
    visible = true;
    text_window.Move(ReserveScrollbar(scroll_panel->GetVirtualRect()));
    text_window.Show();
    scroll_panel->Show();

    checkbox.SetState(false);
    checkbox.MoveAndShow(GetCheckboxRect(rc));
    if (accept_button != nullptr)
      accept_button->SetEnabled(false);
  }

  void Hide() noexcept override {
    visible = false;
    scroll_panel->Hide();
    text_window.FastHide();
    checkbox.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    scroll_panel->Move(GetScrollRect(rc));
    if (visible) {
      UpdateVirtualHeight();
      text_window.Move(ReserveScrollbar(scroll_panel->GetVirtualRect()));
    }
    checkbox.Move(GetCheckboxRect(rc));
  }

  bool SetFocus() noexcept override {
    scroll_panel->SetFocus();
    return true;
  }
};

static constexpr const char *disclaimer_text =
  "# Important Safety Notice\n\n"
  "By using XCSoar, you acknowledge and accept the following:\n\n"
  "## No Warranty (GPL Section 11)\n\n"
  "Because the program is licensed free of charge, there is no warranty "
  "for the program, to the extent permitted by applicable law. Except when "
  "otherwise stated in writing the copyright holders and/or other parties "
  "provide the program \"AS IS\" without warranty of any kind, either expressed "
  "or implied, including, but not limited to, the implied warranties of "
  "merchantability and fitness for a particular purpose. The entire risk as "
  "to the quality and performance of the program is with you. Should the "
  "program prove defective, you assume the cost of all necessary servicing, "
  "repair or correction.\n\n"
  "## Limitations\n\n"
  "- XCSoar is for **situational awareness only**\n"
  "- XCSoar is **not** a FLARM display\n"
  "- XCSoar is **not** aviation certified in any way\n"
  "- The artificial horizon is **not** fit for any purpose\n"
  "- Databanks (airspace, waypoints) are **not** guaranteed to be current\n"
  "- XCSoar may contain bugs and may indicate wrong data\n\n"
  "## Pilot Responsibility\n\n"
  "The **Pilot in Command** is always responsible for the safe operation "
  "of the aircraft. Never rely solely on XCSoar for navigation or "
  "situational awareness.\n\n"
  "[Full GPL License](xcsoar://dialog/credits)";

bool
dlgDisclaimerShowModal() noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Safety Disclaimer"));

  // Create combined widget with text and checkbox
  auto widget = std::make_unique<DisclaimerWidget>(look, disclaimer_text);
  DisclaimerWidget *widget_ptr = widget.get();

  dialog.FinishPreliminary(std::move(widget));

  // Add OK button (starts disabled)
  Button *ok_button = dialog.AddButton(_("OK"), mrOK);
  ok_button->SetEnabled(false);
  widget_ptr->SetAcceptButton(ok_button);

  // Add Cancel button
  dialog.AddButton(_("Cancel"), mrCancel);

  return dialog.ShowModal() == mrOK;
}

bool
CheckShowDisclaimer() noexcept
{
  // Get the version that was previously acknowledged
  const char *acknowledged_version =
    Profile::Get(ProfileKeys::DisclaimerAcknowledgedVersion);
  if (acknowledged_version != nullptr &&
      StringIsEqual(acknowledged_version, XCSoar_Version)) {
    // Already acknowledged for this version
    return true;
  }

  // Show the disclaimer dialog
  if (!dlgDisclaimerShowModal()) {
    // User declined
    return false;
  }

  // Save the acknowledgment
  Profile::Set(ProfileKeys::DisclaimerAcknowledgedVersion, XCSoar_Version);
  Profile::Save();
  return true;
}
