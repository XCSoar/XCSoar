// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

/**
 * Text styling types for Markdown rendering.
 */
enum class TextStyle : uint8_t {
  Normal,
  Bold,
  Heading1,
  Heading2,
  Heading3,
  ListItem,
  Checkbox,         ///< Unchecked checkbox: - [ ]
  CheckboxChecked,  ///< Checked checkbox: - [x]
  COUNT             ///< Sentinel for array sizing
};

/**
 * A styled span within text (bold, heading, etc.).
 */
struct StyledSpan {
  /** Character offset where span starts in processed text */
  std::size_t start;

  /** Character offset where span ends (exclusive) */
  std::size_t end;

  /** The style applied to this span */
  TextStyle style;
};

/**
 * A parsed link within text.
 */
struct MarkdownLink {
  /** Character offset where link starts in processed text */
  std::size_t start;

  /** Character offset where link ends (exclusive) */
  std::size_t end;

  /** The display text (for markdown links, differs from URL) */
  std::string display_text;

  /** The URL to open when activated */
  std::string url;
};

/**
 * Result of parsing Markdown text.
 */
struct ParsedMarkdown {
  /** The processed text with markdown syntax removed */
  std::string text;

  /** Links found in the text */
  std::vector<MarkdownLink> links;

  /** Style spans (bold, headings, list items) */
  std::vector<StyledSpan> styles;
};

/**
 * Parse text for Markdown formatting.
 *
 * Supports:
 * - Bold: **text** or __text__
 * - Headings: # H1, ## H2, ### H3 (at line start)
 * - List items: - item or * item (at line start)
 * - Checkboxes: - [ ] unchecked, - [x] checked (at line start)
 * - Markdown links: [display text](url)
 * - Raw URLs: http://, https://, xcsoar://
 *
 * @param input The raw text to parse
 * @return Processed text with extracted links and style spans
 */
[[gnu::pure]]
ParsedMarkdown
ParseMarkdown(const char *input);
