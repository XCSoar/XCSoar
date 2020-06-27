TARGETS = PC WIN64 \
	PPC2000 PPC2003 PPC2003X WM5 WM5X \
	ALTAIR \
	UNIX UNIX32 UNIX64 OPT \
	WAYLAND \
	PI PI2 CUBIE KOBO NEON \
	ANDROID ANDROID7 ANDROID7NEON ANDROID86 ANDROIDMIPS \
	ANDROIDAARCH64 ANDROIDX64 ANDROIDMIPS64 \
	ANDROIDFAT \
	CYGWIN \
	OSX64 IOS32 IOS64

ifeq ($(TARGET),)
  ifeq ($(HOST_IS_UNIX),y)
    ifeq ($(HOST_IS_DARWIN),y)
      TARGET = OSX64
    else
      TARGET = UNIX
    endif
  else
    TARGET = PC
  endif
else
  ifeq ($(filter $(TARGET),$(TARGETS)),)
    $(error Unknown target: $(TARGET))
  endif
endif

ifeq ($(MAKECMDGOALS),python)
  TARGET_FLAVOR := $(TARGET)_PYTHON
else
  TARGET_FLAVOR := $(TARGET)
endif

HAVE_CE := n
HAVE_FPU := y
X64 := n
TARGET_IS_ARM = n
TARGET_IS_ARMHF = n
XSCALE := n
ARMV5 = n
ARMV6 = n
ARMV7 := n
NEON := n
AARCH64 := n
X86 := n
MIPS := n
MIPS64 := n
FAT_BINARY := n

TARGET_IS_DARWIN := n
TARGET_IS_LINUX := n
TARGET_IS_ANDROID := n
TARGET_IS_PI := n
TARGET_IS_PI4 := n
TARGET_IS_PI32 := n
TARGET_IS_PI64 := n
TARGET_IS_KOBO := n
HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

USE_CROSSTOOL_NG := n

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
  TARGET_IS_ARMHF = y
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

ifeq ($(TARGET),ANDROIDAARCH64)
  AARCH64 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDX64)
  X64 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDMIPS64)
  MIPS64 := y
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

ifeq ($(TARGET),OPT)
  override TARGET = UNIX
  DEBUG = n
endif

ifeq ($(TARGET),WAYLAND)
  # experimental target for the Wayland display server
  override TARGET = UNIX
  USE_WAYLAND = y
endif

ifeq ($(TARGET),UNIX)
  # LOCAL_TCPREFIX is set in local-config.mk if configure was run.
  TCPREFIX := $(LOCAL_TCPREFIX)
  TCSUFFIX := $(LOCAL_TCSUFFIX)
  TARGET_IS_ARM = $(HOST_IS_ARM)
  TARGET_IS_PI = $(HOST_IS_PI)
  TARGET_IS_PI4 = $(HOST_IS_PI4)
  TARGET_IS_PI32 = $(call bool_and,$(HOST_IS_PI),$(HOST_IS_ARM))
  TARGET_IS_PI64 = $(call bool_and,$(HOST_IS_PI),$(HOST_IS_AARCH64))
  ARMV6 = $(HOST_IS_ARMV6)
  ARMV7 = $(HOST_IS_ARMV7)
  NEON = $(HOST_HAS_NEON)
  TARGET_IS_ARMHF := $(call bool_or,$(ARMV7),$(TARGET_IS_PI32))
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
  TCPREFIX := arm-linux-gnueabihf-
  ifeq ($(HOST_IS_PI),n)
    PI ?= /opt/pi/root
  endif
  TARGET_IS_PI = y
  TARGET_IS_PI32 = y
  TARGET_IS_ARM = y
  TARGET_IS_ARMHF = y
  ARMV6 = y
endif

ifeq ($(TARGET),PI2)
  override TARGET = NEON
  ifeq ($(HOST_IS_PI),n)
    PI ?= /opt/pi/root
  endif
  TARGET_IS_PI = y
  TARGET_IS_PI32 = y
endif

ifeq ($(TARGET),CUBIE)
  # cross-crompiling for Cubieboard
  override TARGET = NEON
  CUBIE ?= /opt/cubie/root
  TARGET_HAS_MALI = y
endif

