// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/LX/NanoFileTransfer.hpp"
#include "TestUtil.hpp"

#include <array>
#include <cstring>

using namespace Nano;

static int32_t
Crc(std::string_view s) noexcept
{
  FileTransferCrc crc;
  crc.Update(s);
  return crc.Get();
}

static void
TestCrc()
{
  /* computed with a reference implementation that matched all 7507
     blocks and the file checksum of a Nano 3 download (firmware
     3.00) */
  ok1(Crc("") == 0);
  ok1(Crc("123456789") == -227313381);

  /* standard CRC-32 of "123456789" is 0xcbf43926 (-873187034); the
     sign-extending shift makes this a different checksum */
  ok1(Crc("123456789") != -873187034);

  /* 0xff bytes keep the register at 0xffffffff */
  ok1(Crc("\xff\xff\xff\xff\xff\xff\xff\xff") == 0);

  ok1(Crc("QUJD") == -496861107);

  /* chained: a copy carries the register on to the next block */
  FileTransferCrc chain;
  chain.Update(std::string_view{"QUJD"});
  FileTransferCrc next = chain;
  next.Update(std::string_view{"REVG"});
  ok1(chain.Get() == -496861107);
  ok1(next.Get() == 246082478);
  ok1(next.Get() == Crc("QUJDREVG"));
}

static bool
Decodes(std::string_view src, std::string_view expected)
{
  std::array<std::byte, 16> buffer;
  const auto n = DecodeBase64(src, buffer);
  return n && *n == expected.size() &&
    std::memcmp(buffer.data(), expected.data(), *n) == 0;
}

static bool
Rejects(std::string_view src)
{
  std::array<std::byte, 16> buffer;
  return !DecodeBase64(src, buffer);
}

static void
TestBase64()
{
  ok1(Decodes("", ""));
  ok1(Decodes("QUJD", "ABC"));
  ok1(Decodes("QUI=", "AB"));
  ok1(Decodes("QQ==", "A"));
  ok1(Decodes("QUJDREVG", "ABCDEF"));
  ok1(Decodes("+/8=", "\xfb\xff"));

  ok1(Rejects("QUJ"));
  ok1(Rejects("QU=D"));
  ok1(Rejects("Q==="));
  ok1(Rejects("QQ==QUJD"));
  ok1(Rejects("QU*D"));

  /* does not fit */
  std::array<std::byte, 2> small;
  ok1(!DecodeBase64("QUJD", small));
}

static void
TestParseFileDataBlock()
{
  auto block = ParseFileDataBlock("0,7507,323897820,M0Q2");
  ok1(block && block->number == 7507 && block->crc == 323897820 &&
      block->base64 == "M0Q2");

  block = ParseFileDataBlock("0,0,-350561111,QUxY");
  ok1(block && block->number == 0 && block->crc == -350561111);

  ok1(!ParseFileDataBlock("1,0,-350561111,QUxY"));
  ok1(!ParseFileDataBlock("0,x,-350561111,QUxY"));
  ok1(!ParseFileDataBlock("0,0,99999999999,QUxY"));
  ok1(!ParseFileDataBlock("0,0,-350561111,"));
  ok1(!ParseFileDataBlock("0,0,-350561111"));
}

int
main()
{
  plan_tests(8 + 12 + 7);

  TestCrc();
  TestBase64();
  TestParseFileDataBlock();

  return exit_status();
}
