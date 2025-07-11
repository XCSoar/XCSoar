import os.path, subprocess, shutil
from typing import Optional, Sequence, Union

from build.makeproject import MakeProject
from build.quilt import push_all
from .toolchain import AnyToolchain

class LuaProject(MakeProject):
    def __init__(self, url: Union[str, Sequence[str]], md5: str, installed: str,
                 **kwargs):
        MakeProject.__init__(self, url, md5, installed, **kwargs)

    def get_make_args(self, toolchain: AnyToolchain) -> list[str]:
        cflags = toolchain.cflags + ' ' + toolchain.cppflags

        # hard-code lua_getlocaledecpoint() because
        # localeconv()->decimal_point is not available on the Bionic
        # version we're depnding on
        cflags += " \"-Dlua_getlocaledecpoint()='.'\""

        if toolchain.is_target_ios:
            cflags += ' -DLUA_USE_IOS'
            
        return MakeProject.get_make_args(self, toolchain) + [
            'CC=' + toolchain.cc,
            'AR=' + toolchain.ar + ' rcu',
            'RANLIB=true',
            'MYCFLAGS=' + cflags,
            'MYLDFLAGS=' + toolchain.ldflags,
            'liblua.a'
        ]

    def _build(self, toolchain: AnyToolchain, target_toolchain: Optional[AnyToolchain]=None) -> None:
        src = self.unpack(toolchain, out_of_tree=False)

        wd = os.path.join(src, 'src')

        self.build_make(toolchain, wd, False)

        includedir = os.path.join(toolchain.install_prefix, 'include')
        libdir = os.path.join(toolchain.install_prefix, 'lib')

        os.makedirs(includedir, exist_ok=True)
        for i in ('lauxlib.h', 'luaconf.h', 'lua.h', 'lualib.h'):
            shutil.copyfile(os.path.join(src, 'src', i),
                            os.path.join(includedir, i))

        os.makedirs(libdir, exist_ok=True)
        shutil.copyfile(os.path.join(src, 'src', 'liblua.a'),
                        os.path.join(libdir, 'liblua.a'))
