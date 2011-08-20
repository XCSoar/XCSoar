#ifndef AIRSPACE_CLASS_HPP
#define AIRSPACE_CLASS_HPP

#include "Compiler.h"

#include <tchar.h>

enum AirspaceClass 
{
  OTHER = 0,
  RESTRICT,
  PROHIBITED,
  DANGER,
  CLASSA,
  CLASSB,
  CLASSC,
  CLASSD,
  NOGLIDER,
  CTR,
  WAVE,
  AATASK,
  CLASSE,
  CLASSF,
  TMZ,
  CLASSG,
  AIRSPACECLASSCOUNT
};

gcc_const
const TCHAR *
airspace_class_as_text(const AirspaceClass item,
                       const bool consise=false);

#endif
