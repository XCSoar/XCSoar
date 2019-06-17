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
        os.symlink(source, os.path.join(self.toolchain.install_prefix, self.installed), True)
        # Touch the szmbolic link to make it younger than the unpack stamp.
        pathlib.Path(target).touch
