// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HelpDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"

#include <cassert>

void
HelpDialog(const char *Caption, const char *HelpText)
{
  assert(HelpText != nullptr);

  const char *prefix = _("Help");

  StaticString<100> full_caption;
  if (Caption != nullptr) {
    full_caption.Format(_T("%s: %s"), prefix, Caption);
    Caption = full_caption.c_str();
  } else
    Caption = prefix;

  const auto &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, Caption, new LargeTextWidget(look, HelpText));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
