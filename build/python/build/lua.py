import os.path, subprocess, shutil

from build.makeproject import MakeProject
from build.quilt import push_all

class LuaProject(MakeProject):
    def __init__(self, toolchain, url, alternative_url, md5, installed,
                 **kwargs):
        MakeProject.__init__(self, toolchain, url, alternative_url, md5, installed, **kwargs)

    def get_make_args(self):
        cflags = self.toolchain.cflags + ' ' + self.toolchain.cppflags

        # hard-code lua_getlocaledecpoint() because
        # localeconv()->decimal_point is not available on the Bionic
        # version we're depnding on
        cflags += " \"-Dlua_getlocaledecpoint()='.'\""

        return MakeProject.get_make_args(self) + [
            'CC=' + self.toolchain.cc,
            'AR=' + self.toolchain.ar + ' rcu',
            'RANLIB=true',
            'MYCFLAGS=' + cflags,
            'MYLDFLAGS=' + self.toolchain.ldflags,
            'liblua.a'
        ]

    def build(self):
        wd = os.path.join(self.build_dir, 'src')

        MakeProject.build(self, wd, False)

        includedir = os.path.join(self.toolchain.install_prefix, 'include')
        libdir = os.path.join(self.toolchain.install_prefix, 'lib')

        os.makedirs(includedir, exist_ok=True)
        for i in ('lauxlib.h', 'luaconf.h', 'lua.h', 'lualib.h'):
            shutil.copyfile(os.path.join(self.build_dir, 'src', i),
                            os.path.join(includedir, i))

        os.makedirs(libdir, exist_ok=True)
        shutil.copyfile(os.path.join(self.build_dir, 'src', 'liblua.a'),
                        os.path.join(libdir, 'liblua.a'))
