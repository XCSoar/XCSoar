#include "AirspaceClass.hpp"
#include "Util/Macros.hpp"

static const TCHAR *airspace_class_names[] = {
  _T("Unknown"),
  _T("Restricted"),
  _T("Prohibited"),
  _T("Danger Area"),
  _T("Class A"),
  _T("Class B"),
  _T("Class C"),
  _T("Class D"),
  _T("No Gliders"),
  _T("CTR"),
  _T("Wave"),
  _T("Task Area"),
  _T("Class E"),
  _T("Class F"),
  _T("Transponder Mandatory Zone"),
  _T("Class G"),
};

static const TCHAR *airspace_class_short_names[] = {
  _T("?"),
  _T("R"),
  _T("P"),
  _T("Q"),
  _T("A"),
  _T("B"),
  _T("C"),
  _T("D"),
  _T("GP"),
  _T("CTR"),
  _T("W"),
  _T("AAT"),
  _T("E"),
  _T("F"),
  _T("TMZ"),
  _T("G"),
};

const TCHAR *
airspace_class_as_text(const AirspaceClass item, const bool concise)
{
  unsigned i = (unsigned)item;

  if (!concise)
    return i < ARRAY_SIZE(airspace_class_names) ?
           airspace_class_names[i] : NULL;

  return i < ARRAY_SIZE(airspace_class_short_names) ?
         airspace_class_short_names[i] : NULL;
}
