# On Linux, we expect Boost to be installed already, and on all other
# targets, it is installed by thirdparty.py.  Neither needs special
# compiler/linker flags.

ifeq ($(USE_THIRDPARTY_LIBS),y)

BOOST_URL = http://netcologne.dl.sourceforge.net/project/boost/boost/1.60.0/boost_1_60_0.tar.bz2
BOOST_MD5 = 65a840e1a0b13a558ff19eeb2c4f0cbe

BOOST_TARBALL_NAME = $(notdir $(BOOST_URL))
BOOST_TARBALL = $(DOWNLOAD_DIR)/$(BOOST_TARBALL_NAME)
BOOST_BASE_NAME = $(patsubst %.tar.bz2,%,$(BOOST_TARBALL_NAME))
BOOST_SRC = $(OUT)/src/$(BOOST_BASE_NAME)
BOOST_PATCHES_DIR = $(topdir)/lib/boost/patches
BOOST_PATCHES = $(addprefix $(BOOST_PATCHES_DIR)/,$(shell cat $(BOOST_PATCHES_DIR)/series))

$(BOOST_TARBALL): | $(DOWNLOAD_DIR)/dirstamp
	@$(NQ)echo "  GET     $@"
	$(Q)./build/download.py $(BOOST_URL) $(BOOST_MD5) $(DOWNLOAD_DIR)

BOOST_UNTAR_STAMP = $(OUT)/src/stamp-$(BOOST_BASE_NAME)
$(BOOST_UNTAR_STAMP): $(BOOST_TARBALL) $(BOOST_PATCHES_DIR)/series $(BOOST_PATCHES)
	@$(NQ)echo "  UNTAR   $(BOOST_TARBALL_NAME)"
	$(Q)rm -rf $(BOOST_SRC)
	$(Q)tar xjfC $< $(OUT)/src
	$(Q)cd $(BOOST_SRC) && QUILT_PATCHES=$(abspath $(BOOST_PATCHES_DIR)) quilt push -a -q
	@touch $@

BOOST_SYMLINK_STAMP = $(THIRDPARTY_LIBS_DIR)/stamp-boost
$(BOOST_SYMLINK_STAMP): $(BOOST_UNTAR_STAMP)
	$(Q)rm -rf $(THIRDPARTY_LIBS_ROOT)/include/boost
	$(Q)ln -s $(abspath $(BOOST_SRC)/boost) $(THIRDPARTY_LIBS_ROOT)/include/boost
	@touch $@

.PHONY: boost
boost: $(BOOST_SYMLINK_STAMP)

endif

BOOST_LDLIBS =

# reduce Boost header bloat a bit
BOOST_CPPFLAGS = -DBOOST_NO_IOSTREAM -DBOOST_MATH_NO_LEXICAL_CAST
BOOST_CPPFLAGS += -DBOOST_UBLAS_NO_STD_CERR
