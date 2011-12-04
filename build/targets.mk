TARGETS = PC WIN64 PPC2000 PPC2003 PPC2003X WM5 WM5X ALTAIR WINE UNIX ANDROID ANDROID7 ANDROIDFAT CYGWIN

# These targets are built when you don't specify the TARGET variable.
DEFAULT_TARGETS = PC PPC2000 PPC2003 WM5 ALTAIR WINE

TARGET_FLAVOR := $(TARGET)

HAVE_CE := n
HAVE_FPU := y
X64 := n
XSCALE := n
ARMV7 := n
X86 := n
FAT_BINARY := n

HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

TARGET_ARCH :=

# virtual targets ("flavors")

ifeq ($(TARGET),WIN64)
  X64 := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = PC
endif

ifeq ($(TARGET),PPC2003X)
  XSCALE := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = PPC2003
endif

ifeq ($(TARGET),WM5X)
  XSCALE := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = WM5
endif

ifeq ($(TARGET),ANDROID7)
  ARMV7 := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROID86)
  X86 := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDFAT)
  FAT_BINARY := y
  TARGET_FLAVOR := $(TARGET)
  override TARGET = ANDROID
endif

# real targets

ifeq ($(TARGET),PPC2000)
  CE_MAJOR := 3
  CE_MINOR := 00
  PCPU := ARM

  HAVE_CE := y
endif

ifeq ($(TARGET),PPC2003)
  CE_MAJOR := 4
  CE_MINOR := 00
  PCPU := ARMV4

  HAVE_CE := y
endif

ifeq ($(TARGET),PC)
  ifeq ($(X64),y)
    TCPATH := x86_64-w64-mingw32-
    TARGET_ARCH += -m64
  else
    TCPATH := i686-w64-mingw32-
    TARGET_ARCH += -march=i586
  endif
  
  ifneq ($(MINGWPATH),"")
    TCPATH := $(MINGWPATH)
  endif

  ifeq ($(WINHOST),y)
    TCPATH :=
  endif

  WINVER = 0x0500
endif

ifeq ($(TARGET),CYGWIN)
  TCPATH :=

  TARGET_ARCH += -march=i586

  WINVER = 0x0500
  WINHOST := y

  HAVE_POSIX := y
  HAVE_WIN32 := y
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(TARGET),ALTAIR)
  CE_MAJOR := 5
  CE_MINOR := 00

  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),WM5)
  PCPU := ARMV4
  CE_MAJOR := 5
  CE_MINOR := 00

  HAVE_CE := y
endif

ifeq ($(TARGET),WINE)
  TCPATH := wine
  TARGET_ARCH += -march=i586
  WINVER = 0x0500

  HAVE_POSIX := y
  HAVE_MSVCRT := n
endif

ifeq ($(TARGET),UNIX)
  TCPATH :=

  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(TARGET),ANDROID)
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r7

  ANDROID_PLATFORM = android-8
  ANDROID_ARCH = arm
  ANDROID_ABI2 = arm-linux-androideabi
  ANDROID_GCC_VERSION = 4.4.3

  ifeq ($(ARMV7),y)
    ANDROID_ABI3 = armeabi-v7a
  else
    ANDROID_ABI3 = armeabi
  endif

  ANDROID_ABI4 = $(ANDROID_ABI2)

  ANDROID_NDK_PLATFORM = $(ANDROID_NDK)/platforms/$(ANDROID_PLATFORM)
  ANDROID_TARGET_ROOT = $(ANDROID_NDK_PLATFORM)/arch-$(ANDROID_ARCH)
  ANDROID_TOOLCHAIN = $(ANDROID_NDK)/toolchains/$(ANDROID_ABI2)-$(ANDROID_GCC_VERSION)/prebuilt/linux-x86
  TCPATH = $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_ABI4)-

  ifeq ($(X86),y)
    ANDROID_PLATFORM := android-9
    ANDROID_ARCH := x86
    ANDROID_ABI2 := x86
    ANDROID_ABI4 := i686-android-linux
    HAVE_FPU := y
  else
  ifeq ($(ARMV7),y)
    TARGET_ARCH += -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -mthumb-interwork
    HAVE_FPU := y
  else
    TARGET_ARCH += -march=armv5te -mtune=xscale -msoft-float -mthumb-interwork
    HAVE_FPU := n
  endif
  endif

  TARGET_ARCH += -fpic -funwind-tables

  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(HAVE_CE),y)
  TCPATH := arm-mingw32ce-
  HAVE_FPU := n

  ifeq ($(XSCALE),y)
    TARGET_ARCH += -mcpu=xscale
  else
    TARGET_ARCH += -mcpu=strongarm1110
  endif