ifeq ($(TARGET),KOBO)
  # Experimental target for Kobo Mini
  override TARGET = NEON
  TARGET_IS_KOBO = y

  # We are using a GNU toolchain (triplet arm-linux-gnueabihf) by default, but
  # the actual host triplet is different.
  ACTUAL_HOST_TRIPLET = armv7a-a8neon-linux-musleabihf

  ifeq ($(USE_CROSSTOOL_NG),y)
    HOST_TRIPLET = $(ACTUAL_HOST_TRIPLET)
    LLVM_TARGET = $(ACTUAL_HOST_TRIPLET)
    KOBO_TOOLCHAIN = $(HOME)/x-tools/$(HOST_TRIPLET)
    KOBO_SYSROOT = $(KOBO_TOOLCHAIN)/$(HOST_TRIPLET)/sysroot
    TCPREFIX = $(KOBO_TOOLCHAIN)/bin/$(HOST_TRIPLET)-
  else
  endif
endif

ifeq ($(TARGET),NEON)
  # Experimental target for generic ARMv7 with NEON
  override TARGET = UNIX
  ifeq ($(USE_CROSSTOOL_NG),y)
    HOST_TRIPLET = arm-unknown-linux-gnueabihf
  else
    HOST_TRIPLET = arm-linux-gnueabihf
  endif
  TCPREFIX = $(HOST_TRIPLET)-
  ifeq ($(CLANG),n)
    TARGET_ARCH += -mcpu=cortex-a8
  endif
  TARGET_IS_ARM = y
  TARGET_IS_ARMHF = y
  ARMV7 := y
  NEON := y
endif

ifeq ($(TARGET),OSX64)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_OSX = y
  OSX_MIN_SUPPORTED_VERSION = 10.7
  HOST_TRIPLET = x86_64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  LIBCXX = y
  CLANG = y
  TARGET_ARCH += -mmacosx-version-min=$(OSX_MIN_SUPPORTED_VERSION)
endif

ifeq ($(TARGET),IOS32)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 5.1
  HOST_TRIPLET = armv7-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK_VERSION = 9.1
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${DARWIN_SDK_VERSION}.sdk
  endif
  LIBCXX = y
  CLANG = y
  TARGET_ARCH += -miphoneos-version-min=$(IOS_MIN_SUPPORTED_VERSION)
endif

ifeq ($(TARGET),IOS64)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 7.0
  HOST_TRIPLET = aarch64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK_VERSION = 9.1
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${DARWIN_SDK_VERSION}.sdk
  endif
  LIBCXX = y
  CLANG = y
  TARGET_ARCH += -miphoneos-version-min=$(IOS_MIN_SUPPORTED_VERSION) -arch arm64
  ASFLAGS += -arch arm64
endif

ifeq ($(TARGET),UNIX)
  ifeq ($(HOST_IS_LINUX)$(TARGET_IS_DARWIN),yn)
    TARGET_IS_LINUX := y
  endif
  ifeq ($(HOST_IS_DARWIN),y)
    TARGET_IS_DARWIN := y
  endif
endif

ifeq ($(TARGET),UNIX)
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y

  ifeq ($(ARMV6),y)
    TARGET_ARCH += -march=armv6

    ifneq ($(CLANG),y)
      # Force-disable thumb just in case the gcc binary was built with
      # thumb enabled by default.  This fixes the dreaded gcc error
      # "sorry, unimplemented: Thumb-1 hard-float VFP ABI".
      TARGET_ARCH += -marm
    endif
  endif

  ifeq ($(ARMV7),y)
    TARGET_ARCH += -march=armv7-a
  endif

  ifeq ($(TARGET_IS_ARMHF),y)
    ifeq ($(ARMV6),y)
      TARGET_ARCH += -mfpu=vfp
    endif

    ifeq ($(NEON),y)
      TARGET_ARCH += -mfpu=neon
    endif

    TARGET_ARCH += -mfloat-abi=hard
  endif

  ifeq ($(TARGET_IS_ARM)$(TARGET_IS_LINUX),yy)
    ifeq ($(TARGET_IS_ARMHF),y)
      LLVM_TARGET = arm-linux-gnueabihf
    else
      LLVM_TARGET = arm-linux-gnueabi
    endif
  endif
endif

