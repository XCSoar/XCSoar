TARGETS = PC WIN64 \
	UNIX UNIX32 UNIX64 OPT \
	WAYLAND \
	PI PI2 CUBIE KOBO NEON \
	ANDROID ANDROID7 ANDROID7NEON ANDROID86 \
	ANDROIDAARCH64 ANDROIDX64 \
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
FAT_BINARY := n

TARGET_IS_DARWIN := n
TARGET_IS_LINUX := n
TARGET_IS_ANDROID := n
TARGET_IS_PI := n
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

ifeq ($(TARGET),ANDROID)
  # The default Android ABI is armeabi-v7a (ARMv7)
  override TARGET = ANDROID7
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

ifeq ($(TARGET),ANDROIDAARCH64)
  AARCH64 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDX64)
  X64 := y
  override TARGET = ANDROID
endif

ifeq ($(TARGET),ANDROIDFAT)
  FAT_BINARY := y
  override TARGET = ANDROID
endif

# real targets

ifeq ($(TARGET),PC)
  ifeq ($(X64),y)
    HOST_TRIPLET = x86_64-w64-mingw32
    TARGET_ARCH += -m64
  else
    HOST_TRIPLET = i686-w64-mingw32
    TARGET_ARCH += -march=i586
  endif
  TCPREFIX = $(HOST_TRIPLET)-

  ifneq ($(MINGWPATH),)
    TCPREFIX := $(MINGWPATH)
  endif

  ifeq ($(HOST_IS_WIN32),y)
    TCPREFIX :=
  endif

  WINVER = 0x0600
endif

ifeq ($(TARGET),CYGWIN)
  TCPREFIX :=

  TARGET_ARCH += -march=i586

  WINVER = 0x0600

  HAVE_POSIX := y
  HAVE_WIN32 := y
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
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
  ARMV6 = $(HOST_IS_ARMV6)
  ARMV7 = $(HOST_IS_ARMV7)
  NEON = $(HOST_HAS_NEON)
  TARGET_IS_ARMHF := $(call bool_or,$(ARMV7),$(TARGET_IS_PI))
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
  TARGET_IS_LINUX = y
  TARGET_IS_PI = y
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
endif

ifeq ($(TARGET),NEON)
  # Experimental target for generic ARMv7 with NEON on Linux
  override TARGET = UNIX
  ifeq ($(USE_CROSSTOOL_NG),y)
    HOST_TRIPLET ?= arm-unknown-linux-gnueabihf
  else
    HOST_TRIPLET ?= arm-linux-gnueabihf
  endif
  TCPREFIX ?= $(HOST_TRIPLET)-
  ifeq ($(CLANG),n)
    TARGET_ARCH += -mcpu=cortex-a8
  endif
  TARGET_IS_LINUX = y
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
  IOS_MIN_SUPPORTED_VERSION = 9.0
  HOST_TRIPLET = armv7-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
  endif
  LIBCXX = y
  CLANG = y
  TARGET_ARCH += -miphoneos-version-min=$(IOS_MIN_SUPPORTED_VERSION)
endif

ifeq ($(TARGET),IOS64)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 9.0
  HOST_TRIPLET = aarch64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
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
      LLVM_TARGET ?= arm-linux-gnueabihf
    else
      LLVM_TARGET ?= arm-linux-gnueabi
    endif
  endif
endif

