// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskQRCodeDialog.hpp"

#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "QR/QRCode.hpp"
#include "Renderer/QRCodeRenderer.hpp"
#include "Task/CupTaskQR.hpp"
#include "Task/XCSoarTaskQR.hpp"
#include "Task/XCTrackTaskEncoder.hpp"
#include "UIGlobals.hpp"
#include "Widget/DrawWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/TextFormat.hpp"
#include "ui/dim/Rect.hpp"

#include <string>

static std::string
TaskQRLabel(const OrderedTask &task)
{
  const std::string_view name = task.GetName().c_str();
  if (name.empty())
    return _("XCSoar task");

  return std::string{name};
}

void
ShowTaskQRCode(const OrderedTask &task) noexcept
{
  if (task.TaskSize() == 0) {
    ShowMessageBox(_("Empty task"), _("QR Code"), MB_OK);
    return;
  }

  /* The XCSoar task file keeps lines, sectors, keyholes and the
     rules.  A SeeYou CUP file is the next choice when that file does
     not fit: it keeps the zones SeeYou can describe.  XCTrack's
     cylinder code is the last fallback. */
  auto text   = EncodeXCSoarTaskQRText(task);
  auto symbol = text ? EncodeQRCode(*text) : QRSymbol{};
  if (!symbol.IsValid()) {
    text = EncodeCupTaskQRText(task);
    if (text) symbol = EncodeQRCode(*text);
  }
  if (!symbol.IsValid()) {
    text = EncodeXCTrackQRText(task);
    if (text) symbol = EncodeQRCode(*text);
  }
  if (!symbol.IsValid()) {
    ShowMessageBox(
      _("This task cannot be shown as a QR code"), _("QR Code"), MB_OK);
    return;
  }

  /* Full screen so the modules are large enough to scan off the
     display.  DrawWidget is the existing owner-draw element; the
     symbol stays sharp because DrawQRSymbol scales by whole
     modules.  A saved task shows its name under the code. */
  const std::string label = TaskQRLabel(task);
  const DialogLook &look  = UIGlobals::GetDialogLook();
  DrawWidget widget(
    [&symbol, &look, label](Canvas &canvas, const PixelRect &rc) {
      canvas.Clear(COLOR_WHITE);

      const unsigned line = look.text_font.GetLineSpacing();
      unsigned band       = line * 2 + line / 2;
      if (band > rc.GetHeight() / 5) band = rc.GetHeight() / 5;

      PixelRect code_rc = rc;
      if (band > line) code_rc.bottom -= static_cast<int>(band);

      DrawQRSymbol(canvas, code_rc, symbol);

      if (band <= line) return;

      canvas.Select(look.text_font);
      canvas.SetTextColor(COLOR_BLACK);
      canvas.SetBackgroundTransparent();
      canvas.DrawFormattedText(
        PixelRect{rc.left, code_rc.bottom, rc.right, rc.bottom},
        label,
        DT_CENTER | DT_VCENTER);
    });

  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look,
                      _("QR Code"),
                      &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