ifeq ($(TARGET),ANDROID)
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r15c

  ANDROID_SDK_PLATFORM = android-26
  ANDROID_NDK_API = 13

  ANDROID_ARCH = arm
  ANDROID_ABI2 = arm-linux-androideabi
  ANDROID_ABI3 = armeabi
  ANDROID_ABI4 = $(ANDROID_ABI2)
  ANDROID_ABI5 = $(ANDROID_ABI3)
  ANDROID_GCC_VERSION = 4.9

  ifeq ($(ARMV7),y)
    ANDROID_ABI3 = armeabi-v7a
    ANDROID_ABI5 = armeabi-v7a
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

  ifeq ($(AARCH64),y)
    ANDROID_ARCH = arm64
    ANDROID_ABI2 = aarch64-linux-android
    ANDROID_ABI3 = arm64-v8a
    ANDROID_NDK_API = 21
  endif

  ifeq ($(X64),y)
    ANDROID_ARCH = x86_64
    ANDROID_ABI2 = x86_64
    ANDROID_ABI3 = x86_64
    ANDROID_ABI4 = x86_64-linux-android
    ANDROID_NDK_API = 21
  endif

  ifeq ($(MIPS64),y)
    ANDROID_ARCH = mips64
    ANDROID_ABI2 = mips64el-linux-android
    ANDROID_ABI3 = mips64
    ANDROID_NDK_API = 21
  endif

  ANDROID_NDK_PLATFORM = android-$(ANDROID_NDK_API)

  ANDROID_SYSROOT = $(ANDROID_NDK)/sysroot
  ANDROID_NDK_PLATFORM_DIR = $(ANDROID_NDK)/platforms/$(ANDROID_NDK_PLATFORM)
  ANDROID_TARGET_ROOT = $(ANDROID_NDK_PLATFORM_DIR)/arch-$(ANDROID_ARCH)

  ANDROID_GCC_TOOLCHAIN_NAME = $(ANDROID_ABI2)-$(ANDROID_GCC_VERSION)

  # clang is the default compiler on Android
  CLANG ?= y

  ifeq ($(CLANG),y)
    ANDROID_TOOLCHAIN_NAME = llvm
    LIBCXX = y
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

  TCPREFIX = $(ANDROID_GCC_TOOLCHAIN)/bin/$(ANDROID_ABI4)-
  LLVM_PREFIX = $(ANDROID_TOOLCHAIN)/bin/

  ifeq ($(X86),y)
    LLVM_TARGET = i686-none-linux-android
    HAVE_FPU := y
  endif

  ifeq ($(MIPS),y)
    LLVM_TARGET = mipsel-none-linux-android
    HAVE_FPU := y
  endif

  ifeq ($(ARMV5),y)
    LLVM_TARGET = armv5te-none-linux-androideabi
    TARGET_ARCH += -march=armv5te -mtune=xscale -msoft-float -mthumb-interwork
    HAVE_FPU := n
  endif

  ifeq ($(ARMV6),y)
    LLVM_TARGET = armv6-none-linux-androideabi
    TARGET_ARCH += -march=armv6 -mtune=xscale -msoft-float -mthumb-interwork
    HAVE_FPU := n
  endif

  ifeq ($(ARMV7),y)
    LLVM_TARGET = armv7a-none-linux-androideabi
    TARGET_ARCH += -march=armv7-a -mfloat-abi=softfp
    HAVE_FPU := y
  endif

  ifeq ($(ARMV7)$(NEON),yy)
    TARGET_ARCH += -mfpu=neon
  endif

  ifeq ($(ARMV7)$(NEON),yn)
    TARGET_ARCH += -mfpu=vfpv3-d16
  endif

  ifeq ($(AARCH64),y)
    LLVM_TARGET = aarch64-linux-android
    HAVE_FPU := y
  endif

  ifeq ($(X64),y)
    LLVM_TARGET = x86_64-linux-android
    HAVE_FPU := y
  endif

  ifeq ($(MIPS64),y)
    LLVM_TARGET = mips64el-linux-android
    HAVE_FPU := y
  endif

  TARGET_ARCH += -fpic -funwind-tables

  TARGET_IS_LINUX := y
  TARGET_IS_ANDROID := y
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
TARGET_CXXFLAGS =
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
  TARGET_CPPFLAGS += -DSTRICT
endif

ifeq ($(HAVE_WIN32),n)
  TARGET_INCLUDES += -I$(SRC)/unix
endif

ifeq ($(TARGET_IS_PI),y)
  TARGET_CPPFLAGS += -DRASPBERRY_PI

  ifneq ($(PI),)
    TARGET_CPPFLAGS += --sysroot=$(PI) -isystem $(PI)/usr/include/arm-linux-gnueabihf -isystem $(PI)/usr/include
  endif
endif

ifeq ($(HOST_IS_ARM)$(TARGET_HAS_MALI),ny)
  # cross-crompiling for Cubieboard
  TARGET_CPPFLAGS += --sysroot=$(CUBIE) -isystem $(CUBIE)/usr/include/arm-linux-gnueabihf
  TARGET_CPPFLAGS += -isystem $(CUBIE)/usr/local/stow/sunxi-mali/include
endif

