# One version. Both URLs are the same tarball; the SourceForge link is
# the fallback. Renovate updates BOOST_VERSION and leaves BOOST_SHA256.
BOOST_VERSION = 1.92.0
BOOST_SHA256 = 5c1d40cb8e19adbf740a4ec2da35b3e58f3f5804b1dce44deb53df72193cbc6c
BOOST_UNDERSCORE = $(subst .,_,$(BOOST_VERSION))
BOOST_URL = https://archives.boost.io/release/$(BOOST_VERSION)/source/boost_$(BOOST_UNDERSCORE).tar.bz2
BOOST_ALTERNATIVE_URL = https://sourceforge.net/projects/boost/files/boost/$(BOOST_VERSION)/boost_$(BOOST_UNDERSCORE).tar.bz2/download

BOOST_TARBALL_NAME = $(notdir $(BOOST_URL))
BOOST_TARBALL = $(DOWNLOAD_DIR)/$(BOOST_TARBALL_NAME)
BOOST_BASE_NAME = $(patsubst %.tar.bz2,%,$(BOOST_TARBALL_NAME))
BOOST_SRC = $(OUT)/src/$(BOOST_BASE_NAME)
BOOST_PATCHES_DIR = $(topdir)/lib/boost/patches
BOOST_PATCHES = $(addprefix $(BOOST_PATCHES_DIR)/,$(shell cat $(BOOST_PATCHES_DIR)/series))

$(BOOST_TARBALL): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(BOOST_URL) $(BOOST_ALTERNATIVE_URL) $(BOOST_SHA256) $(DOWNLOAD_DIR)

BOOST_UNTAR_STAMP = $(OUT)/src/stamp-$(BOOST_BASE_NAME)
$(BOOST_UNTAR_STAMP): $(BOOST_TARBALL) $(BOOST_PATCHES_DIR)/series $(BOOST_PATCHES) | $(OUT)/src/dirstamp
	@$(NQ)echo "  UNTAR   $(BOOST_TARBALL_NAME)"
	$(Q)rm -rf $(BOOST_SRC)
	$(Q)tar xjfC $< $(OUT)/src $(BOOST_BASE_NAME)/boost
	$(Q)cd $(BOOST_SRC) && QUILT_PATCHES=$(abspath $(BOOST_PATCHES_DIR)) quilt push -a -q
	@touch $@

.PHONY: boost
boost: $(BOOST_UNTAR_STAMP)

# We use only the header-only Boost libraries, so no linker flags
# required.
BOOST_LDLIBS =

# reduce Boost header bloat a bit
BOOST_CPPFLAGS = -isystem $(OUT)/src/$(BOOST_BASE_NAME)
BOOST_CPPFLAGS += -DBOOST_NO_IOSTREAM -DBOOST_MATH_NO_LEXICAL_CAST
BOOST_CPPFLAGS += -DBOOST_UBLAS_NO_STD_CERR
BOOST_CPPFLAGS += -DBOOST_ERROR_CODE_HEADER_ONLY
BOOST_CPPFLAGS += -DBOOST_SYSTEM_NO_DEPRECATED
BOOST_CPPFLAGS += -DBOOST_NO_STD_LOCALE -DBOOST_LEXICAL_CAST_ASSUME_C_LOCALE

ifeq ($(HAVE_WIN32),y)
BOOST_CPPFLAGS += -DBOOST_SYSTEM_DISABLE_THREADS
endif

# Prevent Boost from using the deprecated std::unary_function class
BOOST_CPPFLAGS += -DBOOST_NO_CXX98_FUNCTION_BASE