endif

######## target definitions

TARGET_INCLUDES =
TARGET_CPPFLAGS = -I$(TARGET_OUTPUT_DIR)/include

ifeq ($(TESTING),y)
  TARGET_CPPFLAGS += -DTESTING=y
endif

ifneq ($(WINVER),)
  TARGET_CPPFLAGS += -DWINVER=$(WINVER) -D_WIN32_WINDOWS=$(WINVER)
  TARGET_CPPFLAGS += -D_WIN32_WINNT=$(WINVER) -D_WIN32_IE=$(WINVER)
endif

ifeq ($(HAVE_CE),y)
  TARGET_CPPFLAGS += -D_WIN32_WCE=0x0$(CE_MAJOR)$(CE_MINOR)
  TARGET_CPPFLAGS += -DWIN32_PLATFORM_PSPC=$(CE_MAJOR)$(CE_MINOR)
endif

ifeq ($(HAVE_WIN32),y)
  TARGET_CPPFLAGS += -DWIN32_LEAN_AND_MEAN
  TARGET_CPPFLAGS += -DNOMINMAX
  ifeq ($(TARGET),CYGWIN)
  TARGET_CPPFLAGS += -DWIN32
  endif
endif

ifeq ($(TARGET),PPC2000)
  TARGET_CPPFLAGS += -DNOLINETO -DNOCLEARTYPE
endif

ifeq ($(TARGET),WINE)
  TARGET_CPPFLAGS += -D__WINE__
  TARGET_CPPFLAGS += -DWINE_STRICT_PROTOTYPES
  # -mno-cygwin
endif

ifeq ($(TARGET),ALTAIR)
  TARGET_CPPFLAGS += -DGNAV
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_CPPFLAGS += -DHAVE_POSIX
  TARGET_CPPFLAGS += -DHAVE_STDINT_H
  TARGET_CPPFLAGS += -DHAVE_UNISTD_H
  TARGET_CPPFLAGS += -DHAVE_VASPRINTF
endif

ifeq ($(HAVE_MSVCRT),y)
  TARGET_CPPFLAGS += -DHAVE_MSVCRT
  TARGET_CPPFLAGS += -DUNICODE -D_UNICODE
endif

ifeq ($(HAVE_WIN32),n)
  TARGET_INCLUDES += -I$(SRC)/unix
endif

ifeq ($(TARGET),WINE)
  TARGET_INCLUDES += -I$(SRC)/wine
endif

ifeq ($(TARGET),ANDROID)
  TARGET_CPPFLAGS += -I$(ANDROID_TARGET_ROOT)/usr/include
  TARGET_CPPFLAGS += -isystem $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include
  TARGET_CPPFLAGS += -isystem $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(ANDROID_ABI3)/include
  TARGET_CPPFLAGS += -DANDROID
  CXXFLAGS += -D__STDC_VERSION__=199901L

  ifeq ($(X86),y)
    # On NDK r6, the macro _BYTE_ORDER never gets defined - workaround:
    TARGET_CPPFLAGS += -D_BYTE_ORDER=_LITTLE_ENDIAN
  endif
endif

####### compiler target

