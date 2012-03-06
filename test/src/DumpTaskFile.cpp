#include "OS/Args.hpp"
#include "Task/TaskFile.hpp"

int
main(int argc, char **argv)
{
  Args args(argc, argv, "FILE.tsk|cup");
  tstring path = args.ExpectNextT();
  args.ExpectEnd();

  TaskFile *file = TaskFile::Create(path.c_str());
  if (file == NULL) {
    fprintf(stderr, "TaskFile::Create() failed\n");
    return EXIT_FAILURE;
  }

  printf("count=%u\n", file->Count());
  delete file;
  return EXIT_SUCCESS;
}

