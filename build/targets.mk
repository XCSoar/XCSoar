TARGETS = PC PPC2000 PPC2003 PPC2003X WM5 WM5X ALTAIR WINE UNIX ANDROID CYGWIN

# These targets are built when you don't specify the TARGET variable.
DEFAULT_TARGETS = PC PPC2000 PPC2003 WM5 ALTAIR WINE

HAVE_CE := n
HAVE_FPU := y
XSCALE := n

HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

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

ifeq ($(TARGET),PPC2003X)
  CE_MAJOR := 4
  CE_MINOR := 00
  PCPU := ARMV4

  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),PC)
  TCPATH := i586-mingw32msvc-
  ifeq ($(WINHOST),y)
    TCPATH :=
  endif

  CPU := i586
  MCPU := -march=$(CPU)

  WINVER = 0x0500
endif

ifeq ($(TARGET),CYGWIN)
  TCPATH :=

  CPU := i586
  MCPU := -march=$(CPU)

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

ifeq ($(TARGET),WM5X)
  PCPU := ARMV4
  CE_MAJOR := 5
  CE_MINOR := 00

  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),WINE)
  TCPATH := wine
  CPU := i586
  MCPU := -march=$(CPU)
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
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r5b

  ANDROID_PLATFORM = android-5
  ANDROID_ARCH = arm
  ANDROID_ABI2 = arm-linux-androideabi
  ANDROID_ABI3 = armeabi
  ANDROID_GCC_VERSION = 4.4.3

  ANDROID_NDK_PLATFORM = $(ANDROID_NDK)/platforms/$(ANDROID_PLATFORM)
  ANDROID_TARGET_ROOT = $(ANDROID_NDK_PLATFORM)/arch-$(ANDROID_ARCH)
  ANDROID_TOOLCHAIN = $(ANDROID_NDK)/toolchains/$(ANDROID_ABI2)-$(ANDROID_GCC_VERSION)/prebuilt/linux-x86
  TCPATH = $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_ABI2)-

  MCPU := -march=armv5te -mtune=xscale -msoft-float -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums
  HAVE_FPU := n
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(HAVE_CE),y)
  TCPATH := arm-mingw32ce-
  CPU := strongarm1110
  HAVE_FPU := n

  ifeq ($(XSCALE),y)
    CPU := xscale
  endif

  MCPU := -mcpu=$(CPU)
endif

######## target definitions

TARGET_CPPFLAGS =
TARGET_INCLUDES =

TARGET_CPPFLAGS = -I$(TARGET_OUTPUT_DIR)/include

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
  # -mno-cygwin
endif

ifeq ($(TARGET),ALTAIR)
  TARGET_CPPFLAGS += -DGNAV
  TARGET_CPPFLAGS += -DDISABLEAUDIO
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

ifeq ($(HAVE_WIN32),n)
  TARGET_CPPFLAGS += -DDISABLEAUDIO
endif

ifeq ($(TARGET),ANDROID)
  TARGET_CPPFLAGS += -I$(ANDROID_TARGET_ROOT)/usr/include
  TARGET_CPPFLAGS += -isystem $(ANDROID_NDK)/sources/cxx-stl/stlport/stlport
  TARGET_CPPFLAGS += -DANDROID
  CXXFLAGS += -D__STDC_VERSION__=199901L
endif

####### compiler target

TARGET_ARCH := $(MCPU)

ifeq ($(HAVE_WIN32),y)
  TARGET_ARCH += -mwin32

  WINDRESFLAGS := -I$(SRC) $(TARGET_CPPFLAGS)
endif # UNIX

ifeq ($(TARGET),PC)
  TARGET_ARCH += -mwindows -mms-bitfields
endif

ifeq ($(TARGET),WINE)
  TARGET_ARCH := $(filter-out -mwin32,$(TARGET_ARCH))
  TARGET_ARCH += -m32
endif

ifeq ($(TARGET),CYGWIN)
  WINDRESFLAGS += -I./Data
  TARGET_ARCH :=
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
    TARGET_LDFLAGS := -Wl,--major-subsystem-version=$(CE_MAJOR)
    TARGET_LDFLAGS += -Wl,--minor-subsystem-version=$(CE_MINOR)
  endif

  ifneq ($(TARGET),WINE)
  ifneq ($(TARGET),CYGWIN)
    # link libstdc++-6.dll statically, so we don't have to distribute it
    TARGET_LDFLAGS += -static
  endif
  endif
endif

ifeq ($(HAVE_POSIX),y)
ifneq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -lpthread
  ifneq ($(TARGET),CYGWIN)
  TARGET_LDFLAGS += -lrt # for clock_gettime()
  endif
endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -nostdlib -Wl,-shared,-Bsymbolic -Wl,--no-undefined
endif

ifneq ($(filter PC WINE,$(TARGET)),)
  TARGET_LDLIBS := -lwinmm -lstdc++
endif

ifeq ($(TARGET),CYGWIN)
  TARGET_LDLIBS := -lwinmm -lstdc++
  TARGET_LDLIBS += -lintl
endif

ifeq ($(HAVE_CE),y)
  TARGET_LDLIBS := -lstdc++

  ifneq ($(TARGET),ALTAIR)
    TARGET_CPPFLAGS += -DHAVE_NOTE_PRJ_DLL
    TARGET_CPPFLAGS += -DHAVE_AYGSHELL_DLL
    TARGET_CPPFLAGS += -DHAVE_IMGDECMP_DLL
  endif
endif

ifeq ($(TARGET),UNIX)
  TARGET_LDLIBS := -lstdc++ -lm
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDLIBS := $(ANDROID_TARGET_ROOT)/usr/lib/libstdc++.so
  TARGET_LDLIBS += $(ANDROID_NDK)/sources/cxx-stl/stlport/libs/$(ANDROID_ABI3)/libstlport_static.a
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/libGLESv1_CM.so
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/libc.so $(ANDROID_TARGET_ROOT)/usr/lib/libm.so
  TARGET_LDLIBS += $(ANDROID_TARGET_ROOT)/usr/lib/liblog.so
  TARGET_LDLIBS += $(ANDROID_TOOLCHAIN)/lib/gcc/$(ANDROID_ABI2)/$(ANDROID_GCC_VERSION)/libgcc.a
endif

######## output files

TARGET_EXEEXT = .exe
NOSTRIP_SUFFIX = -ns

ifeq ($(TARGET),WINE)
  TARGET_EXEEXT :=
  NOSTRIP_SUFFIX :=
endif

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
  NOSTRIP_SUFFIX :=
endif

ifeq ($(TARGET),ANDROID)
  TARGET_EXEEXT := .so
endif
