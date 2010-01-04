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
MINIMAL := n
XSCALE := n
GTARGET := $(TARGET)

ifeq ($(TARGET),PPC2002)
  CONFIG_PPC2002 := y
endif

ifeq ($(TARGET),PPC2003)
  CONFIG_PPC2003 := y
endif

ifeq ($(TARGET),PPC2003X)
  CONFIG_PPC2003 := y
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
  MINIMAL := y
  XSCALE := y
endif

ifeq ($(TARGET),ALTAIRPORTRAIT)
  CONFIG_ALTAIR := y
  ALTAIR_PORTRAIT := y
  MINIMAL := y
  XSCALE := y
endif

ifeq ($(TARGET),PNA)
  CONFIG_PNA := y
  CONFIG_PPC2003 := y
  MINIMAL := n
endif

ifeq ($(TARGET),WM5)
  CONFIG_WM5 := y
  MINIMAL := n
endif

ifeq ($(TARGET),WM5X)
  CONFIG_WM5 := y
  MINIMAL := n
  XSCALE := y
endif

############# build and CPU info

ifeq ($(CONFIG_PC),y)
  TCPATH := i586-mingw32msvc-

  ifeq ($(WINHOST),y)
    TCPATH :=
  endif

  CPU := i586
  MCPU := -mcpu=$(CPU)
else

  ifeq ($(CONFIG_WINE),y)
    TCPATH := wine
    CPU := i586
    MCPU := -mcpu=$(CPU)
  else
    TCPATH := arm-mingw32ce-

    ifeq ($(XSCALE),y)
      CPU := xscale
      MCPU := -mcpu=$(CPU)
    else
      CPU :=
      MCPU :=
    endif

    ifeq ($(TARGET),PNA)
      CPU := arm1136j-s
      MCPU :=
    endif

    ifeq ($(CONFIG_PPC2002),y)
      CPU := strongarm1110
      MCPU := -mcpu=$(CPU)
    endif

    ifeq ($(TARGET),PPC2000)
      CPU := strongarm1110
      MCPU := -mcpu=$(CPU)
    endif
  endif
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
  ENABLE_SDL := y
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
  CE_PLATFORM := 300
  PCPU := ARM
endif

ifeq ($(CONFIG_PPC2002),y)
  CE_MAJOR := 3
  CE_MINOR := 00
  CE_PLATFORM := 310
  TARGET := PPC2002
  PCPU := ARM
endif
ifeq ($(CONFIG_PPC2003),y)
  CE_MAJOR := 4
  CE_MINOR := 00
  CE_PLATFORM := 400
  PCPU := ARMV4
endif

ifeq ($(CONFIG_WM5),y)
  CE_MAJOR := 5
  CE_MINOR := 00
  CE_PLATFORM := 500
  PCPU := ARMV4
endif

# armv4i
ifeq ($(CONFIG_ALTAIR),y)
  CE_MAJOR := 5
  CE_MINOR := 00
  CE_PLATFORM := 500
  TARGET := ALTAIR

  ifeq ($(ALTAIR_PORTRAIT),y)
    TARGET := ALTAIRPORTRAIT
  endif
endif

ifeq ($(CONFIG_PC),y)
  # armv4i
  CE_MAJOR := 5
  CE_MINOR := 00
  CE_PLATFORM := 500
  TARGET := PC
endif

ifeq ($(CONFIG_WINE),y)
  # armv4i
  CE_MAJOR := 5
  CE_MINOR := 00
  CE_PLATFORM := 500
  TARGET := WINE
  CONFIG_PC := y
endif

CE_VERSION := 0x0$(CE_MAJOR)$(CE_MINOR)

######## target definitions

TARGET_CPPFLAGS =
TARGET_INCLUDES =

