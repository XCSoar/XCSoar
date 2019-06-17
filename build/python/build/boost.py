import os
import pathlib

from build.makeproject import MakeProject

class BoostProject(MakeProject):
    def __init__(self, toolchain, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, toolchain, url, alternative_url, md5, installed, **kwargs)

    def build(self):
        # Create symbolic link of the boost includes to the installation directory
        source = os.path.join(self.src_dir,'boost') 
        target = os.path.join(self.toolchain.install_prefix, self.installed)
        try:
            os.unlink(target)
        except FileNotFoundError:
            pass
        pathlib.Path(os.path.join(self.toolchain.install_prefix,'include')).mkdir(parents=True, exist_ok=True)
        os.symlink(source, os.path.join(self.toolchain.install_prefix, self.installed), True)
        # Touch the symbolic link to make it younger than the unpack stamp.
        pathlib.Path(target).touch
