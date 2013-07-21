TARGETS = PC WIN64 \
	PPC2000 PPC2003 PPC2003X WM5 WM5X \
	ALTAIR \
	UNIX UNIX32 UNIX64 OPT \
	PI KOBO NEON \
	ANDROID ANDROID7 ANDROID7NEON ANDROID86 ANDROIDMIPS \
	ANDROIDFAT \
	WINE CYGWIN

ifeq ($(TARGET),)
  ifeq ($(HOST_IS_UNIX),y)
    TARGET = UNIX
  else
    TARGET = PC
  endif
else
  ifeq ($(filter $(TARGET),$(TARGETS)),)
    $(error Unknown target: $(TARGET))
  endif
endif

TARGET_FLAVOR := $(TARGET)

HAVE_CE := n
HAVE_FPU := y
X64 := n
TARGET_IS_ARM = n
XSCALE := n
ARMV5 = n
ARMV6 = n
ARMV7 := n
NEON := n
X86 := n
MIPS := n
FAT_BINARY := n

TARGET_IS_DARWIN := n
TARGET_IS_LINUX := n
TARGET_IS_PI := n
TARGET_IS_KOBO := n
HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

TARGET_ARCH :=

# virtual targets ("flavors")

ifeq ($(TARGET),WIN64)
  X64 := y
  override TARGET = PC
endif

ifeq ($(TARGET),PPC2003X)
  TARGET_IS_ARM = y
  XSCALE := y
  override TARGET = PPC2003
endif

ifeq ($(TARGET),WM5X)
  TARGET_IS_ARM = y
  XSCALE := y
  override TARGET = WM5
endif

ifeq ($(TARGET),ANDROID)
  TARGET_IS_ARM = y
  ifeq ($(DEBUG),n)
    ARMV6 = y
  else
    # ARMv5 in the debug build, to allow installation on the emulator
    ARMV5 = y
  endif
endif

ifeq ($(TARGET),ANDROID7NEON)
  NEON := y
  override TARGET = ANDROID7
endif

ifeq ($(TARGET),ANDROID7)
  TARGET_IS_ARM = y
  ARMV7 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROID86)
  X86 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDMIPS)
  MIPS := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDFAT)
  FAT_BINARY := y
  override TARGET = ANDROID
endif

# real targets

ifeq ($(TARGET),PPC2000)
  TARGET_IS_ARM = y
  CE_MAJOR := 3
  CE_MINOR := 00
  PCPU := ARM

  HAVE_CE := y
endif

ifeq ($(TARGET),PPC2003)
  TARGET_IS_ARM = y
  CE_MAJOR := 4
  CE_MINOR := 00
  PCPU := ARMV4

  HAVE_CE := y
endif

ifeq ($(TARGET),PC)
  ifeq ($(X64),y)
    TCPREFIX := x86_64-w64-mingw32-
    TARGET_ARCH += -m64
  else
    TCPREFIX := i686-w64-mingw32-
    TARGET_ARCH += -march=i586
  endif

  ifneq ($(MINGWPATH),)
    TCPREFIX := $(MINGWPATH)
  endif

  ifeq ($(HOST_IS_WIN32),y)
    TCPREFIX :=
  endif

  WINVER = 0x0500
endif

ifeq ($(TARGET),CYGWIN)
  TCPREFIX :=

  TARGET_ARCH += -march=i586

  WINVER = 0x0500

  HAVE_POSIX := y
  HAVE_WIN32 := y
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(TARGET),ALTAIR)
  CE_MAJOR := 5
  CE_MINOR := 00

  TARGET_IS_ARM = y
  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),WM5)
  TARGET_IS_ARM = y
  PCPU := ARMV4
  CE_MAJOR := 5
  CE_MINOR := 00

  HAVE_CE := y
endif

ifeq ($(TARGET),WINE)
  TCPREFIX := wine
  TARGET_ARCH += -march=i586
  WINVER = 0x0500

  HAVE_POSIX := y
  HAVE_MSVCRT := n
endif

ifeq ($(TARGET),OPT)
  override TARGET = UNIX
  DEBUG = n
endif

ifeq ($(TARGET),UNIX)
  # LOCAL_TCPREFIX is set in local-config.mk if configure was run.
  TCPREFIX := $(LOCAL_TCPREFIX)
  TCSUFFIX := $(LOCAL_TCSUFFIX)
  TARGET_IS_ARM = $(HOST_IS_ARM)
  TARGET_IS_PI = $(HOST_IS_PI)
  ARMV6 = $(HOST_IS_ARMV6)
  ARMV7 = $(HOST_IS_ARMV7)
  NEON = $(HOST_HAS_NEON)
  TARGET_HAS_MALI = $(HOST_HAS_MALI)
endif

ifeq ($(TARGET),UNIX32)
  override TARGET = UNIX
  TARGET_ARCH += -m32
