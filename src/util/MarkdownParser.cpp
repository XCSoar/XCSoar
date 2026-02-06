// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MarkdownParser.hpp"
#include "util/UriSchemes.hpp"

#include <cstring>

/**
 * Check if character can be part of a URL.
 * Includes unreserved characters, reserved characters, and sub-delimiters
 * per RFC 3986.
 */
[[gnu::const]]
static constexpr bool
IsUrlChar(char c) noexcept
{
  // Unreserved: A-Z a-z 0-9 - . _ ~
  // Reserved gen-delims: : / ? # [ ] @
  // Reserved sub-delims: ! $ & ' ( ) * + , ; =
  // Also % for percent-encoding
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '-' || c == '_' || c == '.' || c == '~' ||
         c == ':' || c == '/' || c == '?' || c == '#' ||
         c == '[' || c == ']' || c == '@' ||
         c == '!' || c == '$' || c == '&' || c == '\'' ||
         c == '(' || c == ')' || c == '*' || c == '+' ||
         c == ',' || c == ';' || c == '=' ||
         c == '%';
}

/**
 * Check if we're at a bold marker (** or __).
 * @return Length of the bold marker (2) or 0 if not a bold marker
 */
[[gnu::pure]]
static unsigned
GetBoldMarkerLength(const char *str) noexcept
{
  if ((str[0] == '*' && str[1] == '*') ||
      (str[0] == '_' && str[1] == '_'))
    return 2;
  return 0;
}

/**
 * Find the end of the current line.
 */
static const char *
FindLineEnd(const char *str) noexcept
{
  while (*str != '\0' && *str != '\n')
    ++str;
  return str;
}

/**
 * Count heading level (# characters at start).
 */
static unsigned
GetHeadingLevel(const char *str) noexcept
{
  unsigned level = 0;
  while (*str == '#' && level < 6) {
    ++level;
    ++str;
  }
  // Must be followed by space or end of line
  if (level > 0 && *str != ' ' && *str != '\t' &&
      *str != '\n' && *str != '\0')
    return 0;
  return level;
}

/**
 * Check if line starts with list marker (- or *).
 */
static bool
IsListItem(const char *str) noexcept
{
  // Skip leading whitespace
  while (*str == ' ' || *str == '\t')
    ++str;
  // Check for - or * followed by space
  return (*str == '-' || *str == '*') &&
         (str[1] == ' ' || str[1] == '\t');
}

/**
 * Check for checkbox marker after list marker: [ ] or [x] or [X]
 * Returns: 0 = not a checkbox, 1 = unchecked, 2 = checked
 */
static int
GetCheckboxState(const char *str) noexcept
{
  if (str[0] != '[')
    return 0;
  if (str[1] == ' ' && str[2] == ']' &&
      (str[3] == ' ' || str[3] == '\t' ||
       str[3] == '\n' || str[3] == '\0'))
    return 1;  // Unchecked: [ ]
  if ((str[1] == 'x' || str[1] == 'X') && str[2] == ']' &&
      (str[3] == ' ' || str[3] == '\t' ||
       str[3] == '\n' || str[3] == '\0'))
    return 2;  // Checked: [x] or [X]
  return 0;
}

/**
 * Skip list marker and return pointer to content.
 */
static const char *
SkipListMarker(const char *str) noexcept
{
  while (*str == ' ' || *str == '\t')
    ++str;
  if (*str == '-' || *str == '*')
    ++str;
  if (*str == ' ' || *str == '\t')
    ++str;
  return str;
}

