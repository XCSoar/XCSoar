LINK = $(CXX)

ld-flags = $(ALL_LDFLAGS) $(TARGET_ARCH)  $(EXTRA_LDFLAGS)
ld-libs = $(ALL_LDLIBS)

# Generates a program linking rule.
#
# Example: $(eval $(call link-program,Foo,FOO))
#
# Arguments: NAME, PREFIX
#
# NAME is the name of the program binary, without the path and without
# the suffix (.exe).
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
#  _STRIP: if "y", then the program will be stripped
#
define link-program

$(2)_BIN = $$(TARGET_BIN_DIR)/$(1)$$(TARGET_EXEEXT)

# Disable stripping on UNIX, because that should usually be done
# during installation (the Debian package is explicitly stripped), and
# we often need the debug symbols while developing.
ifeq ($$(TARGET),UNIX)
ifeq ($$(TARGET_IS_KOBO),n) # .. but do strip Kobo binaries
$(2)_STRIP := n
endif
endif

ifeq ($$($(2)_STRIP),y)
$(2)_NOSTRIP = $$(TARGET_BIN_DIR)/$(1)-ns$$(TARGET_EXEEXT)
else
$(2)_NOSTRIP = $$($(2)_BIN)
endif

$(2)_DEPENDS_FLAT = $$(call flat-depends,$(2))

$(2)_LDADD += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_LDADD))
$(2)_LDLIBS += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_LDLIBS))

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS)
$$($(2)_OBJS): CPPFLAGS += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_CPPFLAGS))

# Link the unstripped binary
$$($(2)_NOSTRIP): $$($(2)_OBJS) $$($(2)_LDADD) $$(TARGET_LDADD) | $$(TARGET_BIN_DIR)/dirstamp
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LINK) $$(ld-flags) -o $$@ $$^ $$($(2)_LDLIBS) $$(ld-libs)

# Strip the binary (optional)
ifeq ($$($(2)_STRIP),y)
$$($(2)_BIN): $$($(2)_NOSTRIP)
	@$$(NQ)echo "  STRIP   $$@"
	$$(Q)$$(STRIP) $$< -o $$@
endif

endef



# Generates a shared library linking rule.
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
#  _NO_LIB_PREFIX: if "y", then the output file will not be prefixed with "lib"
#
#  _STRIP: if "y", then the library will be stripped
#
define link-shared-library

$(2)_LIB_PREFIX = lib
ifeq ($$($(2)_NO_LIB_PREFIX),y)
$(2)_LIB_PREFIX =
endif

$(2)_BIN = $$(ABI_BIN_DIR)/$$($(2)_LIB_PREFIX)$(1).so

ifeq ($$($(2)_STRIP),y)
$(2)_NOSTRIP = $$(ABI_BIN_DIR)/$$($(2)_LIB_PREFIX)$(1)-ns.so
else
$(2)_NOSTRIP = $$($(2)_BIN)
endif

$(2)_DEPENDS_FLAT = $$(call flat-depends,$(2))

$(2)_LDADD += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_LDADD))
$(2)_LDLIBS += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_LDLIBS))

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS)
$$($(2)_OBJS): CPPFLAGS += $$(foreach i,$$($(2)_DEPENDS_FLAT),$$($$(i)_CPPFLAGS))

# Link the unstripped binary
ifneq ($(TARGET_IS_DARWIN),y)
$$($(2)_NOSTRIP): LDFLAGS += -shared -Wl,-Bsymbolic
endif

$$($(2)_NOSTRIP): $$($(2)_OBJS) $$($(2)_LDADD) $$(TARGET_LDADD) | $$(ABI_BIN_DIR)/dirstamp
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LINK) $$(ld-flags) -o $$@ $$^ $$($(2)_LDLIBS) $$(ld-libs)

# Strip the binary (optional)
ifeq ($$($(2)_STRIP),y)
$$($(2)_BIN): $$($(2)_NOSTRIP)
	@$$(NQ)echo "  STRIP   $$@"
	$$(Q)$$(STRIP) $$< -o $$@
endif

endef


# Generates a static library linking rule.
#
# Example: $(eval $(call link-library,foo,FOO))
#
# Arguments: NAME, PREFIX
define link-library

$(2)_BIN = $$(ABI_OUTPUT_DIR)/$(1).a

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CFLAGS += $$($(2)_CFLAGS) $$($(2)_CFLAGS_INTERNAL)
$$($(2)_OBJS): CXXFLAGS += $$($(2)_CXXFLAGS) $$($(2)_CXXFLAGS_INTERNAL)
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS) $$($(2)_CPPFLAGS_INTERNAL)
$$($(2)_OBJS): CPPFLAGS += $(patsubst %,$$(%_CPPFLAGS),$($(2)_DEPENDS))

# Link
$$($(2)_BIN): $$($(2)_OBJS)
	@$$(NQ)echo "  AR      $$@"
	$$(Q)$$(AR) $$(ARFLAGS) $$@ $$^

$(2)_LIBS = $$($(2)_BIN)
$(2)_LDADD = $$($(2)_BIN)

endef
