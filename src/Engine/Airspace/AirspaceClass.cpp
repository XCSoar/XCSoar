#include "AirspaceClass.hpp"
#include <assert.h>

const TCHAR *
airspace_class_as_text(const AirspaceClass item,
                       const bool concise)
{
  switch(item) {
  case RESTRICT:
    if (!concise) {
      return _T("Restricted");
    } else {
      return _T("R"); // was LxR
    }
  case PROHIBITED:
    if (!concise) {
      return _T("Prohibited");
    } else {
      return _T("P"); // was LxP 
    }
  case DANGER:
    if (!concise) {
      return _T("Danger Area");
    } else {
      return _T("Q"); // was lxD
    }
  case CLASSA:
    if (!concise) {
      return _T("Class A");
    } else {
      return _T("A");
    }
  case CLASSB:
    if (!concise) {
      return _T("Class B");
    } else {
      return _T("B");
    }
  case CLASSC:
    if (!concise) {
      return _T("Class C");
    } else {
      return _T("C");
    }
  case CLASSD:
    if (!concise) {
      return _T("Class D");
    } else {
      return _T("D");
    }
  case CLASSE:
    if (!concise) {
      return _T("Class E");
    } else {
      return _T("E");
    }
  case CLASSF:
    if (!concise) {
      return _T("Class F");
    } else {
      return _T("F");
    }
  case CLASSG:
    if (!concise) {
      return _T("Class G");
    } else {
      return _T("G");
    }
  case NOGLIDER:
    if (!concise) {
      return _T("No Glider");
    } else {
      return _T("GP"); // was NoGld
    }
  case CTR:
    if (!concise) {
      return _T("CTR");
    } else {
      return _T("CTR");
    }
  case WAVE:
    if (!concise) {
      return _T("Wave");
    } else {
      return _T("W"); // was Wav
    }
  case TMZ:
    if (!concise) {
      return _T("Transponder Mandatory Zone");
    } else {
      return _T("TMZ");
    }
  case AATASK:
    if (!concise) {
      return _T("AAT");
    } else {
      return _T("T");
    }
  default:
    break;
  };
  assert(1);
  return _T("Unknown");
}
