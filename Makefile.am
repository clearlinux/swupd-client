EXTRA_DIST = COPYING findstatic.pl

AM_CFLAGS = -fPIC -Iinclude/ -O2 -g -Wall -W -Wformat-security -D_FORTIFY_SOURCE=2 -fno-common -std=gnu99
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swupd

swupd_SOURCES = \
	src/swupd.c \
	$(swupd_hashdump_SOURCES) \
	$(swupd_update_SOURCES) \
	$(swupd_check_update_SOURCES) \
	$(swupd_verify_SOURCES) \
	$(clr_bundle_add_SOURCES) \
	$(clr_bundle_rm_SOURCES) \
	$(swupd_search_SOURCES)

check_PROGRAMS = \
	swupd_bsdiff_bench \
	swupd_hashtest \
	swupd_listtest \
	swupd_fuzz \
	swupd_locktest \
	swupd_sig_verifytest

SWUPD_COMMON_SOURCES = \
	src/curl.c \
	src/delta.c \
	src/download.c \
	src/filedesc.c \
	src/hash.c \
	src/helpers.c \
	src/heuristics.c \
	src/list.c \
	src/lock.c \
	src/manifest.c \
	src/packs.c \
	src/signature.c \
	src/staging.c \
	src/stats.c \
	src/subscriptions.c \
	src/update.c \
	src/version.c \
	src/xattrs.c \
	src/globals.c \
	src/scripts.c \
	src/bundle.c

lib_LTLIBRARIES = libswupd.la
libswupd_la_SOURCES = $(SWUPD_COMMON_SOURCES)
libswupd_la_LIBADD = $(SWUPD_COMPRESSION_LIBS) $(openssl_LIBS) $(curl_LIBS) $(bsdiff_LIBS)
# Library version changes according to the libtool convention:
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
LIBSWUPD_CURRENT=2
LIBSWUPD_REVISION=0
LIBSWUPD_AGE=0
libswupd_la_LDFLAGS = \
	-version-info $(LIBSWUPD_CURRENT):$(LIBSWUPD_REVISION):$(LIBSWUPD_AGE)

swupd_update_SOURCES = src/main.c
swupd_verify_SOURCES = src/verify.c
swupd_check_update_SOURCES = src/check_update.c
swupd_search_SOURCES = src/search.c

clr_bundle_add_SOURCES = src/clr_bundle_add.c
clr_bundle_rm_SOURCES = src/clr_bundle_rm.c

swupd_hashdump_SOURCES = src/hashdump.c

swupd_bsdiff_bench_SOURCES = test/bsdiff_bench.c
swupd_hashtest_SOURCES = test/hash_test.c
swupd_listtest_SOURCES = test/listtest.c
swupd_fuzz_SOURCES = test/fuzz.c
swupd_locktest_SOURCES = test/locktest.c
swupd_sig_verifytest_SOURCES = test/signature_verify_test.c


AM_CPPFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
SWUPD_COMPRESSION_LIBS = $(lzma_LIBS) $(zlib_LIBS) $(bzip2_LIBS)
SWUPD_CORE_LIBS = libswupd.la ${curl_LIBS} $(openssl_LIBS) $(SWUPD_COMPRESSION_LIBS) $(bsdiff_LIBS)

swupd_LDADD = $(SWUPD_CORE_LIBS) $(pthread_LIBS)

swupd_bsdiff_bench_LDADD = $(SWUPD_CORE_LIBS)
swupd_hashtest_LDADD = $(SWUPD_CORE_LIBS) $(pthread_LIBS)
swupd_listtest_LDADD = $(SWUPD_CORE_LIBS)
swupd_fuzz_LDADD = $(SWUPD_CORE_LIBS) $(pthread_LIBS)
swupd_locktest_LDADD = $(SWUPD_CORE_LIBS) $(pthread_LIBS)
swupd_sig_verifytest_LDADD = $(SWUPD_CORE_LIBS)

noinst_HEADERS = $(top_srcdir)/include/*

swupdcertsdir = @swupdcertsdir@
SWUPD_CERTS = certs/157753a5.0 \
	certs/425b0f6b.0 \
	certs/425b0f6b.key \
	certs/8d28ae65.0 \
	certs/d6325660.0 \
	certs/d6325660.1
swupdcerts_DATA = $(SWUPD_CERTS)

EXTRA_DIST += \
	data/check-update.service \
	data/check-update.timer \
	$(SWUPD_CERTS)

DISTCHECK_CONFIGURE_FLAGS = \
        --with-systemdsystemunitdir=$$dc_install_base/$(systemdunitdir)

systemdunitdir = @SYSTEMD_UNITDIR@

systemdunit_DATA = \
        data/check-update.service \
        data/check-update.timer

systemdunit-install-local:
	mkdir -p $(DESTDIR)$(systemdunitdir)/multi-user.target.wants/
	ln -sf ../check-update.timer $(DESTDIR)$(systemdunitdir)/multi-user.target.wants/check-update.timer
install-data-local: systemdunit-install-local

systemdunit-uninstall-local:
	rm -f $(DESTDIR)$(systemdunitdir)/multi-user.target.wants/check-update.timer
uninstall-local: systemdunit-uninstall-local

distclean-local:
	rm -rf aclocal.m4 ar-lib autom4te.cache config.guess config.h.in config.h.in~ config.sub configure depcomp install-sh ltmain.sh m4 Makefile.in missing compile

install-exec-hook:
	perl findstatic.pl */*.o | grep -v Checking ||:


TEST_EXTENSIONS = .py

if ENABLE_TESTS
tap_driver = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/tap-driver.sh

LOG_DRIVER = $(tap_driver)
PY_LOG_DRIVER = $(tap_driver)

TESTS = $(dist_check_SCRIPTS)

dist_check_SCRIPTS = \
	functional-tests.py
endif

if COVERAGE
AM_CFLAGS += --coverage

coverage: coverage-clean
	mkdir -p coverage
	lcov --compat-libtool --directory . --capture --output-file coverage/report
	genhtml -o coverage/ coverage/report

coverage-clean:
	rm -rf coverage
endif

release:
	@git rev-parse v$(PACKAGE_VERSION) &> /dev/null; \
	if [ "$$?" -eq 0 ]; then \
		echo "Error: Release for $(PACKAGE_VERSION) already exists."; \
		echo "Bump version in configure.ac before releasing."; \
		exit 1; \
	fi
	@git tag -a -m "$(PACKAGE_NAME) release $(PACKAGE_VERSION)" v$(PACKAGE_VERSION)
	@printf "\nNew release $(PACKAGE_VERSION) tagged!\n\n"
