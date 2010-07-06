TARGETS = PC PPC2000 PPC2002 PPC2003 PPC2003X PNA WM5 ALTAIR ALTAIRPORTRAIT WINE UNIX

# These targets are built when you don't specify the TARGET variable.
DEFAULT_TARGETS = PC PPC2002 PPC2003 PNA WM5 ALTAIR WINE

CONFIG_PPC2002 := n
CONFIG_PPC2003 := n
CONFIG_ALTAIR := n
CONFIG_PC := n
CONFIG_WINE := n
ALTAIR_PORTRAIT := n
CONFIG_PNA := n
HAVE_CE := n
HAVE_FPU := y
XSCALE := n
GTARGET := $(TARGET)

ifeq ($(TARGET),PPC2000)
  HAVE_CE := y
endif

ifeq ($(TARGET),PPC2002)
  CONFIG_PPC2002 := y
  HAVE_CE := y
endif

ifeq ($(TARGET),PPC2003)
  CONFIG_PPC2003 := y
  HAVE_CE := y
endif

ifeq ($(TARGET),PPC2003X)
  CONFIG_PPC2003 := y
  HAVE_CE := y
  XSCALE := y
  GTARGET := PPC2003
endif

ifeq ($(TARGET),PC)
  CONFIG_PC := y
endif

ifeq ($(TARGET),WINE)
  CONFIG_WINE := y
endif

ifeq ($(TARGET),ALTAIR)
  CONFIG_ALTAIR := y
  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),ALTAIRPORTRAIT)
  CONFIG_ALTAIR := y
  ALTAIR_PORTRAIT := y
  HAVE_CE := y
  XSCALE := y
endif

ifeq ($(TARGET),PNA)
  CONFIG_PNA := y
  CONFIG_PPC2003 := y
  HAVE_CE := y
endif

ifeq ($(TARGET),WM5)
  CONFIG_WM5 := y
  HAVE_CE := y
endif

ifeq ($(TARGET),WM5X)
  CONFIG_WM5 := y
  HAVE_CE := y
  XSCALE := y
endif

############# build and CPU info

ifeq ($(CONFIG_PC),y)
  TCPATH := i586-mingw32msvc-

  ifeq ($(WINHOST),y)
    TCPATH :=
  endif

  CPU := i586
  MCPU := -march=$(CPU)
endif

ifeq ($(CONFIG_WINE),y)
  TCPATH := wine
  CPU := i586
  MCPU := -march=$(CPU)
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

ifeq ($(TARGET),UNIX)
  TCPATH :=
endif

ifeq ($(TARGET),CYGWIN)
  TCPATH :=
endif

############# platform info

HAVE_POSIX := n
HAVE_WIN32 := y
HAVE_MSVCRT := y

ifeq ($(TARGET),WINE)
  HAVE_POSIX := y
  HAVE_MSVCRT := n
endif

ifeq ($(TARGET),UNIX)
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := y
endif

ifeq ($(TARGET),CYGWIN)
  HAVE_POSIX := y
  HAVE_WIN32 := n
  HAVE_MSVCRT := n
  HAVE_VASPRINTF := n
endif

ifeq ($(TARGET),PPC2000)
  CE_MAJOR := 3
  CE_MINOR := 00
  PCPU := ARM
endif

ifeq ($(CONFIG_PPC2002),y)
  CE_MAJOR := 3
  CE_MINOR := 10
  TARGET := PPC2002
  PCPU := ARM
endif
ifeq ($(CONFIG_PPC2003),y)
  CE_MAJOR := 4
  CE_MINOR := 00
  PCPU := ARMV4
endif

ifeq ($(CONFIG_WM5),y)
  CE_MAJOR := 5
  CE_MINOR := 00
  PCPU := ARMV4
endif

# armv4i
ifeq ($(CONFIG_ALTAIR),y)
  CE_MAJOR := 5
  CE_MINOR := 00
  TARGET := ALTAIR

  ifeq ($(ALTAIR_PORTRAIT),y)
    TARGET := ALTAIRPORTRAIT
  endif
endif

ifeq ($(CONFIG_PC),y)
  WINVER = 0x0500
  TARGET := PC
endif

ifeq ($(CONFIG_WINE),y)
  WINVER = 0x0500
  TARGET := WINE
  CONFIG_PC := y
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
  ifeq ($(CONFIG_PC),y)
    TARGET_CPPFLAGS += -DWINDOWSPC=1
  endif