ifeq ($(HAVE_WIN32),y)
  ifeq ($(TARGET),WINE)
    TARGET_ARCH += -m32
  else
    TARGET_ARCH += -mwin32
  endif

  WINDRESFLAGS := -I$(SRC) $(TARGET_CPPFLAGS)
endif # HAVE_WIN32

ifeq ($(TARGET),PC)
  TARGET_ARCH += -mwindows -mms-bitfields
endif

ifeq ($(TARGET),CYGWIN)
  WINDRESFLAGS += -I./Data
endif

####### linker configuration

TARGET_LDFLAGS =
TARGET_LDLIBS =

ifeq ($(TARGET),PC)
  TARGET_LDFLAGS += -Wl,--major-subsystem-version=5
  TARGET_LDFLAGS += -Wl,--minor-subsystem-version=00

  # default to "console"; see SCREEN_LDLIBS
  TARGET_LDFLAGS += -Wl,-subsystem,console
endif

ifeq ($(HAVE_WIN32),y)
  ifeq ($(HAVE_CE),y)
    TARGET_LDFLAGS += -Wl,--major-subsystem-version=$(CE_MAJOR)
    TARGET_LDFLAGS += -Wl,--minor-subsystem-version=$(CE_MINOR)
  endif

  ifeq ($(TARGET),WINE)
    TARGET_LDLIBS += -lpthread
  else
  ifneq ($(TARGET),CYGWIN)
    # link libstdc++-6.dll statically, so we don't have to distribute it
    TARGET_LDFLAGS += -static
  endif
  endif
endif

ifeq ($(HAVE_POSIX),y)
ifneq ($(TARGET),ANDROID)
  TARGET_LDLIBS += -lpthread
  ifeq ($(shell uname -s),Linux)
  TARGET_LDLIBS += -lrt # for clock_gettime()
  endif
endif
  ifeq ($(shell uname -s),Darwin)
    TARGET_LDFLAGS += -static-libgcc
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -nostdlib -Wl,-shared,-Bsymbolic -Wl,--no-undefined

  ifeq ($(ARMV7),y)
    TARGET_LDFLAGS += -Wl,--fix-cortex-a8
  endif
endif

ifneq ($(filter PC WINE,$(TARGET)),)
  TARGET_LDLIBS += -lwinmm
endif

ifeq ($(TARGET),CYGWIN)
  TARGET_LDLIBS += -lwinmm
  TARGET_LDLIBS += -lintl
endif

ifeq ($(HAVE_CE),y)
  ifneq ($(TARGET),ALTAIR)
    TARGET_CPPFLAGS += -DHAVE_NOTE_PRJ_DLL
    TARGET_CPPFLAGS += -DHAVE_AYGSHELL_DLL
    TARGET_CPPFLAGS += -DHAVE_IMGDECMP_DLL
  endif
endif

ifeq ($(TARGET),UNIX)
  ifeq ($(shell uname -s),Darwin)
  TARGET_LDLIBS += $(shell $(CXX) -print-file-name=libstdc++.a)
  TARGET_AR = "libtool"
  TARGET_ARFLAGS = -static -o
  else
  TARGET_LDLIBS += -lm
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDLIBS += $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(ANDROID_ABI3)/libgnustl_static.a
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/libGLESv1_CM.so
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/libc.so $(ANDROID_TARGET_ROOT)/usr/lib/libm.so
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/liblog.so
  TARGET_LDLIBS += $(ANDROID_TOOLCHAIN)/lib/gcc/$(ANDROID_ABI4)/$(ANDROID_GCC_VERSION)/libgcc.a
endif

######## output files

TARGET_EXEEXT = .exe
NOSTRIP_SUFFIX = -ns

ifeq ($(TARGET),WINE)
  NOSTRIP_SUFFIX :=
endif

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
  NOSTRIP_SUFFIX :=
endif

ifeq ($(TARGET),ANDROID)
  TARGET_EXEEXT := .so
endif
