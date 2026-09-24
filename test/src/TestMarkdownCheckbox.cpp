// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/MarkdownParser.hpp"
#include "TestUtil.hpp"

static void
TestEmpty()
{
  const auto parsed = ParseMarkdown("");
  std::vector<uint8_t> toggled;
  const auto checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked.empty());
  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, checked));
}

static void
TestToggleRoundTrip()
{
  const auto parsed = ParseMarkdown("- [ ] one\n- [ ] two\n");
  std::vector<uint8_t> toggled;
  auto checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked.size() == 2);
  ok1(checked[0] == 0);
  ok1(checked[1] == 0);

  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, {1, 0}));
  checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked.size() == 2);
  ok1(checked[0] == 1);
  ok1(checked[1] == 0);

  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, {1, 1}));
  checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked[0] == 1);
  ok1(checked[1] == 1);

  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, {0, 0}));
  checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked[0] == 0);
  ok1(checked[1] == 0);
}

static void
TestOriginalCheckedAndMismatch()
{
  const auto parsed = ParseMarkdown("- [x] done\n- [ ] todo\n");
  std::vector<uint8_t> toggled;
  auto checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked.size() == 2);
  ok1(checked[0] == 1);
  ok1(checked[1] == 0);

  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, {0, 1}));
  checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked[0] == 0);
  ok1(checked[1] == 1);

  const auto before = toggled;
  ok1(!ApplyMarkdownCheckboxStates(parsed, toggled, {1}));
  ok1(toggled == before);
  ok1(!ApplyMarkdownCheckboxStates(parsed, toggled, {0, 1, 1}));
  ok1(toggled == before);
}

static void
TestDocumentOrderWithOtherMarkup()
{
  const auto parsed = ParseMarkdown(
    "# Head\n"
    "- [ ] first **bold**\n"
    "- item\n"
    "- [x] second [link](https://xcsoar.org)\n");
  std::vector<uint8_t> toggled;
  auto checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked.size() == 2);
  ok1(checked[0] == 0);
  ok1(checked[1] == 1);

  ok1(ApplyMarkdownCheckboxStates(parsed, toggled, {1, 0}));
  checked = ReadMarkdownCheckboxStates(parsed, toggled);
  ok1(checked[0] == 1);
  ok1(checked[1] == 0);
}

int
main()
{
  plan_tests(2 + 13 + 10 + 6);

  TestEmpty();
  TestToggleRoundTrip();
  TestOriginalCheckedAndMismatch();
  TestDocumentOrderWithOtherMarkup();

  return exit_status();
}
