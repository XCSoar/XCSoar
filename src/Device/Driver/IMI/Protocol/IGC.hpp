// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Types.hpp"

class BufferedOutputStream;

namespace IMI {

void WriteHeader(BufferedOutputStream &os, const TDeclaration &decl, IMIBYTE tampered);
void WriteFix(BufferedOutputStream &os, const Fix &fix, bool fromB2, int no_enl);
void WriteSignature(BufferedOutputStream &os, const Signature &sig, IMIWORD sn);

} // namespace IMI