endif

ifeq ($(TARGET),UNIX64)
  override TARGET = UNIX
  TARGET_ARCH += -m64
endif

ifeq ($(TARGET),PI)
  override TARGET = UNIX
  TCPREFIX := arm-unknown-linux-gnueabi-
  PI ?= /opt/pi/root
  TARGET_ARCH += -march=armv6j -mfpu=vfp -mfloat-abi=hard
  TARGET_IS_PI = y
  TARGET_IS_ARM = y
  ARMV6 = y
endif

ifeq ($(TARGET),KOBO)
  # Experimental target for Kobo Mini
  override TARGET = NEON
  KOBO ?= /opt/kobo/arm-unknown-linux-gnueabi
  TARGET_IS_KOBO = y
endif

ifeq ($(TARGET),NEON)
  # Experimental target for generic ARMv7 with NEON
  override TARGET = UNIX
  TCPREFIX = arm-unknown-linux-gnueabi-
  ifeq ($(CLANG),n)
    TARGET_ARCH += -mcpu=cortex-a8 -mfloat-abi=hard
  endif
  TARGET_IS_ARM = y
  ARMV7 := y
  NEON := y
endif

ifeq ($(TARGET),UNIX)
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y

  ifeq ($(ARMV6),y)
    TARGET_ARCH += -march=armv6
  endif

  ifeq ($(ARMV7),y)
    ifeq ($(CLANG),y)
      TARGET_ARCH += -target armv7-none-linux-gnueabihf -integrated-as
    else
      TARGET_ARCH += -march=armv7-a
    endif
  endif

  ifeq ($(NEON),y)
    TARGET_ARCH += -mfpu=neon
  endif
endif

ifeq ($(filter $(TARGET),UNIX WINE),$(TARGET))
  ifeq ($(HOST_IS_LINUX),y)
    TARGET_IS_LINUX := y
  endif
  ifeq ($(HOST_IS_DARWIN),y)
    TARGET_IS_DARWIN := y
  endif
endif

ifeq ($(TARGET),ANDROID)
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r8e

  ANDROID_PLATFORM = android-17
  ANDROID_SDK_PLATFORM = $(ANDROID_PLATFORM)

  # NDK r8b has only android-14
  ANDROID_NDK_PLATFORM = android-14

  ANDROID_ARCH = arm
  ANDROID_ABI2 = arm-linux-androideabi
  ANDROID_ABI3 = armeabi
  ANDROID_ABI4 = $(ANDROID_ABI2)
  ANDROID_ABI_SUBDIR = .
  ANDROID_GCC_VERSION = 4.7
  ANDROID_GCC_VERSION2 = $(ANDROID_GCC_VERSION)

  ifeq ($(ARMV7),y)
    ANDROID_ABI3 = armeabi-v7a
    ANDROID_ABI_SUBDIR = armv7-a
  endif

  ifeq ($(X86),y)
    ANDROID_ARCH = x86
    ANDROID_ABI2 = x86
    ANDROID_ABI3 = x86
    ANDROID_ABI4 = i686-linux-android
  endif

  ifeq ($(MIPS),y)
    ANDROID_ARCH = mips
    ANDROID_ABI2 = mipsel-linux-android
    ANDROID_ABI3 = mips
  endif

  ANDROID_NDK_PLATFORM_DIR = $(ANDROID_NDK)/platforms/$(ANDROID_NDK_PLATFORM)
  ANDROID_TARGET_ROOT = $(ANDROID_NDK_PLATFORM_DIR)/arch-$(ANDROID_ARCH)

  ANDROID_GCC_TOOLCHAIN_NAME = $(ANDROID_ABI2)-$(ANDROID_GCC_VERSION)

  ifeq ($(CLANG),y)
    ANDROID_TOOLCHAIN_NAME = llvm-3.2

    # workaround: use libstdc++ 4.6, because 4.7 fails to link with
    # clang due to missing __atomic_* symbols
    ANDROID_GCC_VERSION = 4.6
  else
    ANDROID_TOOLCHAIN_NAME = $(ANDROID_GCC_TOOLCHAIN_NAME)
  endif

  ifeq ($(HOST_IS_DARWIN),y)
    ifeq ($(UNAME_M),x86_64)
      ANDROID_HOST_TAG = darwin-x86_64
    else
      ANDROID_HOST_TAG = darwin-x86
    endif
  else ifeq ($(HOST_IS_WIN32),y)
    ANDROID_HOST_TAG = windows
  else ifeq ($(UNAME_M),x86_64)
    ANDROID_HOST_TAG = linux-x86_64
  else
    ANDROID_HOST_TAG = linux-x86
  endif

  ANDROID_GCC_TOOLCHAIN = $(ANDROID_NDK)/toolchains/$(ANDROID_GCC_TOOLCHAIN_NAME)/prebuilt/$(ANDROID_HOST_TAG)
  ANDROID_TOOLCHAIN = $(ANDROID_NDK)/toolchains/$(ANDROID_TOOLCHAIN_NAME)/prebuilt/$(ANDROID_HOST_TAG)

  ifeq ($(CLANG),y)
    TCPREFIX = $(ANDROID_TOOLCHAIN)/bin/
  else
    TCPREFIX = $(ANDROID_TOOLCHAIN)/bin/$(ANDROID_ABI4)-
  endif

  ifeq ($(X86),y)
    HAVE_FPU := y
  endif

  ifeq ($(MIPS),y)
    HAVE_FPU := y
  endif

  ifeq ($(ARMV5),y)
    LLVM_TRIPLE = armv5te-none-linux-androideabi
    TARGET_ARCH += -march=armv5te -mtune=xscale -msoft-float -mthumb-interwork
    HAVE_FPU := n
  endif

  ifeq ($(ARMV6),y)
    LLVM_TRIPLE = armv6-none-linux-androideabi
    TARGET_ARCH += -march=armv6 -mtune=xscale -msoft-float -mthumb-interwork
    HAVE_FPU := n
  endif

  ifeq ($(ARMV7),y)
    LLVM_TRIPLE = armv7-none-linux-androideabi
    TARGET_ARCH += -march=armv7-a -mfloat-abi=softfp -mthumb-interwork
    HAVE_FPU := y
  endif

  ifeq ($(ARMV7)$(NEON),yy)
    TARGET_ARCH += -mfpu=neon
  endif

  ifeq ($(ARMV7)$(NEON),yn)
    TARGET_ARCH += -mfpu=vfpv3-d16
  endif

  TARGET_ARCH += -fpic -funwind-tables

  TARGET_IS_LINUX := y
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(HAVE_CE),y)
  TCPREFIX := arm-mingw32ce-
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

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  TARGET_CPPFLAGS += --sysroot=$(PI) -isystem $(PI)/usr/include/arm-linux-gnueabihf
endif

