AM_CPPFLAGS = -I$(top_builddir)/src/lib -I$(top_srcdir)/src/lib
AM_CPPFLAGS += $(BOOST_INCLUDES)
AM_CPPFLAGS += -DTEST_DATA_DIR=\"$(srcdir)/testdata\"
AM_CXXFLAGS = $(BUNDY_CXXFLAGS)

CLEANFILES = *.gcno *.gcda

TESTS_ENVIRONMENT = \
	$(LIBTOOL) --mode=execute $(VALGRIND_COMMAND)

TESTS =
if HAVE_GTEST
TESTS += run_unittests
run_unittests_SOURCES = run_unittests.cc
run_unittests_SOURCES += benchmark_unittest.cc
run_unittests_SOURCES += loadquery_unittest.cc

run_unittests_CPPFLAGS = $(AM_CPPFLAGS) $(GTEST_INCLUDES)
run_unittests_LDFLAGS = $(AM_LDFLAGS) $(GTEST_LDFLAGS)
run_unittests_LDADD  = $(top_builddir)/src/lib/bench/libbundy-bench.la
run_unittests_LDADD += $(top_builddir)/src/lib/dns/libbundy-dns++.la
run_unittests_LDADD += $(top_builddir)/src/lib/util/libbundy-util.la
run_unittests_LDADD += $(top_builddir)/src/lib/util/unittests/libutil_unittests.la
run_unittests_LDADD += $(top_builddir)/src/lib/exceptions/libbundy-exceptions.la
run_unittests_LDADD += $(GTEST_LDADD)
endif

noinst_PROGRAMS = $(TESTS)

EXTRA_DIST = testdata/query.txt
