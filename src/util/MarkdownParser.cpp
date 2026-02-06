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
 * Check for admonition marker: !!! type
 * Returns the AdmonitionType if matched, or -1 if not.
 * On match, *end_out is set past the marker (to '\n' or '\0').
 */
static int
GetAdmonitionType(const char *str, const char **end_out) noexcept
{
  if (str[0] != '!' || str[1] != '!' || str[2] != '!')
    return -1;

  const char *p = str + 3;
  // Skip optional whitespace after !!!
  while (*p == ' ' || *p == '\t')
    ++p;

  struct {
    const char *name;
    AdmonitionType type;
  } static constexpr types[] = {
    {"warning", AdmonitionType::WARNING},
    {"note", AdmonitionType::NOTE},
    {"important", AdmonitionType::IMPORTANT},
    {"tip", AdmonitionType::TIP},
    {"caution", AdmonitionType::CAUTION},
  };

  static_assert(std::size(types) == 5,
                "types array must cover all AdmonitionType values");

  for (const auto &t : types) {
    std::size_t len = strlen(t.name);
    if (strncmp(p, t.name, len) == 0) {
      const char *after = p + len;
      // Must be followed by whitespace, newline, or end of string
      if (*after == '\0' || *after == '\n' ||
          *after == ' ' || *after == '\t') {
        // Skip to end of line
        while (*after != '\0' && *after != '\n')
          ++after;
        *end_out = after;
        return static_cast<int>(t.type);
      }
    }
  }

  return -1;
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
    // Process line-start constructs (admonitions, headings, list items)
    if (at_line_start) {
      // Check for admonition: !!! type
      const char *adm_end;
      int adm_type = GetAdmonitionType(str, &adm_end);
      if (adm_type >= 0) {
        result.admonitions.push_back(
          {result.text.size(),
           static_cast<AdmonitionType>(adm_type)});
        str = adm_end;
        if (*str == '\n')
          ++str;
        at_line_start = true;
        continue;
      }

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

          // Check for inline image: ![alt](url)
          if (*p == '!' && *(p + 1) == '[') {
            const char *alt_begin = p + 2;
            const char *alt_end = alt_begin;

            while (*alt_end != '\0' && *alt_end != ']' &&
                   alt_end < line_end)
              ++alt_end;

            if (*alt_end == ']' && *(alt_end + 1) == '(') {
              const char *img_url_begin = alt_end + 2;
              const char *img_url_end = img_url_begin;

              while (*img_url_end != '\0' && *img_url_end != ')' &&
                     img_url_end < line_end)
                ++img_url_end;

              if (*img_url_end == ')') {
                MarkdownImage image;
                image.position = result.text.size();
                image.alt_text.assign(alt_begin, alt_end);
                image.url.assign(img_url_begin, img_url_end);
                image.is_block = false; // inline in list item

                if (!image.alt_text.empty())
                  result.text.append(image.alt_text);
                else
                  result.text += ' ';

                result.images.push_back(std::move(image));
                p = img_url_end + 1;
                continue;
              }
            }
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

    const bool was_at_line_start = at_line_start;
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

    // Check for image: ![alt](url)
    if (*str == '!' && *(str + 1) == '[') {
      const char *alt_begin = str + 2;
      const char *alt_end = alt_begin;

      while (*alt_end != '\0' && *alt_end != ']' &&
             *alt_end != '\n')
        ++alt_end;

      if (*alt_end == ']' && *(alt_end + 1) == '(') {
        const char *img_url_begin = alt_end + 2;
        const char *img_url_end = img_url_begin;

        while (*img_url_end != '\0' && *img_url_end != ')' &&
               *img_url_end != '\n')
          ++img_url_end;

        if (*img_url_end == ')') {
          // Check if this image is the sole content on its line
          // (block image): nothing before it on the line and
          // nothing after except whitespace/newline
          const char *after = img_url_end + 1;
          bool trailing_empty = (*after == '\0' || *after == '\n');
          bool is_block = was_at_line_start && trailing_empty;

          MarkdownImage image;
          image.position = result.text.size();
          image.alt_text.assign(alt_begin, alt_end);
          image.url.assign(img_url_begin, img_url_end);
          image.is_block = is_block;

          // Add placeholder text (alt text or single space)
          if (!image.alt_text.empty())
            result.text.append(image.alt_text);
          else
            result.text += ' ';

          result.images.push_back(std::move(image));

          str = img_url_end + 1;
          continue;
        }
      }
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
