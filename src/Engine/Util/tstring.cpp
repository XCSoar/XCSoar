#include "tstring.hpp"

tstring
trim(const tstring& StrToTrim)
{
  tstring TrimmedString;

#define WHITESPACE _T(" \r\t")

  // Find first non whitespace char in StrToTrim
  tstring::size_type First = StrToTrim.find_first_not_of(WHITESPACE);

  // Check whether something went wrong?
  if( First == tstring::npos )
  {
    First = 0;
  }

  // Find last non whitespace char from StrToTrim
  tstring::size_type Last = StrToTrim.find_last_not_of(WHITESPACE);
  // If something didn't go wrong, Last will be recomputed to get real length of substring
  if( Last != tstring::npos )
  {
    Last = ( Last + 1 ) - First;
  }

  // Copy such a string to TrimmedString
  TrimmedString = StrToTrim.substr( First, Last );
  return TrimmedString;
}
