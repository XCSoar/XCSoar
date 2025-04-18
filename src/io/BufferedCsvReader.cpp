// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BufferedCsvReader.hpp"

#include <vector>
#include <optional>
#include <stdexcept>

size_t
ReadCsvRecord(BufferedReader &reader, std::span<std::string_view> views) {

  bool prior_return  { false };
  bool prior_quote { false };    // Used to disables double quote on start quote too

  bool field_quoted { false };

  size_t fields_parsed { 0 };

  size_t input_offset { 0 };
  size_t input_shift { 0 };
  const std::byte *field_start { nullptr };  // Set on for sure start
  const std::byte *field_end { nullptr };    // Set on possible end

  std::span<std::byte> input;
  std::byte *input_data_prev { nullptr };
  char input_char { 0 };         // Always initialized if used, but keep compiler happy

  // Reader data available
  while (true) {
    // Get input and shift pointers if required
    input = reader.Read();

    if ( input.data() != input_data_prev ) {
      if ( field_start )
        field_start = input.data() + (field_start - input_data_prev);

      if ( field_end )
        field_end = input.data() + (field_end - input_data_prev);

      for ( auto &view : views.subspan(0,fields_parsed) )
        view = std::string_view(reinterpret_cast<const char*>
                                (input.data() + (reinterpret_cast<const std::byte*>(view.data()) - input_data_prev)),
                                view.size());

      input_data_prev = input.data();
    }

    // Parse input chunk up to end of record
    while ( input_offset < input.size() ) {
      input_char = std::to_integer<char>(input[input_offset]);

      // DOS newline and quoted double quote reduction
      input_shift += ( ( prior_return && input_char == '\n' ) ||
                       ( prior_quote  && input_char == '"' ) );
      if ( input_shift )
        input[input_offset-input_shift] = input[input_offset];

      // Start tagging (set prior_quote so starting quote can't double quote, may be one past end)
      if ( !field_start && input_char != ' ' ) {
        field_quoted = prior_quote = input_char == '"';
        field_start = input.data() + input_offset - input_shift + field_quoted;
      }

      // End tagging
      if ( field_start && !field_end &&
           ( ( !field_quoted && ( input_char == ' ' || input_char == ',' ||
                                  input_char == '\n' ) ) ||
             ( field_quoted  && !prior_quote && input_char == '"' ) ) )
        // Maybe end
        field_end = input.data() + input_offset - input_shift;

      else if ( field_end && ( input_char != ' '  && input_char != ','  &&
                               input_char != '\r' && input_char != '\n' ) )
        // Not end
        field_end = nullptr;

      // Potential DOS newline and quoted double quote
      prior_return = input_char == '\r';
      prior_quote = field_quoted && !prior_quote && input_char == '"';

      // Next character
      ++input_offset;

      // End of field
      if ( field_end && input_char == ',' ) {
        // Viems for field (relative to nullptr for now as reader.Fill can shift data)
        if (fields_parsed < views.size())
          views[fields_parsed] = std::string_view(reinterpret_cast<const char*>(field_start),
                                                  field_end - field_start);
        ++fields_parsed;

        // Next field
        input_shift = 0;
        field_start = nullptr;
        field_end = nullptr;
      }

      // Record end
      if ( field_end && input_char == '\n' )
        break;
    }

    // Handle end of record or input (queues more input on the side if end of input)
    if ( (field_end && input_char == '\n') || !reader.Fill(true) ) {
      // End of file in quotes
      if ( field_quoted && !field_end )
        throw std::runtime_error{"CSV file ended in middle of quoted field"};

      // Start tagging (length may be 0)
      if ( !field_start )
        field_start = input.data() + (input_offset - input_shift);

      // End tagging (length may be 0)
      if ( !field_end )
        field_end = input.data() + (input_offset - input_shift);

      break;
    }
  }

  // End of field
  if ( input_offset && fields_parsed < views.size() ) {
    views[fields_parsed] = std::string_view(reinterpret_cast<const char*>(field_start),
                                            field_end - field_start);
    ++fields_parsed;
  }

  // Zero remaining views (use input.data() as length may be 0)
  for ( auto &view : views.last( views.size() - std::min(fields_parsed,views.size()) ) )
    view = std::string_view(reinterpret_cast<char*>(input.data()) + (input_offset - input_shift), 0);

  // Consume characters
  reader.Consume(input_offset);

  // Return fields parsed
  return fields_parsed;
}
