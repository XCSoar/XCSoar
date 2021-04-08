#include "system/Args.hpp"
#include "Task/TaskFile.hpp"
#include "util/PrintException.hxx"

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "FILE.tsk|cup|igc");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  const auto file = TaskFile::Create(path);
  if (!file) {
    fprintf(stderr, "TaskFile::Create() failed\n");
    return EXIT_FAILURE;
  }

  for (const auto &i : file->GetList())
    _tprintf(_T("task: %s\n"), i.c_str());

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}

