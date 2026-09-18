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

class MessageBoxForm final : public WndForm {
  WndFrame text_frame;
  boost::container::static_vector<Button, 3> buttons;
  const PixelSize button_size = Layout::Scale(PixelSize{60u, 32u});

  void AddButton(const char *caption, int result, WindowStyle style) noexcept {
    buttons.emplace_back(GetClientAreaWindow(), GetLook().button,
                         caption, PixelRect{button_size}, style,
                         MakeModalResultCallback(result));
  }

  void CreateButtons(unsigned flags) noexcept {
    WindowStyle style;
    style.TabStop();

    const unsigned button_flags = flags & 0x000f;
    if (button_flags == MB_OK || button_flags == MB_OKCANCEL)
      AddButton(_("OK"), IDOK, style);

    if (button_flags == MB_YESNO || button_flags == MB_YESNOCANCEL) {
      AddButton(_("Yes"), IDYES, style);
      AddButton(_("No"), IDNO, style);
    }

    if (button_flags == MB_ABORTRETRYIGNORE ||
        button_flags == MB_RETRYCANCEL)
      AddButton(_("Retry"), IDRETRY, style);

    if (button_flags == MB_OKCANCEL ||
        button_flags == MB_RETRYCANCEL ||
        button_flags == MB_YESNOCANCEL)
      AddButton(_("Cancel"), IDCANCEL, style);

    if (button_flags == MB_ABORTRETRYIGNORE) {
      AddButton(_("Abort"), IDABORT, style);
      AddButton(_("Ignore"), IDIGNORE, style);
    }
  }

  void UpdateLayout(const PixelRect &parent_rc) noexcept {
    const unsigned available_width = parent_rc.GetWidth() > 2
      ? parent_rc.GetWidth() - 2
      : parent_rc.GetWidth();
    PixelSize client_size(std::min(Layout::Scale(200u), available_width),
                          Layout::Scale(160u));

    text_frame.Resize(client_size);
    const unsigned text_height = text_frame.GetTextHeight();
    const unsigned desired_client_height =
      Layout::Scale(10u) + text_height + button_size.height;
    const unsigned dialog_overhead =
      ClientAreaToDialogSize(PixelSize{}).height;
    const unsigned available_height = parent_rc.GetHeight() > dialog_overhead
      ? parent_rc.GetHeight() - dialog_overhead
      : parent_rc.GetHeight();
    client_size.height = std::min(desired_client_height, available_height);

    const auto dialog_size = ClientAreaToDialogSize(client_size);
    Move({parent_rc.CenteredTopLeft(dialog_size), dialog_size});

    const unsigned cell_width = client_size.width / buttons.size();
    const unsigned actual_button_width =
      std::min(button_size.width, cell_width);
    const unsigned button_top = client_size.height >
        button_size.height + Layout::Scale(4u)
      ? client_size.height - button_size.height - Layout::Scale(4u)
      : 0;
    text_frame.Resize({client_size.width,
                       std::min(text_height + Layout::GetTextPadding(),
                                button_top)});

    for (unsigned i = 0; i < buttons.size(); ++i) {
      const int button_x = cell_width * i +
        (cell_width - actual_button_width) / 2;
      buttons[i].Move({PixelPoint(button_x, button_top),
                       PixelSize{actual_button_width, button_size.height}});
    }
  }

public:
  MessageBoxForm(UI::SingleWindow &main_window, const DialogLook &look,
                 const char *text, const char *caption,
                 unsigned flags) noexcept
    :WndForm(main_window, look,
             PixelRect{Layout::Scale(PixelSize{200u, 160u})}, caption,
             GetMessageBoxStyle()),
     text_frame(GetClientAreaWindow(), look,
                GetClientAreaWindow().GetClientRect())
  {
    text_frame.SetText(text);
    text_frame.SetAlignCenter();
    CreateButtons(flags);
    UpdateLayout(main_window.GetClientRect());
  }

  void ReinitialiseLayout(const PixelRect &parent_rc) noexcept override {
    UpdateLayout(parent_rc);
  }
};

int
ShowMessageBox(const char *text, const char *caption,
               unsigned flags) noexcept
{
  assert(text != nullptr);

  auto &main_window = UIGlobals::GetMainWindow();
  MessageBoxForm form(main_window, UIGlobals::GetDialogLook(),
                      text, caption, flags);
  return form.ShowModal();
}