endif

ifeq ($(CONFIG_PNA),y)
  TARGET_CPPFLAGS += -DPNA
  TARGET_CPPFLAGS += -DNOLINETO
endif

ifeq ($(TARGET),PPC2000)
  TARGET_CPPFLAGS += -DNOLINETO -DNOCLEARTYPE
endif

ifeq ($(TARGET),PPC2002)
  TARGET_CPPFLAGS += -DNOLINETO -DNOCLEARTYPE
endif

ifeq ($(CONFIG_WINE),y)
  TARGET_CPPFLAGS += -D__WINE__
  # -mno-cygwin
endif

ifeq ($(CONFIG_ALTAIR),y)
  TARGET_CPPFLAGS += -DGNAV
  TARGET_CPPFLAGS += -DDISABLEAUDIO
  ifeq ($(ALTAIR_PORTRAIT),y)
    TARGET_CPPFLAGS += -DFORCEPORTRAIT
  endif
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_CPPFLAGS += -DHAVE_POSIX
  TARGET_CPPFLAGS += -DHAVE_STDINT_H
  TARGET_CPPFLAGS += -DHAVE_UNISTD_H
endif

ifeq ($(HAVE_MSVCRT),y)
  TARGET_CPPFLAGS += -DHAVE_MSVCRT
  TARGET_CPPFLAGS += -DUNICODE -D_UNICODE
endif

ifeq ($(HAVE_POSIX)$(CONFIG_PC),nn)
  TARGET_INCLUDES += -I$(SRC)/mingw32compat
endif

ifeq ($(HAVE_WIN32),n)
  TARGET_INCLUDES += -I$(SRC)/unix
endif

ifeq ($(TARGET),WINE)
  TARGET_INCLUDES += -I$(SRC)/wine
endif

ifeq ($(TARGET),UNIX)
  TARGET_CPPFLAGS += -DDISABLEAUDIO
  TARGET_CPPFLAGS += $(shell pkg-config --cflags gconf-2.0)
  TARGET_LDLIBS += $(shell pkg-config --libs gconf-2.0)
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
  TARGET_ARCH :=
endif

####### linker configuration

TARGET_LDFLAGS =
TARGET_LDLIBS =

ifeq ($(TARGET),PC)
  TARGET_LDFLAGS += -Wl,--major-subsystem-version=5
  TARGET_LDFLAGS += -Wl,--minor-subsystem-version=00
  TARGET_LDFLAGS += -Wl,-subsystem,windows
endif

ifeq ($(HAVE_WIN32),y)
  ifeq ($(CONFIG_PC),n)
    TARGET_LDFLAGS := -Wl,--major-subsystem-version=$(CE_MAJOR)
    TARGET_LDFLAGS += -Wl,--minor-subsystem-version=$(CE_MINOR)
  endif

  ifneq ($(TARGET),WINE)
    # link libstdc++-6.dll statically, so we don't have to distribute it
    TARGET_LDFLAGS += -static
  endif
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_LDFLAGS += -lpthread
  TARGET_LDFLAGS += -lrt # for clock_gettime()
endif

ifeq ($(CONFIG_PC),y)
  TARGET_LDLIBS := -lcomctl32 -lkernel32 -luser32 -lgdi32 -ladvapi32 -lwinmm -lmsimg32 -lstdc++
endif

ifeq ($(HAVE_CE),y)
  TARGET_LDLIBS := -lcommctrl -lstdc++

  ifeq ($(findstring $(TARGET),PNA ALTAIR ALTAIRPORTRAIT),)
    TARGET_CPPFLAGS += -DHAVE_NOTE_PRJ_DLL
    TARGET_CPPFLAGS += -DHAVE_AYGSHELL_DLL

    TARGET_LDLIBS += -limgdecmp
    TARGET_CPPFLAGS += -DHAVE_IMGDECMP_DLL
  endif
endif

ifeq ($(TARGET),UNIX)
  TARGET_LDLIBS := -lstdc++ -lm
endif

######## output files

TARGET_EXEEXT = .exe
NOSTRIP_SUFFIX = -ns

ifeq ($(CONFIG_WINE),y)
  TARGET_EXEEXT :=
  NOSTRIP_SUFFIX :=
endif

ifeq ($(TARGET),UNIX)
  TARGET_EXEEXT :=
  NOSTRIP_SUFFIX :=
endif
