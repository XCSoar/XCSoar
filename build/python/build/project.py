import os, shutil
import re
import pathlib

from build.download import download_and_verify
from build.tar import untar
from build.quilt import push_all
from build.verify import verify_file_digest

class Project:
    def __init__(self, toolchain, url, alternative_url, md5, installed, name=None, version=None,
                 base=None,
                 patches=None,
                 out_of_tree=True):
        self.toolchain = toolchain
        
        basename = os.path.basename(url)
        if base is None:
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
        self.out_of_tree = out_of_tree
        
        # Various paths and files which were built implicitly in the download and unpack methods.
        self.tarball = os.path.join(self.toolchain.tarball_path, basename)
        if self.out_of_tree:
            self.unpack_base_path = self.toolchain.src_path
        else:
            self.unpack_base_path = self.toolchain.build_path

        # src_dir is at the same time the unpack dir of the tarball"
        self.src_dir = os.path.join(self.unpack_base_path,self.base)
        self.unpack_stamp_file = self.src_dir + '-stamp'
        self.build_dir = os.path.join(self.toolchain.build_path, self.base)
        # cached status flags. Uninitialized here because determining them is expensive
        self.is_installed = None
        self.is_unpacked = None
        self.is_downloaded = None

    def download(self):
        download_and_verify(self.url, self.alternative_url, self.md5, self.tarball)
        self.is_downloaded = True

    def check_installed(self):
        result = False
        if self.check_unpacked():
            installed = os.path.join(self.toolchain.install_prefix, self.installed)
            unpack_mtime = os.path.getmtime(self.unpack_stamp_file)
            try:
                result = os.path.getmtime(installed) > unpack_mtime
            except FileNotFoundError:
                pass
        return result
    
    def check_unpacked(self):
        if self.is_unpacked is None:
            self.is_unpacked = False
            if self.check_downloaded():
                # The tarball surely exists
                mtime_tarball = os.path.getmtime(self.tarball)
                # The unpack timestamp may not yet exist.
                try:
                    mtime_unpack_stamp = os.path.getmtime(self.unpack_stamp_file)
                    # if unpacking happened later than downloading the unpacked source is up-to-date.
                    self.is_unpacked = mtime_tarball < mtime_unpack_stamp
                except FileNotFoundError:
                    pass
        return self.is_unpacked

    def check_downloaded(self):
        if self.is_downloaded is None:
            self.is_downloaded = False
            try:
                self.is_downloaded = verify_file_digest(self.tarball, self.md5)
            except FileNotFoundError:
                pass
        return self.is_downloaded


    def unpack(self):
        try:
            os.remove(self.unpack_stamp_file)
        except FileNotFoundError:
            pass
        try:
            shutil.rmtree(self.src_dir)
        except FileNotFoundError:
            pass
        untar(self.tarball, self.unpack_base_path)
        # Check if the tarball was extraced to the expected directory.
        if not os.path.exists(self.src_dir):
            print ('Tarball "' + self.tarball + '" did not unpack to the expected directory "' + self.src_dir + '"')
            raise FileNotFoundError

        pathlib.Path(self.unpack_stamp_file).touch(exist_ok = True)

        if self.patches is not None:
            push_all(self.toolchain,self.src_dir, self.patches)
        self.is_unpacked = True

    def make_clean_build_path(self):
        try:
            shutil.rmtree(self.build_dir)
        except FileNotFoundError:
            pass
        # If the build is done in-tree the wipe-out deleted the sources too.
        if not self.out_of_tree:
            self.is_unpacked = False
        os.makedirs(self.build_dir, exist_ok=True)
