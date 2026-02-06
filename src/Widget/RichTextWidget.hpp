// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <functional>
#include <string>

struct DialogLook;

/**
 * A widget that displays multi-line text with Markdown formatting.
 *
 * Supported Markdown features:
 * - Bold: **text** or __text__
 * - Headings: # H1, ## H2, ### H3
 * - List items: - item or * item
 * - Checkboxes: - [ ] unchecked, - [x] checked
 * - Links: [display text](url)
 * - Raw URLs: http://, https://, xcsoar://
 *
 * Link handling:
 * - http:// and https:// URLs open in browser
 * - xcsoar:// URLs trigger internal dialogs (see Dialogs/InternalLink.hpp)
 *
 * Use with VScrollWidget for scrolling support.
 */
class RichTextWidget : public WindowWidget {
  const DialogLook *look = nullptr;
  std::string text;
  bool parse_links = true;
  std::function<void()> link_return_callback;

public:
  /**
   * Default constructor for subclasses that provide look via GetLook().
   */
  RichTextWidget() noexcept = default;

  /**
   * Constructor with explicit look and optional text.
   *
   * @param _look The dialog look for styling
   * @param _text The text to display (may be nullptr)
   * @param _parse_markdown If true (default), Markdown formatting is
   *                        parsed and rendered. Set to false for
   *                        large texts where formatting is not needed.
   */
  explicit RichTextWidget(const DialogLook &_look,
                          const char *_text = nullptr,
                          bool _parse_links = true) noexcept
    : look(&_look), text(_text != nullptr ? _text : ""),
      parse_links(_parse_links) {}

  /**
   * Update the displayed text.
   */
  void SetText(const char *text) noexcept;

  /**
   * Set a callback invoked after an internal (xcsoar://) link dialog
   * closes, allowing the caller to refresh dynamic content.
   * Must be called before Prepare().
   */
  void SetLinkReturnCallback(std::function<void()> cb) noexcept {
    link_return_callback = std::move(cb);
  }

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

protected:
  /**
   * Get the dialog look for styling.
   * Subclasses can override to provide their own look.
   */
  [[gnu::pure]]
  virtual const DialogLook &GetLook() const noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
};
