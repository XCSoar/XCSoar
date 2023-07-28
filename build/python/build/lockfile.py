import fcntl
from contextlib import contextmanager
from typing import BinaryIO, Generator

@contextmanager
def lockfile(path: str) -> Generator[BinaryIO, None, None]:
    with open(path, 'wb') as f:
        fcntl.flock(f.fileno(), fcntl.LOCK_EX)
        yield f
