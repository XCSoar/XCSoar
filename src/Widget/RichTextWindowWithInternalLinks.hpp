// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/control/RichTextWindow.hpp"
#include "Dialogs/InternalLink.hpp"
#include "util/UriSchemes.hpp"
#include "system/OpenLink.hpp"

#include <functional>

/**
 * A RichTextWindow subclass that handles xcsoar:// internal links
 * in addition to http:// and https:// external links.
 *
 * This class lives in the Widget layer (not ui/control) because it
 * depends on Dialogs/InternalLink which is part of the UI layer.
 */
class RichTextWindowWithInternalLinks : public RichTextWindow {
  /**
   * Optional callback invoked after an internal (xcsoar://) link
   * dialog closes, so the caller can refresh dynamic content.
   */
  std::function<void()> on_internal_link_return;

public:
  void SetInternalLinkReturnCallback(std::function<void()> cb) noexcept {
    on_internal_link_return = std::move(cb);
  }

protected:
  bool OnLinkActivated(std::size_t index) noexcept override {
    const auto &links = GetLinks();
    if (index >= links.size())
      return false;

    const char *url = links[index].url.c_str();

    // Handle internal xcsoar:// links
    if (StringStartsWith(url, "xcsoar://")) {
      bool result = HandleInternalLink(url);
      /* The subdialog has closed; notify the caller so it
         can refresh any dynamic content. */
      if (on_internal_link_return)
        on_internal_link_return();
      return result;
    }

    // Handle external URLs and app schemes
    if (IsExternalUriScheme(url))
      return OpenLink(url);

    return false;
  }
};
