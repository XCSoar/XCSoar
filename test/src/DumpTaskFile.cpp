#include "OS/Args.hpp"
#include "Task/TaskFile.hpp"

#include <memory>

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.tsk|cup|igc");
  const auto path = args.ExpectNextPath();
  args.ExpectEnd();

  std::unique_ptr<TaskFile> file(TaskFile::Create(path));
  if (!file) {
    fprintf(stderr, "TaskFile::Create() failed\n");
    return EXIT_FAILURE;
  }

  unsigned count = file->Count();
  printf("Number of tasks: %u\n---\n", count);

  for (unsigned i = 0; i < count; ++i) {
    const TCHAR *saved_name = file->GetName(i);
    _tprintf(_T("%u: %s\n"), i, saved_name != NULL ? saved_name : _T(""));
  }

  return EXIT_SUCCESS;
}

