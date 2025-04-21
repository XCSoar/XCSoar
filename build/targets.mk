TARGETS = PC WIN64 \
	UNIX UNIX32 UNIX64 OPT \
	WAYLAND \
	FUZZER \
	PI PI2 CUBIE KOBO NEON \
	ANDROID ANDROID7 ANDROID86 \
	ANDROIDAARCH64 ANDROIDX64 \
	ANDROIDFAT \
	OSX64 MACOS IOS32 IOS64 IOS64SIM

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
TARGET_IS_IOS := n
TARGET_IS_LINUX := n
TARGET_IS_ANDROID := n
TARGET_IS_PI := n
TARGET_IS_PI32 := n
TARGET_IS_PI64 := n
TARGET_IS_KOBO := n
TARGET_IS_CUBIE := n
HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

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

ifeq ($(TARGET),ANDROID7)
  TARGET_IS_ARM = y
  TARGET_IS_ARMHF = y
  ARMV7 := y
  NEON := y
  override TARGET = ANDROID
  override TARGET_FLAVOR = ANDROID
endif

ifeq ($(TARGET),ANDROID86)
  X86 := y
  override TARGET = ANDROID
  override TARGET_FLAVOR = ANDROID
endif

ifeq ($(TARGET),ANDROIDAARCH64)
  AARCH64 := y
  override TARGET = ANDROID
  override TARGET_FLAVOR = ANDROID
endif

ifeq ($(TARGET),ANDROIDX64)
  X64 := y
  override TARGET = ANDROID
  override TARGET_FLAVOR = ANDROID
endif

ifeq ($(TARGET),ANDROIDFAT)
  FAT_BINARY := y
  override TARGET = ANDROID
  override TARGET_FLAVOR = ANDROID
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

ifeq ($(TARGET),OPT)
  override TARGET = UNIX
  DEBUG = n
endif

ifeq ($(TARGET),WAYLAND)
  # experimental target for the Wayland display server
  override TARGET = UNIX
  USE_WAYLAND = y
endif

ifeq ($(TARGET),FUZZER)
  # this target builds fuzzers using libfuzzer (https://llvm.org/docs/LibFuzzer.html)
  override TARGET = UNIX

  FUZZER = y
  LIBFUZZER = y
  CLANG = y
  VFB = y

  # Debian builds libfuzzer with GCC's libstdc++ instead of LLVM's libc++
  LIBCXX = n
endif

ifeq ($(TARGET),UNIX)
  # LOCAL_TCPREFIX is set in local-config.mk if configure was run.
  TCPREFIX := $(LOCAL_TCPREFIX)
  TCSUFFIX := $(LOCAL_TCSUFFIX)
  TARGET_IS_ARM = $(HOST_IS_ARM)
  TARGET_IS_PI = $(HOST_IS_PI)
  TARGET_IS_PI32 = $(call bool_and,$(HOST_IS_PI),$(HOST_IS_ARM))
  TARGET_IS_PI64 = $(call bool_and,$(HOST_IS_PI),$(HOST_IS_AARCH64))
  TARGET_IS_CUBIE = $(HOST_IS_CUBIE)
  ARMV6 = $(HOST_IS_ARMV6)
  ARMV7 = $(HOST_IS_ARMV7)
  NEON = $(HOST_HAS_NEON)
  TARGET_IS_ARMHF := $(call bool_or,$(ARMV7),$(TARGET_IS_PI32))
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
  TARGET_IS_CUBIE=y
endif

ifeq ($(TARGET),KOBO)
  # Experimental target for Kobo Mini
  override TARGET = NEON
  TARGET_IS_KOBO = y

  HOST_TRIPLET = armv7a-kobo-linux-musleabihf
endif

ifeq ($(TARGET),NEON)
  # Experimental target for generic ARMv7 with NEON on Linux
  override TARGET = UNIX
  HOST_TRIPLET ?= arm-linux-gnueabihf
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
  OSX_MIN_SUPPORTED_VERSION = 12.0
  HOST_TRIPLET = x86_64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  CLANG = y
  TARGET_ARCH += -mmacosx-version-min=$(OSX_MIN_SUPPORTED_VERSION)
endif

ifeq ($(TARGET),MACOS)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_OSX = y
  OSX_MIN_SUPPORTED_VERSION = 12.0
  HOST_TRIPLET = aarch64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  CLANG = y
  TARGET_ARCH += -mmacosx-version-min=$(OSX_MIN_SUPPORTED_VERSION)
  TARGET_IS_ARM = y
endif

ifeq ($(TARGET),IOS32)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 10.0
  HOST_TRIPLET = armv7-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
  endif
  CLANG = y
  TARGET_ARCH += -miphoneos-version-min=$(IOS_MIN_SUPPORTED_VERSION)
endif

ifeq ($(TARGET),IOS64)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 11.0
  HOST_TRIPLET = aarch64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
  endif
  CLANG = y
  TARGET_ARCH += -miphoneos-version-min=$(IOS_MIN_SUPPORTED_VERSION) -arch arm64
  ASFLAGS += -arch arm64
endif

