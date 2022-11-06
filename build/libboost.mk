BOOST_URL = https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.tar.bz2
BOOST_ALTERNATIVE_URL = https://sourceforge.net/projects/boost/files/boost/1.80.0/boost_1_80_0.tar.bz2/download
BOOST_MD5 = 1e19565d82e43bc59209a168f5ac899d3ba471d55c7610c677d4ccf2c9c500c0

BOOST_TARBALL_NAME = $(notdir $(BOOST_URL))
BOOST_TARBALL = $(DOWNLOAD_DIR)/$(BOOST_TARBALL_NAME)
BOOST_BASE_NAME = $(patsubst %.tar.bz2,%,$(BOOST_TARBALL_NAME))
BOOST_SRC = $(OUT)/src/$(BOOST_BASE_NAME)
BOOST_PATCHES_DIR = $(topdir)/lib/boost/patches
BOOST_PATCHES = $(addprefix $(BOOST_PATCHES_DIR)/,$(shell cat $(BOOST_PATCHES_DIR)/series))

$(BOOST_TARBALL): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(BOOST_URL) $(BOOST_ALTERNATIVE_URL) $(BOOST_MD5) $(DOWNLOAD_DIR)

BOOST_UNTAR_STAMP = $(OUT)/src/stamp-$(BOOST_BASE_NAME)
$(BOOST_UNTAR_STAMP): $(BOOST_TARBALL) $(BOOST_PATCHES_DIR)/series $(BOOST_PATCHES) | $(OUT)/src/dirstamp
	@$(NQ)echo "  UNTAR   $(BOOST_TARBALL_NAME)"
	$(Q)rm -rf $(BOOST_SRC)
	$(Q)tar xjfC $< $(OUT)/src
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

# Prevent Boost from using the deprecated std::unary_function class
BOOST_CPPFLAGS += -DBOOST_NO_CXX98_FUNCTION_BASE
