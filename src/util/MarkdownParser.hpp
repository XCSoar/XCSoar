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
 * A parsed image within text: ![alt text](url)
 */
struct MarkdownImage {
  /** Character offset where image placeholder starts in processed text */
  std::size_t position;

  /** The alt text for the image */
  std::string alt_text;

  /** The image URL (file path or "resource:IDB_NAME") */
  std::string url;

  /** True if the image is the sole content on its line (block image) */
  bool is_block;
};

/**
 * Admonition types for !!! blocks.
 *
 * Syntax:  !!! type
 * (on its own line, followed by content)
 *
 * Supported types: warning, note, important, tip, caution.
 * The admonition marker is consumed by the parser and
 * colours the next heading according to the type.
 */
enum class AdmonitionType : uint8_t {
  WARNING,
  NOTE,
  IMPORTANT,
  TIP,
  CAUTION,
};

/**
 * An admonition marker parsed from a !!! line.
 * Headings that immediately follow (within a few characters)
 * are rendered in the colour associated with this type.
 */
struct Admonition {
  /** Character offset in processed text where the marker sits */
  std::size_t position;

  AdmonitionType type;
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

  /** Images found in the text */
  std::vector<MarkdownImage> images;

  /** Admonition markers from !!! lines */
  std::vector<Admonition> admonitions;
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
 * - Admonitions: !!! warning, !!! note, !!! important, !!! tip,
 *   !!! caution (at line start; colours the next heading)
 *
 * @param input The raw text to parse
 * @return Processed text with extracted links and style spans
 */
[[gnu::pure]]
ParsedMarkdown
ParseMarkdown(const char *input);