ifeq ($(HAVE_WIN32),y)
  ifeq ($(CONFIG_PC),y)
    TARGET_CPPFLAGS += -D_WIN32_WINDOWS=$(CE_VERSION) -DWINVER=$(CE_VERSION) 
    TARGET_CPPFLAGS += -D_WIN32_IE=$(CE_VERSION) -DWINDOWSPC=1
  else
    TARGET_CPPFLAGS += -D_WIN32_WCE=$(CE_VERSION)
    TARGET_CPPFLAGS += -DWIN32_PLATFORM_PSPC=$(CE_PLATFORM)
  endif
endif

ifeq ($(CONFIG_PNA),y)
  TARGET_CPPFLAGS += -DCECORE -DPNA
  TARGET_CPPFLAGS += -DNOLINETO
endif

ifeq ($(TARGET),PPC2000)
  TARGET_CPPFLAGS += -DNOLINETO -DNOCLEARTYPE
endif

ifeq ($(TARGET),PPC2002)
  TARGET_CPPFLAGS += -DNOLINETO -DNOCLEARTYPE
endif

ifeq ($(CONFIG_PC),y)
  TARGET_CPPFLAGS += -D_WINDOWS -DWIN32 -DCECORE -DUNDER_CE=300

  ifeq ($(CONFIG_WINE),y)
    TARGET_CPPFLAGS += -D__WINE__
    # -mno-cygwin
  else
    TARGET_CPPFLAGS += -D_MBCS
  endif
else
  TARGET_CPPFLAGS += -D_ARM_

  ifeq ($(CONFIG_ALTAIR),y)
    TARGET_CPPFLAGS +=-IPPC2005 -DGNAV
    ifeq ($(ALTAIR_PORTRAIT),y)
      TARGET_CPPFLAGS += -DFORCEPORTRAIT
    endif
  endif
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_CPPFLAGS += -DHAVE_POSIX
  TARGET_CPPFLAGS += -DHAVE_STDINT_H
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

ifeq ($(TARGET),UNIX)
  TARGET_ARCH :=
else

  ifeq ($(CONFIG_PC),y)
    TARGET_ARCH := -mwindows -march=i586 -mms-bitfields
  else
    TARGET_ARCH := -mwin32 $(MCPU)
    ifeq ($(TARGET),PNA)
      TARGET_ARCH := -mwin32
    endif
  endif
  ifeq ($(TARGET),CYGWIN)
    TARGET_ARCH :=
  endif

  WINDRESFLAGS := -I$(SRC) $(TARGET_CPPFLAGS) -D_MINGW32_
  ifeq ($(CONFIG_ALTAIR),y)
    WINDRESFLAGS += -DGNAV
  endif
endif # UNIX

ifeq ($(TARGET),WINE)
  TARGET_ARCH += -m32
endif

####### linker configuration

TARGET_LDFLAGS =
TARGET_LDLIBS =

ifeq ($(HAVE_WIN32),y)
  ifneq ($(CONFIG_WINE),y)
    TARGET_LDFLAGS := -Wl,--major-subsystem-version=$(CE_MAJOR)
    TARGET_LDFLAGS += -Wl,--minor-subsystem-version=$(CE_MINOR)
    ifeq ($(CONFIG_PC),y)
      TARGET_LDFLAGS += -Wl,-subsystem,windows
    endif
  endif
endif

ifeq ($(HAVE_POSIX),y)
  TARGET_LDFLAGS += -lpthread
  TARGET_LDFLAGS += -lrt # for clock_gettime()
endif

ifeq ($(HAVE_WIN32),y)
  ifeq ($(CONFIG_PC),y)
    TARGET_LDLIBS := -lcomctl32 -lkernel32 -luser32 -lgdi32 -ladvapi32 -lwinmm -lmsimg32 -lstdc++
  else
    TARGET_LDLIBS := -lcommctrl -lstdc++
    ifeq ($(MINIMAL),n)
      TARGET_LDLIBS += -laygshell
      ifneq ($(TARGET),PNA)
        TARGET_LDLIBS += -limgdecmp
      endif
    endif
  endif
else
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