ifeq ($(TARGET),ANDROID)
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r20

  ANDROID_SDK_PLATFORM = android-26
  ANDROID_NDK_API = 21

  # The naming of CPU ABIs, architectures, and various NDK directory names is an unholy mess.
  # Therefore a number of variables exist for each supported ABI.
  # Here is a brief outline where you can look up the names in the NDK in case that a new
  # architecture appears in the NDK, or names chane in new NDK versions:
  # ANDROID_ARCH: See $ANDROID_NDK/<NDK-Level>/. Name is without the prefix "arch-"
  # ANDROID_NDK_GCC_TOOLCHAIN_ABI: See $ANDROID_NDK/toolchains/. Name without the GCC version suffix (now "-4.9")
  # ANDROID_NDK_STL_LIB_ABI: See $ANDROID_NDK/sources/cxx-stl/llvm-libc++/libs
  # LLVM_TARGET: Open the appropriate compiler script in $ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin,
  #   e.g. aarch64-linux-android21-clang++ for AARCH64, NDK level 21, 
  #   and transcribe the value of the option "--target". 
  # HOST_TRIPLET = $(ANDROID_NDK_GCC_TOOLCHAIN_ABI)
  # ANDROID_APK_LIB_ABI: See https://developer.android.com/ndk/guides/abis#sa for valid names.
  # ANDROID_GCC_VERSION: Suffix of directories in $ANDROID_NDK/toolchains/. Since many NDK versions = "4.9".

  # Not architecture dependent
  ANDROID_GCC_VERSION = 4.9

  # Default is ARM V7a
  ANDROID_ARCH                  = arm
  ANDROID_NDK_GCC_TOOLCHAIN_ABI = arm-linux-androideabi
  ANDROID_NDK_STL_LIB_ABI       = armeabi-v7a
  ANDROID_APK_LIB_ABI           = armeabi-v7a
  LLVM_TARGET                  := armv7a-linux-androideabi
  HOST_TRIPLET                  = arm-linux-androideabi

  ifeq ($(X86),y)
    ANDROID_ARCH                  = x86
    ANDROID_NDK_GCC_TOOLCHAIN_ABI = x86
    ANDROID_NDK_STL_LIB_ABI       = x86
    ANDROID_APK_LIB_ABI           = x86
    LLVM_TARGET                  := i686-linux-android
    HOST_TRIPLET                  = i686-linux-android
  endif

  ifeq ($(AARCH64),y)
    ANDROID_ARCH                  = arm64
    ANDROID_NDK_GCC_TOOLCHAIN_ABI = aarch64-linux-android
    ANDROID_NDK_STL_LIB_ABI       = arm64-v8a
    ANDROID_APK_LIB_ABI           = arm64-v8a
    LLVM_TARGET                  := aarch64-linux-android
    HOST_TRIPLET                  = aarch64-linux-android
  endif

  ifeq ($(X64),y)
    ANDROID_ARCH                  = x86_64
    ANDROID_NDK_GCC_TOOLCHAIN_ABI = x86_64
    ANDROID_NDK_STL_LIB_ABI       = x86_64
    ANDROID_APK_LIB_ABI           = x86_64
    LLVM_TARGET                  := x86_64-linux-android
    HOST_TRIPLET                  = x86_64-linux-android
  endif

  # Like in the clang compiler scripts in the NDK add the NDK level to the LLVM target
  LLVM_TARGET := $(LLVM_TARGET)$(ANDROID_NDK_API)

  ANDROID_NDK_PLATFORM = android-$(ANDROID_NDK_API)

  ANDROID_SYSROOT = $(ANDROID_NDK)/sysroot
  ANDROID_NDK_PLATFORM_DIR = $(ANDROID_NDK)/platforms/$(ANDROID_NDK_PLATFORM)
  ANDROID_TARGET_ROOT = $(ANDROID_NDK_PLATFORM_DIR)/arch-$(ANDROID_ARCH)

  ANDROID_GCC_TOOLCHAIN_NAME = $(ANDROID_NDK_GCC_TOOLCHAIN_ABI)-$(ANDROID_GCC_VERSION)

  # clang is the mandatory compiler on Android
  override CLANG = y

  ANDROID_TOOLCHAIN_NAME = llvm
  override LIBCXX = y

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

  TCPREFIX = $(ANDROID_GCC_TOOLCHAIN)/bin/$(HOST_TRIPLET)-
  LLVM_PREFIX = $(ANDROID_TOOLCHAIN)/bin/


  ifeq ($(ARMV7),y)
    TARGET_ARCH += -march=armv7-a -mfloat-abi=softfp

    ifeq ($(NEON),y)
      TARGET_ARCH += -mfpu=neon
    else
      TARGET_ARCH += -mfpu=vfpv3-d16
    endif
  endif

  TARGET_ARCH += -fpic -funwind-tables

  TARGET_IS_LINUX := y
  TARGET_IS_ANDROID := y
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

######## target definitions

TARGET_INCLUDES =
TARGET_CXXFLAGS =
TARGET_CPPFLAGS = -I$(TARGET_OUTPUT_DIR)/include

