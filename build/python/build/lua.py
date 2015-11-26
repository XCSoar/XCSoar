import os.path, subprocess, shutil

from build.project import Project
from build.quilt import push_all

class LuaProject(Project):
    def __init__(self, url, md5, installed,
                 **kwargs):
        Project.__init__(self, url, md5, installed, **kwargs)

    def build(self, toolchain):
        src = self.unpack(toolchain, out_of_tree=False)

        cflags = toolchain.cflags + ' ' + toolchain.cppflags

        # hard-code lua_getlocaledecpoint() because
        # localeconv()->decimal_point is not available on the Bionic
        # version we're depnding on
        cflags += " \"-Dlua_getlocaledecpoint()='.'\""

        subprocess.check_call(['/usr/bin/make', '-j12',
                               'CC=' + toolchain.cc,
                               'AR=' + toolchain.ar + ' rcu',
                               'RANLIB=true',
                               'MYCFLAGS=' + cflags,
                               'MYLDFLAGS=' + toolchain.ldflags,
                               'liblua.a'],
                              cwd=os.path.join(src, 'src'), env=toolchain.env)

        includedir = os.path.join(toolchain.install_prefix, 'include')
        libdir = os.path.join(toolchain.install_prefix, 'lib')

        os.makedirs(includedir, exist_ok=True)
        for i in ('lauxlib.h', 'luaconf.h', 'lua.h', 'lualib.h'):
            shutil.copyfile(os.path.join(src, 'src', i),
                            os.path.join(includedir, i))

        os.makedirs(libdir, exist_ok=True)
        shutil.copyfile(os.path.join(src, 'src', 'liblua.a'),
                        os.path.join(libdir, 'liblua.a'))