ifeq ($(TARGET_IS_KOBO),y)
  TARGET_CPPFLAGS += -DKOBO
  TARGET_CPPFLAGS += -isystem $(KOBO)/include
endif

ifeq ($(TARGET),ANDROID)
  TARGET_CPPFLAGS += --sysroot=$(ANDROID_TARGET_ROOT)
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
endif

ifeq ($(TARGET),PC)
  TARGET_ARCH += -mwindows -mms-bitfields
endif

ifeq ($(TARGET),CYGWIN)
  WINDRESFLAGS += -I./Data
endif

####### linker configuration

TARGET_LDFLAGS =
TARGET_LDLIBS =
TARGET_LDADD =
TARGET_STATIC ?= n

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
    TARGET_STATIC = y
  endif
  endif
endif

ifeq ($(HAVE_POSIX),y)
ifneq ($(TARGET),ANDROID)
  TARGET_LDLIBS += -lpthread
  ifeq ($(TARGET_IS_LINUX),y)
  TARGET_LDLIBS += -lrt # for clock_gettime()
  endif
endif
endif

ifeq ($(HOST_IS_PI)$(TARGET_IS_PI),ny)
  TARGET_LDFLAGS += --sysroot=$(PI) -L$(PI)/usr/lib/arm-linux-gnueabihf
endif

ifeq ($(TARGET_IS_KOBO),y)
  TARGET_LDFLAGS += -L$(KOBO)/lib
  TARGET_STATIC = y
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -Wl,--no-undefined
  TARGET_LDFLAGS += --sysroot=$(ANDROID_TARGET_ROOT)
  TARGET_LDFLAGS += -L$(ANDROID_TARGET_ROOT)/usr/lib

  ifeq ($(ARMV7),y)
    TARGET_LDFLAGS += -Wl,--fix-cortex-a8
  endif
endif

ifneq ($(filter PC WINE CYGWIN,$(TARGET)),)
  TARGET_LDLIBS += -lwinmm
endif

ifeq ($(TARGET),CYGWIN)
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
  ifeq ($(TARGET_IS_DARWIN),n)
  TARGET_LDLIBS += -lm
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDLIBS += -lc -lm
  TARGET_LDLIBS += -llog
  TARGET_LDADD += $(ANDROID_GCC_TOOLCHAIN)/lib/gcc/$(ANDROID_ABI4)/$(ANDROID_GCC_VERSION2)/$(ANDROID_ABI_SUBDIR)/libgcc.a
endif

ifeq ($(TARGET_STATIC),y)
  TARGET_LDFLAGS += -static
endif

######## output files

TARGET_EXEEXT = .exe

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
endif

ifeq ($(TARGET),ANDROID)
  TARGET_EXEEXT :=
endif