ifneq ($(WINVER),)
  TARGET_CPPFLAGS += -DWINVER=$(WINVER) -D_WIN32_WINDOWS=$(WINVER)
  TARGET_CPPFLAGS += -D_WIN32_WINNT=$(WINVER) -D_WIN32_IE=$(WINVER)
endif

ifeq ($(HAVE_WIN32),y)
  TARGET_CPPFLAGS += -DWIN32_LEAN_AND_MEAN
  TARGET_CPPFLAGS += -DNOMINMAX
  ifeq ($(TARGET),CYGWIN)
  TARGET_CPPFLAGS += -DWIN32
  endif

  # kludge for the CURL build, which fails if _WIN32_WINNT >= 0x0600,
  # due to duplicate struct pollfd definition (winsock2.h and CURL's
  # select.h)
  TARGET_CPPFLAGS += -DHAVE_STRUCT_POLLFD
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

  # At least in Debian's arm-linux-gnueabihf GCC, PIE is enabled by default.
  # PIE brings no benefit for us, and we can get smaller binaries when disabling
  # it.
  TARGET_ARCH += -fno-PIE

  ifeq ($(CLANG),y)
    # Always use -fomit-frame-pointer for now, to circumvent Clang bug #34165.
    # https://bugs.llvm.org/show_bug.cgi?id=34165
    # http://www.openwall.com/lists/musl/2017/10/07/3
    TARGET_ARCH += -fomit-frame-pointer
  endif

  # We are using a GNU toolchain (triplet arm-linux-gnueabihf) by default, but
  # the actual host triplet is different.
  ACTUAL_HOST_TRIPLET = armv7a-a8neon-linux-musleabihf

  ifeq ($(USE_CROSSTOOL_NG),y)
    HOST_TRIPLET = $(ACTUAL_HOST_TRIPLET)
    LLVM_TARGET = $(ACTUAL_HOST_TRIPLET)
    KOBO_TOOLCHAIN = $(HOME)/x-tools/$(HOST_TRIPLET)
    KOBO_SYSROOT = $(KOBO_TOOLCHAIN)/$(HOST_TRIPLET)/sysroot
    TCPREFIX = $(KOBO_TOOLCHAIN)/bin/$(HOST_TRIPLET)-

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
  TARGET_CPPFLAGS += -isystem $(ANDROID_SYSROOT)/usr/include/$(HOST_TRIPLET)
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

ifeq ($(TARGET),PC)
  TARGET_LDFLAGS += -Wl,--major-subsystem-version=5
  TARGET_LDFLAGS += -Wl,--minor-subsystem-version=00

  # default to "console"; see SCREEN_LDLIBS
  TARGET_LDFLAGS += -Wl,-subsystem,console
endif

ifeq ($(HAVE_WIN32),y)
  ifneq ($(TARGET),CYGWIN)
    # link libstdc++-6.dll statically, so we don't have to distribute it
    TARGET_LDFLAGS += -static-libstdc++ -static-libgcc
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
  ifeq ($(X64),y)
    TARGET_LDFLAGS += -L$(ANDROID_TARGET_ROOT)/usr/lib64
    TARGET_LDFLAGS += -B$(ANDROID_TARGET_ROOT)/usr/lib64
  else
    TARGET_LDFLAGS += -L$(ANDROID_TARGET_ROOT)/usr/lib
    TARGET_LDFLAGS += -B$(ANDROID_TARGET_ROOT)/usr/lib
  endif

  ifeq ($(ARMV7),y)
    TARGET_LDFLAGS += -Wl,--fix-cortex-a8

    # workaround for "... uses VFP register arguments, output does not"
    TARGET_LDFLAGS += -Wl,--no-warn-mismatch
  endif

  # clang as linker driver adds the option '-pie' to the linker command.
  # This option is incompatible with the option '-shared'.
  TARGET_LDFLAGS += -no-pie

endif

ifeq ($(HAVE_WIN32),y)
  # for boost::asio::ip::tcp::acceptor
  TARGET_LDLIBS += -lmswsock
endif

ifneq ($(filter PC CYGWIN,$(TARGET)),)
  TARGET_LDLIBS += -lwinmm
endif

ifeq ($(TARGET),CYGWIN)
  TARGET_LDLIBS += -lintl
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

######## output files

TARGET_EXEEXT = .exe

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
endif

ifeq ($(TARGET),ANDROID)
  TARGET_EXEEXT :=
endif
