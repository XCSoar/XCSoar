LINK = $(CXX)

ifeq ($(ICF),y)
LINK += -fuse-ld=gold -Wl,--icf=all
endif

ld-flags = $(ALL_LDFLAGS) $(TARGET_ARCH)
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

$(2)_LDADD += $(patsubst %,$$(%_LDADD),$($(2)_DEPENDS))
$(2)_LDLIBS += $(patsubst %,$$(%_LDLIBS),$($(2)_DEPENDS))

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS)
$$($(2)_OBJS): CPPFLAGS += $(patsubst %,$$(%_CPPFLAGS),$($(2)_DEPENDS))

ifeq ($$(LLVM),y)

# Link all LLVM bitcode files together
$$($(2)_NOSTRIP).bc: $$($(2)_OBJS) $$($(2)_LDADD) | $$(TARGET_BIN_DIR)/dirstamp
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LLVM_LINK) -o $$@ $$^

# Optimise the large bitcode file
$$($(2)_NOSTRIP)-opt.bc: $$($(2)_NOSTRIP).bc
	@$$(NQ)echo "  OPT     $$@"
	$$(Q)$$(LLVM_OPT) -o $$@ $$^ -std-compile-opts -std-link-opts -O2 $(TARGET_LLVM_FLAGS)

# Compile to native CPU assembly
$$($(2)_NOSTRIP).s: $$($(2)_NOSTRIP)-opt.bc
	@$$(NQ)echo "  LLC     $$@"
	$$(Q)$$(LLVM_LLC) -o $$@ $$^ -O2 $(TARGET_LLVM_FLAGS)

# Assemble to native CPU object
$$($(2)_NOSTRIP).o: $$($(2)_NOSTRIP).s
	@$$(NQ)echo "  AS      $$@"
	$$(Q)$(AS) -o $$@ $$^

# Link the unstripped binary
$$($(2)_NOSTRIP): $$($(2)_NOSTRIP).o $$(TARGET_LDADD)
	@$$(NQ)echo "  CLANG   $$@"
	$$(Q)$$(LINK) $$(ld-flags) -o $$@ $$^ $$($(2)_LDLIBS) $$(ld-libs)
else

# Link the unstripped binary
$$($(2)_NOSTRIP): $$($(2)_OBJS) $$($(2)_LDADD) $$(TARGET_LDADD) | $$(TARGET_BIN_DIR)/dirstamp
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LINK) $$(ld-flags) -o $$@ $$^ $$($(2)_LDLIBS) $$(ld-libs)

endif

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

$(2)_LDADD += $(patsubst %,$$(%_LDADD),$($(2)_DEPENDS))
$(2)_LDLIBS += $(patsubst %,$$(%_LDLIBS),$($(2)_DEPENDS))

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS)
$$($(2)_OBJS): CPPFLAGS += $(patsubst %,$$(%_CPPFLAGS),$($(2)_DEPENDS))

# Link the unstripped binary
ifneq ($(HOST_IS_DARWIN),y)
$$($(2)_NOSTRIP): LDFLAGS += -Wl,-shared,-Bsymbolic
endif

ifeq ($$(TARGET),ANDROID)
$$($(2)_NOSTRIP): LDFLAGS += -nostdlib
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

ifeq ($$(LLVM),y)
$(2)_BIN = $$(ABI_OUTPUT_DIR)/$(1).bc
else
$(2)_BIN = $$(ABI_OUTPUT_DIR)/$(1).a
endif

# Compile
$(2)_OBJS = $$(call SRC_TO_OBJ,$$($(2)_SOURCES))
$$($(2)_OBJS): CFLAGS += $$($(2)_CFLAGS) $$($(2)_CFLAGS_INTERNAL)
$$($(2)_OBJS): CXXFLAGS += $$($(2)_CXXFLAGS) $$($(2)_CXXFLAGS_INTERNAL)
$$($(2)_OBJS): CPPFLAGS += $$($(2)_CPPFLAGS) $$($(2)_CPPFLAGS_INTERNAL)
$$($(2)_OBJS): CPPFLAGS += $(patsubst %,$$(%_CPPFLAGS),$($(2)_DEPENDS))

# Link
$$($(2)_BIN): $$($(2)_OBJS)
ifeq ($$(LLVM),y)
	@$$(NQ)echo "  LINK    $$@"
	$$(Q)$$(LLVM_LINK) -o $$@ $$^
else
	@$$(NQ)echo "  AR      $$@"
	$$(Q)$$(AR) $$(ARFLAGS) $$@ $$^
endif

$(2)_LIBS = $$($(2)_BIN)
$(2)_LDADD = $$($(2)_BIN)

endef