ifeq ($(TARGET_IS_KOBO),y)
  TARGET_CPPFLAGS += -DKOBO

  # Use Thumb instructions (which is the default in Debian's arm-linux-gnueabihf
  # toolchain, but this might be different when using another toolchain, or
  # Clang instead of GCC).
  TARGET_ARCH += -mthumb

  # We are using a GNU toolchain (triplet arm-linux-gnueabihf) by default, but
  # the actual host triplet is different.
  ACTUAL_HOST_TRIPLET = armv7a-a8neon-linux-musleabihf

  LIBSTDCXX_HEADERS_DIR = $(abspath $(THIRDPARTY_LIBS_ROOT)/include/libstdc++)
  TARGET_CXXFLAGS += \
      -nostdinc++ \
      -isystem $(LIBSTDCXX_HEADERS_DIR) \
      -isystem $(LIBSTDCXX_HEADERS_DIR)/$(ACTUAL_HOST_TRIPLET)

  ifeq ($(USE_CROSSTOOL_NG),y)
    ifeq ($(CLANG),y)
      TARGET_CPPFLAGS += -B$(KOBO_TOOLCHAIN)
      TARGET_CPPFLAGS += --sysroot=$(KOBO_SYSROOT)
    endif
  else
    LIBSTDCXX_HEADERS_DIR = $(abspath $(THIRDPARTY_LIBS_ROOT)/include/libstdc++)
    TARGET_CXXFLAGS += \
      -nostdinc++ \
      -isystem $(LIBSTDCXX_HEADERS_DIR) \
      -isystem $(LIBSTDCXX_HEADERS_DIR)/$(ACTUAL_HOST_TRIPLET)
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_CPPFLAGS += --sysroot=$(ANDROID_SYSROOT)
  TARGET_CPPFLAGS += -isystem $(ANDROID_SYSROOT)/usr/include/$(ANDROID_ABI4)
  TARGET_CPPFLAGS += -DANDROID
  TARGET_CPPFLAGS += -D__ANDROID_API__=$(ANDROID_NDK_API)
  CXXFLAGS += -D__STDC_VERSION__=199901L

  ifeq ($(X86),y)
    # On NDK r6, the macro _BYTE_ORDER never gets defined - workaround:
    TARGET_CPPFLAGS += -D_BYTE_ORDER=_LITTLE_ENDIAN
  endif
endif

####### compiler target

ifeq ($(HAVE_WIN32),y)
  TARGET_ARCH += -mwin32

  WINDRESFLAGS := -I$(OUT)/include -I$(SRC) $(TARGET_CPPFLAGS)
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
  ifneq ($(TARGET),CYGWIN)
    # link libstdc++-6.dll statically, so we don't have to distribute it
    TARGET_STATIC = y
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

ifeq ($(HOST_IS_ARM)$(TARGET_HAS_MALI),ny)
  # cross-crompiling for Cubieboard
  TARGET_LDFLAGS += --sysroot=$(CUBIE)
  TARGET_LDFLAGS += -L$(CUBIE)/lib/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(CUBIE)/usr/lib/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(CUBIE)/usr/local/stow/sunxi-mali/lib
endif

ifeq ($(TARGET_IS_KOBO),y)
  TARGET_LDFLAGS += --static
  ifeq ($(USE_CROSSTOOL_NG),y)
    ifeq ($(CLANG),y)
     TARGET_LDFLAGS += -B$(KOBO_TOOLCHAIN)
     TARGET_LDFLAGS += -B$(KOBO_TOOLCHAIN)/bin
     TARGET_LDFLAGS += --sysroot=$(KOBO_SYSROOT)
    endif
  else
    TARGET_LDFLAGS += -specs=$(abspath $(THIRDPARTY_LIBS_ROOT)/lib/musl-gcc.specs)
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -Wl,--no-undefined
  TARGET_LDFLAGS += --sysroot=$(ANDROID_TARGET_ROOT)
  ifeq ($(call bool_or,$(X64),$(MIPS64)),y)
    TARGET_LDFLAGS += -L$(ANDROID_TARGET_ROOT)/usr/lib64
  else
    TARGET_LDFLAGS += -L$(ANDROID_TARGET_ROOT)/usr/lib
  endif

  ifeq ($(ARMV7),y)
    TARGET_LDFLAGS += -Wl,--fix-cortex-a8
  endif
endif

ifneq ($(filter PC CYGWIN,$(TARGET)),)
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
  TARGET_LDLIBS += -latomic
endif

ifeq ($(TARGET),UNIX)
  ifeq ($(TARGET_IS_DARWIN),n)
  TARGET_LDLIBS += -lm
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDLIBS += -lc
  TARGET_LDLIBS += -lm

  TARGET_LDLIBS += -llog
  TARGET_LDLIBS += -lgcc
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
