// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget.hpp"
#include "ui/dim/Rect.hpp"

#include <functional>
#include <memory>
#include <string>

struct DialogLook;
class Button;
class CheckBoxControl;

/**
 * A composite widget for onboarding dialog pages.
 *
 * Provides a scrollable markdown content area with an optional
 * bottom bar containing either a checkbox or one/two buttons.
 * All pages share a uniform visual appearance.
 *
 * The bottom bar controls use standard control heights from
 * Layout and are positioned at the bottom of the allocated
 * rectangle.
 */
class OnboardingPageWidget : public NullWidget {
public:
  /** Variant types for the bottom bar */
  enum class BottomBarType : uint8_t {
    NONE,           ///< Content only, no bottom bar
    CHECKBOX,       ///< Single checkbox
    ONE_BUTTON,     ///< Single button
    TWO_BUTTONS,    ///< Two side-by-side buttons
  };

private:
  const DialogLook &look;
  std::string markdown_text;

  BottomBarType bar_type = BottomBarType::NONE;

  /** The scrollable markdown content widget */
  std::unique_ptr<Widget> scroll_widget;

  /** Optional checkbox in bottom bar */
  std::unique_ptr<CheckBoxControl> checkbox;

  /** Optional buttons in bottom bar */
  std::unique_ptr<Button> button1;
  std::unique_ptr<Button> button2;

  // Configuration for deferred creation
  std::string checkbox_label;
  std::function<void(bool)> checkbox_callback;
  bool checkbox_initial_state = false;

  std::string button1_label;
  std::function<void()> button1_callback;

  std::string button2_label;
  std::function<void()> button2_callback;

  bool visible = false;

  /** Whether to parse markdown links in the content text */
  bool parse_links = true;

  /**
   * Optional callback for horizontal swipe gestures, forwarded
   * to the internal VScrollWidget.
   */
  std::function<void(bool)> gesture_callback;

public:
  explicit OnboardingPageWidget(const DialogLook &_look,
                                const char *_markdown_text) noexcept;

  /**
   * Disable link parsing in the content text.
   * Must be called before Initialise().
   */
  void SetParseLinks(bool enabled) noexcept {
    parse_links = enabled;
  }



  /**
   * Set a callback for horizontal swipe gestures.
   * Must be called before Initialise().
   *
   * @param cb Called with true for swipe-left (next page),
   *           false for swipe-right (previous page)
   */
  void SetGestureCallback(std::function<void(bool)> cb) noexcept {
    gesture_callback = std::move(cb);
  }
  ~OnboardingPageWidget() noexcept override;

  /**
   * Create a content-only page (no bottom bar).
   */
  static std::unique_ptr<OnboardingPageWidget>
  CreateContentPage(const DialogLook &look,
                    const char *markdown_text) noexcept;

  /**
   * Create a page with a checkbox in the bottom bar.
   */
  static std::unique_ptr<OnboardingPageWidget>
  CreateCheckboxPage(const DialogLook &look,
                     const char *markdown_text,
                     const char *checkbox_label,
                     bool initial_state,
                     std::function<void(bool)> callback) noexcept;

  /**
   * Create a page with a single button in the bottom bar.
   */
  static std::unique_ptr<OnboardingPageWidget>
  CreateButtonPage(const DialogLook &look,
                   const char *markdown_text,
                   const char *button_label,
                   std::function<void()> callback) noexcept;

  /**
   * Create a page with two buttons in the bottom bar.
   */
  static std::unique_ptr<OnboardingPageWidget>
  CreateTwoButtonPage(const DialogLook &look,
                      const char *markdown_text,
                      const char *button1_label,
                      std::function<void()> button1_cb,
                      const char *button2_label,
                      std::function<void()> button2_cb) noexcept;

  /**
   * Get checkbox state. Only valid for CHECKBOX type.
   */
  [[gnu::pure]]
  bool GetCheckboxState() const noexcept;

  /**
   * Set checkbox state. Only valid for CHECKBOX type.
   */
  void SetCheckboxState(bool value) noexcept;

  /**
   * Update the displayed markdown text.
   */
  void SetText(const char *text) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Initialise(ContainerWindow &parent,
                  const PixelRect &rc) noexcept override;
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;
  bool HasFocus() const noexcept override;
  bool KeyPress(unsigned key_code) noexcept override;

private:
  /**
   * Calculate the layout: split rect into content area and
   * optional bottom bar.
   */
  struct PageLayout {
    PixelRect content;
    PixelRect bottom_bar;
    bool has_bottom_bar;
  };

  [[gnu::pure]]
  PageLayout CalcLayout(const PixelRect &rc) const noexcept;

  /**
   * Create the bottom bar controls.
   */
  void CreateBottomBar(ContainerWindow &parent,
                       const PixelRect &rc) noexcept;

  /**
   * Position (and optionally show) the bottom bar controls.
   *
   * @param bar_rect Rectangle for the bottom bar area
   * @param show If true, controls are moved and shown;
   *             if false, only moved
   */
  void PositionBottomBar(const PixelRect &bar_rect,
                         bool show) noexcept;
};
