import os, shutil, subprocess

def untar(tarball_path, parent_path,):
    subprocess.check_call(['tar', 'xfC', tarball_path, parent_path])