ParsedMarkdown
ParseMarkdown(const char *input)
{
  ParsedMarkdown result;

  if (input == nullptr || *input == '\0')
    return result;

  result.text.reserve(strlen(input));

  const char *str = input;
  bool at_line_start = true;
  bool in_bold = false;
  std::size_t bold_start = 0;

  while (*str != '\0') {
    // Process line-start constructs (headings, list items)
    if (at_line_start) {
      // Check for heading
      unsigned heading_level = GetHeadingLevel(str);
      if (heading_level > 0) {
        // Skip # characters and whitespace
        str += heading_level;
        while (*str == ' ' || *str == '\t')
          ++str;

        // Find end of heading line
        const char *line_end = FindLineEnd(str);
        std::size_t heading_start = result.text.size();

        // Copy heading content
        result.text.append(str, line_end);

        // Add heading style span
        TextStyle style = (heading_level == 1) ? TextStyle::Heading1 :
                         (heading_level == 2) ? TextStyle::Heading2 :
                                                TextStyle::Heading3;
        result.styles.push_back({heading_start, result.text.size(), style});

        // Skip to end of line
        str = line_end;
        if (*str == '\n') {
          result.text += '\n';
          ++str;
        }
        at_line_start = true;
        continue;
      }

      // Check for list item
      if (IsListItem(str)) {
        const char *content = SkipListMarker(str);
        const char *line_end = FindLineEnd(str);

        // Check for checkbox: [ ] or [x]
        int checkbox_state = GetCheckboxState(content);
        std::size_t marker_start = result.text.size();

        if (checkbox_state > 0) {
          // Add placeholder for checkbox (will be rendered as graphic)
          result.text += "  ";  // Two spaces as placeholder
          result.styles.push_back({marker_start, marker_start + 2,
            checkbox_state == 2 ? TextStyle::CheckboxChecked : TextStyle::Checkbox});
          // Skip past the checkbox marker "[ ]" or "[x]"
          content += 3;
          // Skip trailing space/tab if present (but not past end of string)
          if (*content == ' ' || *content == '\t')
            ++content;
        } else {
          // Regular list item - add bullet character
          result.text += "- ";
          result.styles.push_back({marker_start, marker_start + 2, TextStyle::ListItem});
        }

        // Find line end from content start
        line_end = FindLineEnd(content);

        // Now process the content for inline formatting (bold, links)
        const char *p = content;

        while (p < line_end) {
          // Check for bold marker
          unsigned bold_len = GetBoldMarkerLength(p);
          if (bold_len > 0) {
            if (!in_bold) {
              in_bold = true;
              bold_start = result.text.size();
            } else {
              result.styles.push_back({bold_start, result.text.size(), TextStyle::Bold});
              in_bold = false;
            }
            p += bold_len;
            continue;
          }

          // Check for markdown link: [text](url)
          if (*p == '[') {
            const char *link_text_begin = p + 1;
            const char *link_text_end = link_text_begin;

            while (*link_text_end != '\0' && *link_text_end != ']' &&
                   link_text_end < line_end)
              ++link_text_end;

            if (*link_text_end == ']' && *(link_text_end + 1) == '(') {
              const char *url_begin = link_text_end + 2;
              const char *url_end = url_begin;

              while (*url_end != '\0' && *url_end != ')' &&
                     url_end < line_end)
                ++url_end;

              if (*url_end == ')') {
                MarkdownLink link;
                link.start = result.text.size();
                link.display_text.assign(link_text_begin, link_text_end);
                link.url.assign(url_begin, url_end);
                link.end = link.start + link.display_text.size();

                result.text.append(link.display_text);
                result.links.push_back(std::move(link));

                p = url_end + 1;
                continue;
              }
            }
          }

          // Check for raw URL
          const UriScheme *scheme = FindUriScheme(p);
          if (scheme != nullptr) {
            const char *url_start = p;

            // Find end of URL (bounded by line_end)
            const char *url_end = url_start;
            while (url_end < line_end && *url_end != '\0' && IsUrlChar(*url_end))
              ++url_end;

            // Remove trailing punctuation that's unlikely part of URL
            while (url_end > url_start) {
              char last = *(url_end - 1);
              if (last == '.' || last == ',' ||
                  last == ')' || last == ']')
                --url_end;
              else
                break;
            }

            // Only treat as URL if there's content after the scheme
            std::size_t url_len = url_end - url_start;
            if (url_len > scheme->length) {
              MarkdownLink link;
              link.start = result.text.size();
              link.display_text.assign(url_start, url_end);
              link.url.assign(url_start, url_end);
              link.end = link.start + link.display_text.size();

              result.text.append(link.display_text);
              result.links.push_back(std::move(link));

              p = url_end;
              continue;
            }
          }

          result.text += *p++;
        }

        str = line_end;
        if (*str == '\n') {
          result.text += '\n';
          ++str;
        }
        at_line_start = true;
        continue;
      }
    }

    // Check for newline
    if (*str == '\n') {
      // Reset unclosed bold at line boundary
      if (in_bold) {
        result.styles.push_back({bold_start, result.text.size(), TextStyle::Bold});
        in_bold = false;
      }
      result.text += *str++;
      at_line_start = true;
      continue;
    }

    at_line_start = false;

    // Check for bold marker
    unsigned bold_len = GetBoldMarkerLength(str);
    if (bold_len > 0) {
      if (!in_bold) {
        in_bold = true;
        bold_start = result.text.size();
      } else {
        result.styles.push_back({bold_start, result.text.size(), TextStyle::Bold});
        in_bold = false;
      }
      str += bold_len;
      continue;
    }

    // Look for markdown link: [text](url)
    if (*str == '[') {
      const char *link_text_begin = str + 1;
      const char *link_text_end = link_text_begin;

      // Find closing ]
      while (*link_text_end != '\0' && *link_text_end != ']' &&
             *link_text_end != '\n')
        ++link_text_end;

      if (*link_text_end == ']' && *(link_text_end + 1) == '(') {
        // Found [text], now look for (url)
        const char *url_begin = link_text_end + 2;
        const char *url_end = url_begin;

        // Find closing )
        while (*url_end != '\0' && *url_end != ')' &&
               *url_end != '\n')
          ++url_end;

        if (*url_end == ')') {
          // Valid markdown link
          MarkdownLink link;
          link.start = result.text.size();
          link.display_text.assign(link_text_begin, link_text_end);
          link.url.assign(url_begin, url_end);
          link.end = link.start + link.display_text.size();

          // Add display text to processed text
          result.text.append(link.display_text);
          result.links.push_back(std::move(link));

          // Move past the markdown link
          str = url_end + 1;
          continue;
        }
      }
    }

    // Look for raw URL prefixes
    const UriScheme *scheme = FindUriScheme(str);

    if (scheme != nullptr) {
      const char *url_start = str;

      // Find end of URL
      const char *url_end = url_start;
      while (*url_end != '\0' && IsUrlChar(*url_end))
        ++url_end;

      // Remove trailing punctuation that's unlikely part of URL
      while (url_end > url_start) {
        char last = *(url_end - 1);
        if (last == '.' || last == ',' ||
            last == ')' || last == ']')
          --url_end;
        else
          break;
      }

      // Only treat as URL if there's content after the scheme
      std::size_t url_len = url_end - url_start;
      if (url_len > scheme->length) {
        MarkdownLink link;
        link.start = result.text.size();
        link.display_text.assign(url_start, url_end);
        link.url.assign(url_start, url_end);
        link.end = link.start + link.display_text.size();

        result.text.append(link.display_text);
        result.links.push_back(std::move(link));

        str = url_end;
        continue;
      }
    }

    // Regular character
    result.text += *str++;
  }

  // Flush any unclosed bold span at end of input
  if (in_bold) {
    result.styles.push_back(
      {bold_start, result.text.size(), TextStyle::Bold});
  }

  return result;
}
