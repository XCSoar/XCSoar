import fcntl
from contextlib import contextmanager

@contextmanager
def lockfile(path):
    with open(path, 'w') as f:
        fcntl.flock(f.fileno(), fcntl.LOCK_EX)
        yield f
