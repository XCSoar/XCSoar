/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_IGC_HPP
#define XCSOAR_IMI_IGC_HPP

#include "Types.hpp"

#include <tchar.h>
#include <cstdio>

namespace IMI
{
  void WriteHeader(const TDeclaration &decl, IMIBYTE tampered, FILE *file);
  void WriteFix(const Fix &fix, bool fromB2, int no_enl, FILE *file);
  void WriteSignature(const Signature &sig, IMIWORD sn, FILE *file);
};

#endif
