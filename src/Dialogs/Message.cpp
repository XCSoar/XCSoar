// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Form/Button.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/SingleWindow.hpp"
#include "UIGlobals.hpp"

#include <boost/container/static_vector.hpp>

#include <algorithm>
#include <cassert>

static WindowStyle
GetMessageBoxStyle() noexcept
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  return style;
}

static constexpr std::size_t MAX_BUTTONS = 4;

class MessageBoxForm final : public WndForm {
  WndFrame text_frame;
  boost::container::static_vector<Button, MAX_BUTTONS> buttons;
  const PixelSize button_size = Layout::Scale(PixelSize{60u, 32u});
  Button *default_button = nullptr;

  void AddButton(const MessageBoxButton &definition, WindowStyle style,
                 int default_result) noexcept {
    buttons.emplace_back(GetClientAreaWindow(), GetLook().button,
                         definition.caption, PixelRect{button_size}, style,
                         MakeModalResultCallback(definition.result));
    if (definition.result == default_result)
      default_button = &buttons.back();
  }

  void UpdateLayout(const PixelRect &parent_rc) noexcept {
    assert(!buttons.empty());

    const unsigned n_buttons = (unsigned)buttons.size();
    unsigned button_width = button_size.width;
    for (const auto &button : buttons)
      button_width = std::max(button_width, button.GetMinimumWidth());

    const unsigned available_width = parent_rc.GetWidth() > 2
      ? parent_rc.GetWidth() - 2
      : parent_rc.GetWidth();
    const unsigned available_columns =
      std::min(n_buttons,
               std::max(1u, available_width / button_width));
    const unsigned rows =
      (n_buttons + available_columns - 1) / available_columns;
    const unsigned columns = (n_buttons + rows - 1) / rows;
    const unsigned desired_width =
      std::max(Layout::Scale(200u), button_width * columns);
    PixelSize client_size(std::min(desired_width, available_width),
                          Layout::Scale(160u));

    text_frame.Resize(client_size);
    const unsigned text_height = text_frame.GetTextHeight();
    const unsigned dialog_overhead =
      ClientAreaToDialogSize(PixelSize{}).height;
    const unsigned available_height = parent_rc.GetHeight() > dialog_overhead
      ? parent_rc.GetHeight() - dialog_overhead
      : parent_rc.GetHeight();

    const unsigned row_gap = Layout::Scale(2u);
    const unsigned buttons_height =
      rows * button_size.height + (rows - 1) * row_gap;
    const unsigned desired_client_height =
      Layout::Scale(10u) + text_height + buttons_height;
    client_size.height = std::min(desired_client_height, available_height);

    const auto dialog_size = ClientAreaToDialogSize(client_size);
    Move({parent_rc.CenteredTopLeft(dialog_size), dialog_size});

    const unsigned button_top = client_size.height >
        buttons_height + Layout::Scale(4u)
      ? client_size.height - buttons_height - Layout::Scale(4u)
      : 0;
    text_frame.Resize({client_size.width,
                       std::min(text_height + Layout::GetTextPadding(),
                                button_top)});

    unsigned i = 0, row = 0;
    while (i < n_buttons) {
      const unsigned remaining = n_buttons - i;
      const unsigned remaining_rows = rows - row;
      // Balance the rows (e.g. two rows of two instead of three plus one).
      const unsigned row_size =
        (remaining + remaining_rows - 1) / remaining_rows;
      const unsigned cell_width = client_size.width / row_size;
      const unsigned actual_button_width =
        std::min(button_width, cell_width);
      const unsigned row_top =
        button_top + row * (button_size.height + row_gap);

      for (unsigned column = 0; column < row_size; ++column, ++i) {
        const int button_x = cell_width * column +
          (cell_width - actual_button_width) / 2;
        buttons[i].Move({PixelPoint(button_x, row_top),
                         PixelSize{actual_button_width,
                                   button_size.height}});
      }

      ++row;
    }
  }

public:
  MessageBoxForm(UI::SingleWindow &main_window, const DialogLook &look,
                 const char *text, const char *caption,
                 std::span<const MessageBoxButton> button_definitions,
                 int default_result) noexcept
    :WndForm(main_window, look,
             PixelRect{Layout::Scale(PixelSize{200u, 160u})}, caption,
             GetMessageBoxStyle()),
     text_frame(GetClientAreaWindow(), look,
                GetClientAreaWindow().GetClientRect())
  {
    text_frame.SetText(text);
    text_frame.SetAlignCenter();

    WindowStyle style;
    style.TabStop();
    for (const auto &definition : button_definitions)
      AddButton(definition, style, default_result);

    UpdateLayout(main_window.GetClientRect());
  }

  void ReinitialiseLayout(const PixelRect &parent_rc) noexcept override {
    UpdateLayout(parent_rc);
  }

protected:
  void SetDefaultFocus() noexcept override {
    if (default_button != nullptr)
      default_button->SetFocus();
    else
      WndForm::SetDefaultFocus();
  }
};

int
ShowMessageBox(const char *text, const char *caption,
               std::span<const MessageBoxButton> button_definitions,
               int default_result) noexcept
{
  assert(text != nullptr);

  if (button_definitions.empty() ||
      button_definitions.size() > MAX_BUTTONS)
    return mrCancel;

  auto &main_window = UIGlobals::GetMainWindow();
  MessageBoxForm form(main_window, UIGlobals::GetDialogLook(),
                      text, caption, button_definitions, default_result);
  return form.ShowModal();
}

int
ShowMessageBox(const char *text, const char *caption,
               unsigned flags) noexcept
{
  boost::container::static_vector<MessageBoxButton, 3> buttons;

  const unsigned button_flags = flags & 0x000f;
  if (button_flags == MB_OK || button_flags == MB_OKCANCEL)
    buttons.push_back({_("OK"), IDOK});

  if (button_flags == MB_YESNO || button_flags == MB_YESNOCANCEL) {
    buttons.push_back({_("Yes"), IDYES});
    buttons.push_back({_("No"), IDNO});
  }

  if (button_flags == MB_ABORTRETRYIGNORE ||
      button_flags == MB_RETRYCANCEL)
    buttons.push_back({_("Retry"), IDRETRY});

  if (button_flags == MB_OKCANCEL ||
      button_flags == MB_RETRYCANCEL ||
      button_flags == MB_YESNOCANCEL)
    buttons.push_back({_("Cancel"), IDCANCEL});

  if (button_flags == MB_ABORTRETRYIGNORE) {
    buttons.push_back({_("Abort"), IDABORT});
    buttons.push_back({_("Ignore"), IDIGNORE});
  }

  return ShowMessageBox(text, caption,
                        {buttons.data(), buttons.size()});
}
