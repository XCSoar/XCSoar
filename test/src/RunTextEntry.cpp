// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_DIALOG
#define ENABLE_MAIN_WINDOW
#define ENABLE_CMDLINE
#define USAGE "[--rotary] [--initial TEXT] [--allowed CHARS]"

#include "Main.hpp"
#include "Dialogs/TextEntry.hpp"
#include "ProductName.hpp"
#include "util/TruncateString.hpp"

#include <cstdio>
#include <cstring>

static bool rotary = false;
static const char *initial = "";
static const char *allowed = nullptr;

static void
ParseCommandLine(Args &args)
{
  while (!args.IsEmpty()) {
    const char *arg = args.GetNext();
    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      printf("Usage: RunTextEntry %s\n"
             "  --rotary        Use the HighScore editor\n"
             "  --initial TEXT  Start with this text\n"
             "  --allowed CHARS Restrict entered characters\n"
             "Print accepted text, or 'cancelled'.\n"
             "Report bugs to: %s\n%s home page: %s\n",
             USAGE, PRODUCT_BUGS_URL, PRODUCT_NAME, PRODUCT_WEB_SITE_URL);
      exit(EXIT_SUCCESS);
    }
    if (strcmp(arg, "--rotary") == 0)
      rotary = true;
    else if (strcmp(arg, "--initial") == 0 && !args.IsEmpty())
      initial = args.GetNext();
    else if (strcmp(arg, "--allowed") == 0 && !args.IsEmpty())
      allowed = args.GetNext();
    else
      args.UsageError();
  }
}

static void
Main([[maybe_unused]] TestMainWindow &main_window)
{
  char text[256];
  CopyTruncateString(text, sizeof(text), initial);
  AllowedCharacters filter;
  if (allowed != nullptr)
    filter = [](const char *) { return allowed; };
  const bool accepted = rotary
    ? KnobTextEntry(text, sizeof(text), "Text entry", filter)
    : TouchTextEntry(text, sizeof(text), "Text entry", filter, false);
  puts(accepted ? text : "cancelled");
}