ifeq ($(TARGET),IOS64SIM)
  override TARGET = UNIX
  TARGET_IS_DARWIN = y
  TARGET_IS_IOS = y
  IOS_MIN_SUPPORTED_VERSION = 11.0
  HOST_TRIPLET = aarch64-apple-darwin
  LLVM_TARGET = $(HOST_TRIPLET)
  ifeq ($(HOST_IS_DARWIN),y)
    DARWIN_SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk
  endif
  CLANG = y
  TARGET_ARCH += -mios-simulator-version-min=$(IOS_MIN_SUPPORTED_VERSION) -arch arm64
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
  ANDROID_NDK ?= $(HOME)/opt/android-ndk-r26d

  ANDROID_SDK_PLATFORM = android-33
  ANDROID_NDK_API = 21

  # The naming of CPU ABIs, architectures, and various NDK directory names is an unholy mess.
  # Therefore a number of variables exist for each supported ABI.
  # Here is a brief outline where you can look up the names in the NDK in case that a new
  # architecture appears in the NDK, or names chane in new NDK versions:
  # LLVM_TARGET: Open the appropriate compiler script in $ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin,
  #   e.g. aarch64-linux-android21-clang++ for AARCH64, NDK level 21, 
  #   and transcribe the value of the option "--target". 
  # ANDROID_APK_LIB_ABI: See https://developer.android.com/ndk/guides/abis#sa for valid names.

  # Default is ARM V7a
  ANDROID_APK_LIB_ABI           = armeabi-v7a
  LLVM_TARGET                  := armv7a-linux-androideabi

  ifeq ($(X86),y)
    ANDROID_APK_LIB_ABI           = x86
    LLVM_TARGET                  := i686-linux-android
  endif

  ifeq ($(AARCH64),y)
    ANDROID_APK_LIB_ABI           = arm64-v8a
    LLVM_TARGET                  := aarch64-linux-android
  endif

  ifeq ($(X64),y)
    ANDROID_APK_LIB_ABI           = x86_64
    LLVM_TARGET                  := x86_64-linux-android
  endif

  XCSOAR_ARCH_SUBDIR = /$(ANDROID_APK_LIB_ABI)

  HOST_TRIPLET := $(LLVM_TARGET)

  # Like in the clang compiler scripts in the NDK add the NDK level to the LLVM target
  LLVM_TARGET := $(LLVM_TARGET)$(ANDROID_NDK_API)

  # clang is the mandatory compiler on Android
  override CLANG = y
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

  LLVM_PREFIX = $(ANDROID_NDK)/toolchains/llvm/prebuilt/$(ANDROID_HOST_TAG)/bin/
  TCPREFIX = $(LLVM_PREFIX)/llvm-

  ifeq ($(ARMV7),y)
    TARGET_ARCH += -mfloat-abi=softfp -mfpu=neon
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

ifeq ($(FUZZER),y)
  ifeq ($(LIBFUZZER),y)
    SANITIZE = fuzzer,address
  endif
  TARGET_CPPFLAGS += -DFUZZER
endif

ifneq ($(WINVER),)
  TARGET_CPPFLAGS += -DWINVER=$(WINVER) -D_WIN32_WINDOWS=$(WINVER)
  TARGET_CPPFLAGS += -D_WIN32_WINNT=$(WINVER) -D_WIN32_IE=$(WINVER)
endif

ifeq ($(HAVE_WIN32),y)
  TARGET_CPPFLAGS += -DWIN32_LEAN_AND_MEAN
  TARGET_CPPFLAGS += -DNOMINMAX

  # kludge for the CURL build, which fails if _WIN32_WINNT >= 0x0600,
  # due to duplicate struct pollfd definition (winsock2.h and CURL's
  # select.h)
  TARGET_CPPFLAGS += -DHAVE_STRUCT_POLLFD
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_CPPFLAGS += -DHAVE_POSIX
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

ifeq ($(HOST_IS_ARM)$(TARGET_IS_CUBIE),ny)
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

  TARGET_CXXFLAGS += -Wno-psabi

  TCPREFIX = $(abspath $(THIRDPARTY_LIBS_DIR))/bin/$(HOST_TRIPLET)-
endif

ifeq ($(TARGET),ANDROID)
  TARGET_CPPFLAGS += -DANDROID
  CXXFLAGS += -D__STDC_VERSION__=199901L
  # disable pretty printer embedding
  CXXFLAGS += -DBOOST_ALL_NO_EMBEDDED_GDB_SCRIPTS
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
  # link libstdc++-6.dll statically, so we don't have to distribute it
  TARGET_LDFLAGS += -static-libstdc++ -static-libgcc
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

ifeq ($(HOST_IS_ARM)$(TARGET_IS_CUBIE),ny)
  # cross-crompiling for Cubieboard
  TARGET_LDFLAGS += -L/usr/arm-linux-gnueabihf/lib --sysroot=$(CUBIE)
  TARGET_LDFLAGS += -L$(CUBIE)/lib/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(CUBIE)/usr/lib/arm-linux-gnueabihf
  TARGET_LDFLAGS += -L$(CUBIE)/usr/local/stow/sunxi-mali/lib
endif

ifeq ($(TARGET_IS_KOBO),y)
  TARGET_LDFLAGS += --static

  # Dirty workaround for a musl/libstdc++ problem: libstdc++ imports
  # these symbols "weakly", and apparently the linker then doesn't
  # pick up libc.a(pthread_cond_*.o); these linker options force the
  # linker to use them.  This needs a proper solution!
  TARGET_LDFLAGS += -Wl,-u,pthread_cond_signal -Wl,-u,pthread_cond_broadcast -Wl,-u,pthread_cond_wait
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDFLAGS += -Wl,--no-undefined

  ifeq ($(ARMV7),y)
    TARGET_LDFLAGS += -Wl,--fix-cortex-a8
  endif
endif

ifeq ($(HAVE_WIN32),y)
  TARGET_LDLIBS += -lwinmm
endif

ifeq ($(TARGET),UNIX)
  ifeq ($(TARGET_IS_DARWIN),n)
  TARGET_LDLIBS += -lm
  endif
endif

ifeq ($(TARGET),ANDROID)
  TARGET_LDLIBS += -llog -landroid
endif

######## output files

TARGET_EXEEXT = .exe

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
endif

ifeq ($(TARGET),ANDROID)
  TARGET_EXEEXT :=
endif
