name-to-so = $(patsubst %,$(TARGET_BIN_DIR)/%.so,$(1))

# Generates a shared library linking rule for python.
#
# Example: $(eval $(call link-shared-library,Foo,FOO))
#
# Arguments: NAME, PREFIX
#
# NAME is the name of the library binary, without the path, without
# the prefix (lib) and without the suffix (.exe).
#
# PREFIX is a prefix for variables that will hold detailed information
# about what is linked, and now.  These must be set before this
# generator function is called.  The following variables will be used:
#
#  _SOURCES: a list of source files
#
#  _CPPFLAGS: preprocessor flags for the compiler
#
#  _LDADD: a list of static libraries that will be linked into the binary
#
#  _LDFLAGS: linker flags
#
#  _DEPENDS: a list of library names this executable depends on; it
#  will use its CPPFLAGS, LDADD and LDFLAGS
#
#  _STRIP: if "y", then the library will be stripped
#
define link-python-library

$(2)_BIN = $$(TARGET_BIN_DIR)/$(1).so

ifeq ($$($(2)_STRIP),y)
$(2)_NOSTRIP = $$(TARGET_BIN_DIR)/$(1)-ns.so
else
$(2)_NOSTRIP = $$($(2)_BIN)
endif

$(2)_LDADD += $(patsubst %,$$(%_LDADD),$($(2)_DEPENDS))
$(2)_LDLIBS += $(patsubst %,$$(%_LDLIBS),$($(2)_DEPENDS))

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS)
$$($(2)_OBJS): CPPFLAGS += $(patsubst %,$$(%_CPPFLAGS),$($(2)_DEPENDS))

# Link the unstripped binary
$$($(2)_NOSTRIP): LDFLAGS += -Wl,-shared,-Bsymbolic
ifeq ($$(TARGET),ANDROID)
$$($(2)_NOSTRIP): LDFLAGS += -nostdlib
endif

$$($(2)_NOSTRIP): $$($(2)_OBJS) $$($(2)_LDADD) $$(TARGET_LDADD) | $$(TARGET_BIN_DIR)/dirstamp
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LINK) $$(ld-flags) -o $$@ $$^ $$(ld-libs) $$($(2)_LDLIBS)

# Strip the binary (optional)
ifeq ($$($(2)_STRIP),y)
$$($(2)_BIN): $$($(2)_NOSTRIP)
	@$$(NQ)echo "  STRIP   $$@"
	$$(Q)$$(STRIP) $$< -o $$@
endif

endef

python: $(call name-to-so,xcsoar)

PYTHON_SOURCES = \
	$(DEBUG_REPLAY_SOURCES) \
	$(SRC)/IGC/IGCFix.cpp \
	$(ENGINE_SRC_DIR)/Trace/Point.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
        $(SRC)/Computer/Wind/Settings.cpp \
        $(SRC)/Computer/Wind/WindEKF.cpp \
        $(SRC)/Computer/Wind/WindEKFGlue.cpp \
        $(SRC)/Computer/Wind/CirclingWind.cpp \
        $(SRC)/Computer/Wind/Computer.cpp \
        $(SRC)/Computer/Wind/MeasurementList.cpp \
        $(SRC)/Computer/Wind/Store.cpp \
	$(TEST_SRC_DIR)/FlightPhaseDetector.cpp \
	$(PYTHON_SRC)/Flight/Flight.cpp \
	$(PYTHON_SRC)/Flight/DebugReplayVector.cpp \
	$(PYTHON_SRC)/Flight/FlightTimes.cpp \
	$(PYTHON_SRC)/Flight/DouglasPeuckerMod.cpp \
	$(PYTHON_SRC)/Flight/AnalyseFlight.cpp \
        $(PYTHON_SRC)/Tools/GoogleEncode.cpp \
	$(PYTHON_SRC)/PythonConverters.cpp \
	$(PYTHON_SRC)/PythonGlue.cpp
PYTHON_LDADD = $(DEBUG_REPLAY_LDADD)
PYTHON_LDLIBS = -lpython2.7
PYTHON_DEPENDS = CONTEST WAYPOINT UTIL GEO TIME MATH ZZIP
PYTHON_CPPFLAGS = -I/usr/include/python2.7 \
	-I$(TEST_SRC_DIR) -Wno-write-strings
PYTHON_FILTER_FLAGS = -Wwrite-strings
$(eval $(call link-python-library,xcsoar,PYTHON))
