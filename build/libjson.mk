JSON_SOURCES = \
	$(SRC)/json/Serialize.cxx \
	$(SRC)/json/ParserOutputStream.cxx \
	$(SRC)/json/Boost.cxx
JSON_CPPFLAGS = -DBOOST_JSON_STANDALONE

ifeq ($(CLANG),y)
  # libc++11 doesn't have std::memory_resource; disable the #warning
  # "Support for std::memory_resource is required to use Boost.JSON
  # standalone, using std::experimental::memory_resource as
  # fallback"
  JSON_CPPFLAGS += -Wno-\#warnings
endif

$(eval $(call link-library,json,JSON))
