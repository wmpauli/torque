 
PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} -I$(PROG_ROOT)/../include --coverage `xml2-config --cflags`
AM_CXXFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I${PROG_ROOT}/${PBS_MACH} -I$(PROG_ROOT)/../include --coverage `xml2-config --cflags`
AM_LIBS=`xml2-config --libs`

lib_LTLIBRARIES = libtest_pbsnode.la

AM_LDFLAGS = @CHECK_LIBS@ $(lib_LTLIBRARIES) $(AM_LIBS)

check_PROGRAMS = test_pbsnode

libtest_pbsnode_la_SOURCES = scaffolding.c ${PROG_ROOT}/pbsnode.cpp ${PROG_ROOT}/execution_slot_tracker.cpp
libtest_pbsnode_la_LDFLAGS = @CHECK_LIBS@ ${AM_LIBS} -shared -lgcov

test_pbsnode_SOURCES = test_pbsnode.c

check_SCRIPTS = ${PROG_ROOT}/../test/coverage_run.sh
TESTS = ${check_PROGRAMS} ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov_core *.lo
