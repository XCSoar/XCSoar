import os, shutil, subprocess

def untar(tarball_path, parent_path, base, lazy=False):
    path = os.path.join(parent_path, base)
    if lazy and os.path.isdir(path):
        return path
    try:
        shutil.rmtree(path)
    except FileNotFoundError:
        pass
    os.makedirs(parent_path, exist_ok=True)
    subprocess.check_call(['tar', 'xfC', tarball_path, parent_path])
    return path
