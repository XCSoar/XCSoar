#include "OS/Args.hpp"
#include "Task/TaskFile.hpp"

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.tsk|cup|igc");
  tstring path = args.ExpectNextT();
  args.ExpectEnd();

  TaskFile *file = TaskFile::Create(path.c_str());
  if (file == NULL) {
    fprintf(stderr, "TaskFile::Create() failed\n");
    return EXIT_FAILURE;
  }

  unsigned count = file->Count();
  printf("Number of tasks: %u\n---\n", count);

  for (unsigned i = 0; i < count; ++i) {
    const TCHAR *saved_name = file->GetName(i);
    _tprintf(_T("%u: %s\n"), i, saved_name != NULL ? saved_name : _T(""));
  }

  delete file;
  return EXIT_SUCCESS;
}

