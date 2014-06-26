BOOST = $(topdir)/lib/boost
BOOST_CPPFLAGS = \
	-DBOOST_NO_IOSTREAM \
	-DBOOST_NO_IOSFWD \
	-DBOOST_NO_STRINGSTREAM \
	-DBOOST_NO_WSTREAMBUF \
	-isystem $(BOOST)/assert/include \
	-isystem $(BOOST)/core/include \
	-isystem $(BOOST)/detail/include \
	-isystem $(BOOST)/functional/include \
	-isystem $(BOOST)/function_types/include \
	-isystem $(BOOST)/integer/include \
	-isystem $(BOOST)/intrusive/include \
	-isystem $(BOOST)/move/include \
	-isystem $(BOOST)/mpl/include \
	-isystem $(BOOST)/preprocessor/include \
	-isystem $(BOOST)/static_assert/include \
	-isystem $(BOOST)/tti/include \
	-isystem $(BOOST)/type_traits/include \
	-isystem $(BOOST)/config/include
