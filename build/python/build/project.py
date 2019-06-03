import os, shutil
import re

from build.download import download_and_verify
from build.tar import untar
from build.quilt import push_all

class Project:
    def __init__(self, toolchain, url, alternative_url, md5, installed, name=None, version=None,
                 base=None,
                 patches=None):
        self.toolchain = toolchain
        
        if base is None:
            basename = os.path.basename(url)
            m = re.match(r'^(.+)\.(tar(\.(gz|bz2|xz|lzma))?|zip)$', basename)
            if not m: raise
            self.base = m.group(1)
        else:
            self.base = base

        if name is None or version is None:
            m = re.match(r'^([-\w]+)-(\d[\d.]*[a-z]?[\d.]*)$', self.base)
            if name is None: name = m.group(1)
            if version is None: version = m.group(2)

        self.name = name
        self.version = version

        self.url = url
        self.alternative_url = alternative_url
        self.md5 = md5
        self.installed = installed

        self.patches = patches

    def download(self):
        return download_and_verify(self.url, self.alternative_url, self.md5, self.toolchain.tarball_path)

    def is_installed(self):
        tarball = self.download()
        installed = os.path.join(self.toolchain.install_prefix, self.installed)
        tarball_mtime = os.path.getmtime(tarball)
        try:
            return os.path.getmtime(installed) >= tarball_mtime
        except FileNotFoundError:
            return False

    def unpack(self, out_of_tree=True):
        if out_of_tree:
            parent_path = self.toolchain.src_path
        else:
            parent_path = self.toolchain.build_path
        path = untar(self.download(), parent_path, self.base)
        if self.patches is not None:
            push_all(self.toolchain,path, self.patches)
        return path

    def make_build_path(self):
        path = os.path.join(self.toolchain.build_path, self.base)
        try:
            shutil.rmtree(path)
        except FileNotFoundError:
            pass
        os.makedirs(path, exist_ok=True)
        return path
