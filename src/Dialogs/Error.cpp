// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Error.hpp"
#include "Message.hpp"
#include "util/ConvertString.hpp"
#include "util/Exception.hxx"
#include "util/StaticString.hxx"

void
ShowError(std::exception_ptr e, const TCHAR *caption) noexcept
{
  ShowMessageBox(UTF8ToWideConverter(GetFullMessage(e).c_str()), caption,
                 MB_OK|MB_ICONEXCLAMATION);
}

void
ShowError(const TCHAR *msg, std::exception_ptr e,
          const TCHAR *caption) noexcept
{
  StaticString<1024> buffer;
  buffer.Format(_T("%s\n%s"), msg,
                UTF8ToWideConverter(GetFullMessage(e).c_str()).c_str());
  buffer.CropIncompleteUTF8();

  ShowMessageBox(msg, caption,
                 MB_OK|MB_ICONEXCLAMATION);
}
