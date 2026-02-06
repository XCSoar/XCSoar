// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/control/RichTextWindow.hpp"
#include "Dialogs/InternalLink.hpp"
#include "util/UriSchemes.hpp"
#include "system/OpenLink.hpp"
#include "LogFile.hpp"

/**
 * A RichTextWindow subclass that handles xcsoar:// internal links
 * in addition to http:// and https:// external links.
 *
 * This class lives in the Widget layer (not ui/control) because it
 * depends on Dialogs/InternalLink which is part of the UI layer.
 */
class RichTextWindowWithInternalLinks : public RichTextWindow {
protected:
  bool OnLinkActivated(std::size_t index) noexcept override {
    const auto &links = GetLinks();
    if (index >= links.size())
      return false;

    const char *url = links[index].url.c_str();

    try {
      // Handle internal xcsoar:// links
      if (StringStartsWith(url, "xcsoar://"))
        return HandleInternalLink(url);

      // Handle external URLs and app schemes
      if (IsExternalUriScheme(url))
        return OpenLink(url);
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to activate link");
      return false;
    }

    return false;
  }
};
